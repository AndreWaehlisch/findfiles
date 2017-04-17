#include <QApplication>

#include "window.h"

int main(int argc, char *argv[]) {
    Q_INIT_RESOURCE(resources); // init resources (needed for static build)
    QCoreApplication::setOrganizationName("AndreWaehlisch");
    QCoreApplication::setApplicationName("Find Files");
    const QApplication app(argc, argv);
    const QIcon mainIcon(":/icon.ico");
    Window window;
    window.setWindowIcon(mainIcon);
    window.show();
    return app.exec();
}
