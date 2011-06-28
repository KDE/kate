#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextCursor>
#include "foldingtree.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QTextCursor *cursor;
    void print(int position);
    FoldingTree *folding_tree;

    // Start of Test Methods
    void testMergeDriver();

private slots:
    void on_pushButton_5_released();
    void on_pushButton_4_released();
    void on_pushButton_3_released();
    void on_pushButton_2_released();
};

#endif // MAINWINDOW_H
