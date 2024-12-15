#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui
{
class TMPL_Q_MAIN_WINDOW_TMPL;
}
QT_END_NAMESPACE

class TMPL_Q_MAIN_WINDOW_TMPL : public QMainWindow
{
    Q_OBJECT

public:
    TMPL_Q_MAIN_WINDOW_TMPL(QWidget *parent = nullptr);
    ~TMPL_Q_MAIN_WINDOW_TMPL();

private:
    Ui::TMPL_Q_MAIN_WINDOW_TMPL *ui;
};
#endif // MAINWINDOW_H
