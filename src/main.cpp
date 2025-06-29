#include <QApplication>

#include "window.h"

int main(int argc, char *argv[])
{
    const QApplication app(argc, argv);
    app.setOrganizationName("AndreWaehlisch");
    app.setApplicationName("Find Files");

    const QIcon mainIcon(":/icon.ico");
    Window window;
    window.setWindowIcon(mainIcon);
    window.show();

    return app.exec();
}
