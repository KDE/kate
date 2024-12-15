#include "TMPL_Q_MAIN_WINDOW_TMPL.h"
#include "./ui_TMPL_Q_MAIN_WINDOW_TMPL.h"

TMPL_Q_MAIN_WINDOW_TMPL::TMPL_Q_MAIN_WINDOW_TMPL(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TMPL_Q_MAIN_WINDOW_TMPL)
{
    ui->setupUi(this);
}

TMPL_Q_MAIN_WINDOW_TMPL::~TMPL_Q_MAIN_WINDOW_TMPL()
{
    delete ui;
}
