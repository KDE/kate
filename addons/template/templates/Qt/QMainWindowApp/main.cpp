#include "TMPL_Q_MAIN_WINDOW_TMPL.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TMPL_Q_MAIN_WINDOW_TMPL w;
    w.show();
    return a.exec();
}
