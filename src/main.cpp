#include <QApplication>

#include "window.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName("AndreWaehlisch");
    QCoreApplication::setApplicationName("Find Files");
    const QApplication app(argc, argv);
    const QIcon mainIcon(":/icon.ico");
    Window window;
    window.setWindowIcon(mainIcon);
    window.show();
    return app.exec();
}
