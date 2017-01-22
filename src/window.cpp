#include "window.h"

/* Setup the graphical user interface (GUI) */
Window::Window(QWidget *parent) : QWidget(parent) {
    fileLabel = new QLabel(tr("Named:"));
    textLabel = new QLabel(tr("Containing text:"));
    directoryLabel = new QLabel(tr("In directory:"));
    filesFoundLabel = new QLabel(tr("0 files(s) found"));

    browseButton = new QPushButton(tr("&Browse..."), this);
    connect(browseButton, &QAbstractButton::clicked, this, &Window::browse);
    findButton = new QPushButton(tr("&Find"), this);
    connect(findButton, &QAbstractButton::clicked, this, &Window::find);
    recursiveCheckBox = new QCheckBox(tr("Recursive"), this);
    sensitiveCheckBox = new QCheckBox(tr("Match case"), this);
    regexCheckBox = new QCheckBox(tr("Enable Regex"), this);
    wholeWordCheckBox = new QCheckBox(tr("Whole word"), this);
    hiddenCheckBox = new QCheckBox(tr("Hidden files"), this);
    systemFilesCheckBox = new QCheckBox(tr("System files"), this);

    fileComboBox = createComboBox(tr("*"));
    textComboBox = createComboBox();
    directoryComboBox = createComboBox(QDir::currentPath());

    createFilesTable();

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(fileLabel, 0, 0);
    mainLayout->addWidget(fileComboBox, 0, 1, 1, 3);

    mainLayout->addWidget(textLabel, 1, 0);
    mainLayout->addWidget(textComboBox, 1, 1, 1, 3);

    mainLayout->addWidget(directoryLabel, 2, 0);
    mainLayout->addWidget(directoryComboBox, 2, 1, 1, 2);
    mainLayout->addWidget(browseButton, 2, 3);

    mainLayout->addWidget(recursiveCheckBox, 3, 1);
    mainLayout->addWidget(sensitiveCheckBox, 3, 2);
    mainLayout->addWidget(hiddenCheckBox, 3, 3);

    mainLayout->addWidget(regexCheckBox, 4, 1);
    mainLayout->addWidget(wholeWordCheckBox, 4, 2);
    mainLayout->addWidget(systemFilesCheckBox, 4, 3);

    mainLayout->addWidget(filesTable, 5, 0, 1, 4);

    mainLayout->addWidget(filesFoundLabel, 6, 0, 1, 3);
    mainLayout->addWidget(findButton, 6, 3);

    setLayout(mainLayout);

    setWindowTitle(tr("Find Files"));
    resize(1000, 500);
}

void Window::browse() {
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Find Files"), QDir::currentPath());

    if(!directory.isEmpty()) {
        if(directoryComboBox->findText(directory) == -1) {
            directoryComboBox->addItem(directory);
        }

        directoryComboBox->setCurrentIndex(directoryComboBox->findText(directory));
    }
}

static void updateComboBox(QComboBox *comboBox) {
    if(comboBox->findText(comboBox->currentText(), Qt::MatchExactly | Qt::MatchCaseSensitive) == -1) {
        comboBox->addItem(comboBox->currentText());
    }
}

void Window::find() {
    filesTable->setRowCount(0); // reset results

    const QString fileName = fileComboBox->currentText().isEmpty() ? "*" : fileComboBox->currentText(); // use '*' if input is empty
    const QString text = textComboBox->currentText(); // text to search for inside of files
    const QString path = directoryComboBox->currentText();

    // add current input to history of combo boxes
    updateComboBox(fileComboBox);
    updateComboBox(textComboBox);
    updateComboBox(directoryComboBox);

    currentDir = QDir(path);
    QStringList files;

    QDir::Filters entryListFilters = QDir::Files | QDir::NoSymLinks;

    // "Hidden files"
    if(hiddenCheckBox->isChecked()) {
        entryListFilters |= QDir::Hidden;
    }

    // "Match case"
    if(sensitiveCheckBox->isChecked()) {
        entryListFilters |= QDir::CaseSensitive;
    }

    // "System files"
    if(systemFilesCheckBox->isChecked()) {
        entryListFilters |= QDir::System;
    }

    // find files
    if(recursiveCheckBox->isChecked()) {
        QDirIterator iterator(currentDir.path(), QStringList(fileName), entryListFilters, QDirIterator::Subdirectories); // TODO: path() the correct one here?

        // recursive loop over all files
        while(iterator.hasNext()) {
            files.append(iterator.next());
        }
    } else {
        files = currentDir.entryList(QStringList(fileName), entryListFilters);
    }

    // if we got a search text search for it in the files
    if(!text.isEmpty()) {
        files = findFiles(files, text);
    }

    // show results
    showFiles(files);
}

QStringList Window::findFiles(const QStringList &files, const QString &text) {
    QProgressDialog progressDialog(this);
    progressDialog.setCancelButtonText(tr("&Cancel"));
    progressDialog.setRange(0, files.size());
    progressDialog.setWindowTitle(tr("Find Files"));

    QStringList foundFiles;

    for(int i = 0; i < files.size(); ++i) {
        progressDialog.setValue(i);
        progressDialog.setLabelText(tr("Searching file number %1 of %2...").arg(i).arg(files.size()));
        qApp->processEvents();

        if(progressDialog.wasCanceled()) {
            break;
        }

        QFile file(currentDir.absoluteFilePath(files[i]));

        if(file.open(QIODevice::ReadOnly)) {
            QString line;
            QTextStream in(&file);

            while(!in.atEnd()) {
                if(progressDialog.wasCanceled()) {
                    break;
                }

                line = in.readLine();

                if(line.contains(text)) {
                    foundFiles << files[i];
                    break;
                }
            }
        }
    }

    return foundFiles;
}

void Window::showFiles(const QStringList &files) {
    for(int i = 0; i < files.size(); ++i) {
        QFile file(currentDir.absoluteFilePath(files[i]));
        qint64 size = QFileInfo(file).size();

        QTableWidgetItem *fileNameItem = new QTableWidgetItem(files[i]);
        fileNameItem->setFlags(fileNameItem->flags() ^ Qt::ItemIsEditable);
        QTableWidgetItem *sizeItem = new QTableWidgetItem(tr("%1 KB").arg(int((size + 1023) / 1024)));
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        sizeItem->setFlags(sizeItem->flags() ^ Qt::ItemIsEditable);

        int row = filesTable->rowCount();
        filesTable->insertRow(row);
        filesTable->setItem(row, 0, fileNameItem);
        filesTable->setItem(row, 1, sizeItem);
    }

    filesFoundLabel->setText(tr("%1 file(s) found").arg(files.size()) + (" (Double click on a file to open it)"));
    filesFoundLabel->setWordWrap(true);
}

QComboBox *Window::createComboBox(const QString &text) {
    /* From the Doc (http://doc.qt.io/qt-5/qcombobox.html#setCompleter):
     * "By default, for an editable combo box, a QCompleter that performs case insensitive inline completion is automatically created."
     * So we have to set that QCompleter to case-sensitive AFTER we setEdtiable to true. */
    QComboBox *comboBox = new QComboBox;
    comboBox->setEditable(true);
    comboBox->completer()->setCaseSensitivity(Qt::CaseSensitive);
    comboBox->setDuplicatesEnabled(true);
    comboBox->addItem(text);
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    return comboBox;
}

void Window::createFilesTable() {
    filesTable = new QTableWidget(0, 2);
    filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QStringList labels;
    labels << tr("Filename") << tr("Size");
    filesTable->setHorizontalHeaderLabels(labels);
    filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    filesTable->verticalHeader()->hide();
    filesTable->setShowGrid(false);
    connect(filesTable, &QTableWidget::cellActivated, this, &Window::openFileOfItem);
}

void Window::openFileOfItem(int row, int /* column */) {
    const QTableWidgetItem *item = filesTable->item(row, 0);
    QDesktopServices::openUrl(QUrl::fromLocalFile(currentDir.absoluteFilePath(item->text())));
}
