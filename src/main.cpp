#include <QApplication>

#include "app/Application.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    Application application;
    application.show();

    return app.exec();
}
