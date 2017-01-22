#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QStringList>
#include <QString>
#include <QRegularExpression>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QCheckBox>
#include <QDir>

class Window : public QWidget {
        Q_OBJECT

    public:
        Window(QWidget *parent = 0);

    private slots:
        void browse();
        void find();
        void openFileOfItem(int row, int column);
        void regexCheckBox_onclick();

    private:
        QStringList findFiles(const QStringList &files, const QString &text, const QRegularExpression &regex);
        void showFiles(const QStringList &files);
        QComboBox *createComboBox(const QString &text = QString());
        void createFilesTable();

        QComboBox *fileComboBox;
        QComboBox *textComboBox;
        QComboBox *directoryComboBox;
        QLabel *fileLabel;
        QLabel *textLabel;
        QLabel *directoryLabel;
        QLabel *filesFoundLabel;
        QPushButton *browseButton;
        QPushButton *findButton;
        QTableWidget *filesTable;
        QCheckBox *recursiveCheckBox;
        QCheckBox *sensitiveCheckBox;
        QCheckBox *regexCheckBox;
        QCheckBox *wholeWordCheckBox;
        QCheckBox *hiddenCheckBox;
        QCheckBox *systemFilesCheckBox;

        QDir currentDir;
};

#endif
