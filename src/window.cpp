#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QDirIterator>
#include <QProgressDialog>
#include <QTextStream>
#include <QCompleter>
#include <QHeaderView>
#include <QDesktopServices>
#include <QSettings>

#include "window.h"

/* Setup the graphical user interface (GUI) */
Window::Window(QWidget *parent) : QWidget(parent) {
    QSettings settings;

    fileLabel = new QLabel(tr("File name:"));
    textLabel = new QLabel(tr("Containing text:"));
    directoryLabel = new QLabel(tr("In directory:"));
    filesFoundLabel = new QLabel(tr("0 files(s) found"));

    browseButton = new QPushButton(tr("&Browse..."), this);
    connect(browseButton, &QAbstractButton::clicked, this, &Window::browse);
    findButton = new QPushButton(tr("&Find"), this);
    connect(findButton, &QAbstractButton::clicked, this, &Window::find);

    recursiveCheckBox = new QCheckBox(tr("Recursive (file)"), this);
    sensitiveCheckBox = new QCheckBox(tr("Match case (file+text)"), this);
    regexCheckBox = new QCheckBox(tr("Enable Regex (file+text)"), this);
    wholeWordCheckBox = new QCheckBox(tr("Whole word (text)"), this);
    hiddenCheckBox = new QCheckBox(tr("Hidden files"), this);
    systemFilesCheckBox = new QCheckBox(tr("System files"), this);

    recursiveCheckBox->setChecked(settings.value("recursiveCheckBox", true).toBool());
    sensitiveCheckBox->setChecked(settings.value("sensitiveCheckBox", false).toBool());
    regexCheckBox->setChecked(settings.value("regexCheckBox", false).toBool());
    wholeWordCheckBox->setChecked(settings.value("wholeWordCheckBox", false).toBool());
    hiddenCheckBox->setChecked(settings.value("hiddenCheckBox", false).toBool());
    systemFilesCheckBox->setChecked(settings.value("systemFilesCheckBox", false).toBool());

    wholeWordCheckBox->setEnabled(!regexCheckBox->isChecked());

    connect(recursiveCheckBox, &QAbstractButton::clicked, this, &Window::recursiveCheckBox_onclick);
    connect(sensitiveCheckBox, &QAbstractButton::clicked, this, &Window::sensitiveCheckBox_onclick);
    connect(regexCheckBox, &QAbstractButton::clicked, this, &Window::regexCheckBox_onclick);
    connect(wholeWordCheckBox, &QAbstractButton::clicked, this, &Window::wholeWordCheckBox_onclick);
    connect(hiddenCheckBox, &QAbstractButton::clicked, this, &Window::hiddenCheckBox_onclick);
    connect(systemFilesCheckBox, &QAbstractButton::clicked, this, &Window::systemFilesCheckBox_onclick);

    fileComboBox = createComboBox(tr("*"));
    textComboBox = createComboBox();
    directoryComboBox = createComboBox(settings.value("lastDirectory", QDir::currentPath()).toString());

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

inline void Window::recursiveCheckBox_onclick() {
    QSettings settings;
    settings.setValue("recursiveCheckBox", recursiveCheckBox->isChecked());
}

inline void Window::sensitiveCheckBox_onclick() {
    QSettings settings;
    settings.setValue("sensitiveCheckBox", sensitiveCheckBox->isChecked());
}

inline void Window::regexCheckBox_onclick() {
    wholeWordCheckBox->setEnabled(!regexCheckBox->isChecked());

    QSettings settings;
    settings.setValue("regexCheckBox", regexCheckBox->isChecked());
    settings.setValue("wholeWordCheckBox", wholeWordCheckBox->isChecked());
}

inline void Window::wholeWordCheckBox_onclick() {
    QSettings settings;
    settings.setValue("wholeWordCheckBox", wholeWordCheckBox->isChecked());
}

inline void Window::hiddenCheckBox_onclick() {
    QSettings settings;
    settings.setValue("hiddenCheckBox", hiddenCheckBox->isChecked());
}

inline void Window::systemFilesCheckBox_onclick() {
    QSettings settings;
    settings.setValue("systemFilesCheckBox", systemFilesCheckBox->isChecked());
}

void Window::browse() {
    QSettings settings;
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Find Files"), directoryComboBox->currentText().isEmpty() ? QDir::currentPath() : directoryComboBox->currentText());

    if(!directory.isEmpty()) {
        if(directoryComboBox->findText(directory) == -1) {
            directoryComboBox->addItem(directory);
        }

        directoryComboBox->setCurrentIndex(directoryComboBox->findText(directory));
        settings.setValue("lastDirectory", directory);
    }
}

static void updateComboBox(QComboBox *comboBox) {
    if(comboBox->findText(comboBox->currentText(), Qt::MatchExactly | Qt::MatchCaseSensitive) == -1) {
        comboBox->addItem(comboBox->currentText());
    }
}

void Window::find() {
    QSettings settings;
    static bool shown = settings.value("licenseShown", false).toBool();

    if(!shown) {
        QFile license(":/QT_BSD_LICENSE.txt");

        if(!license.open(QIODevice::ReadOnly | QIODevice::Text)) {
            exit(1);
        }

        QByteArray text = license.readAll();

        license.close();

        QMessageBox::warning(findButton->parentWidget(), tr("Find Files QT BSD License"), QString(text));

        shown = true;
        settings.setValue("licenseShown", true);
    }

    filesTable->setRowCount(0); // reset results

    const QString fileName = fileComboBox->currentText().isEmpty() ? "*" : fileComboBox->currentText(); // use '*' if input is empty
    const QString text = textComboBox->currentText(); // text to search for inside of files
    const QString path = directoryComboBox->currentText();
    const QRegularExpression regex(fileName, sensitiveCheckBox->isChecked() ? QRegularExpression::NoPatternOption : QRegularExpression::CaseInsensitiveOption); // only used if user requested regex

    if(regexCheckBox->isChecked() && !regex.isValid()) {
        filesFoundLabel->setText(tr("Invalid Regex!"));
        filesFoundLabel->setStyleSheet("QLabel { color : red; }");
        QApplication::beep();
        return;
    } else {
        filesFoundLabel->setText(tr("Searching..."));
        filesFoundLabel->setStyleSheet("QLabel { color : black; }");
        qApp->processEvents();
    }

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
        QDirIterator iterator(currentDir.path(), QStringList(regexCheckBox->isChecked() ? "*" : fileName), entryListFilters, QDirIterator::Subdirectories);

        // recursive loop over all files
        while(iterator.hasNext()) {
            files.append(iterator.next());
        }
    } else {
        files = currentDir.entryList(QStringList(regexCheckBox->isChecked() ? "*" : fileName), entryListFilters);
    }

    // QDirIterator and QDir only accept wildcards ("*" etc.), so we have to re-filter the results if user requested Regular Expressions (Regex)
    if(regexCheckBox->isChecked()) {
        QFileInfo currentFile;

        for(int i = files.size(); i-- > 0;) {
            currentFile.setFile(files[i]);

            if(!regex.match(currentFile.fileName()).hasMatch()) {
                files.removeAt(i); // filter out all files which don't match the regex
            }
        }
    }

    // if we got a search text search for it in the files
    if(!text.isEmpty()) {
        files = findFiles(files, text, regex);
    }

    // show results
    showFiles(files);
}

QStringList Window::findFiles(const QStringList &files, const QString &text, const QRegularExpression &regex) {
    QProgressDialog progressDialog(this);
    progressDialog.setCancelButtonText(tr("&Cancel"));
    progressDialog.setRange(0, files.size());
    progressDialog.setWindowTitle(tr("Find Files"));

    QStringList foundFiles;

    const bool doRegex = regexCheckBox->isChecked();
    const bool wholeWord = wholeWordCheckBox->isChecked();
    const QRegularExpression wholeWordRegex(QString("\\W") + QRegularExpression::escape(text) + QString("\\W")); // insert two non-word characters and use input text as non-regex

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

                if(doRegex ? line.contains(regex) : wholeWord ? line.contains(wholeWordRegex) : line.contains(text, sensitiveCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
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
