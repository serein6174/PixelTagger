#include <QApplication>

#include "view/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.resize(1100, 720);
    window.show();

    return app.exec();
}
