#include "tstestapp.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TsTestApp w;
    w.show();

    return app.exec();
}
