#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QtCore/qdebug.h>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->textEdit->setReadOnly(true);
    folding_tree = new FoldingTree();
    ui->textEdit->setText(folding_tree->toString());
}

MainWindow::~MainWindow()
{
    delete ui;
}

// insert Start Node
void MainWindow::on_pushButton_2_released()
{
  int position = ui->textEdit_2->toPlainText().toInt();
  folding_tree->insertStartNode(position);
  ui->textEdit->selectAll();
  ui->textEdit->clear();
  QString string = folding_tree->toString();
  ui->textEdit->setPlainText(string);
  //QMessageBox msgBox;
  //msgBox.setText(QString("x = %1").arg(position));
  //msgBox.exec();
}


// Delete node
void MainWindow::on_pushButton_3_released()
{
  int position = ui->textEdit_3->toPlainText().toInt();
  folding_tree->deleteNode(position);
  ui->textEdit->selectAll();
  ui->textEdit->clear();
  QString string = folding_tree->toString();
  ui->textEdit->setPlainText(string);
}


// insert End node
void MainWindow::on_pushButton_4_released()
{
  int position = ui->textEdit_4->toPlainText().toInt();
  folding_tree->insertEndNode(position);
  ui->textEdit->selectAll();
  ui->textEdit->clear();
  QString string = folding_tree->toString();
  ui->textEdit->setPlainText(string);
}

// driver method
void MainWindow::on_pushButton_5_released()
{
  //testMergeDriver();
}

void MainWindow::on_pushButton_6_released()
{
  ui->textEdit->selectAll();
  ui->textEdit->clear();
  QString string = folding_tree->toString();
  ui->textEdit->setPlainText(string);
}

// Start of Testing Methods
/*
void MainWindow::testMergeDriver()
{
  QVector <FoldingNode *> list1;
  list1.append(new FoldingNode(1,1,NULL));
  list1.append(new FoldingNode(3,1,NULL));
  list1.append(new FoldingNode(6,1,NULL));
  list1.append(new FoldingNode(9,1,NULL));

  QVector <FoldingNode *> list2;
  list2.append(new FoldingNode(2,1,NULL));
  list2.append(new FoldingNode(4,1,NULL));
  list2.append(new FoldingNode(5,1,NULL));
  list2.append(new FoldingNode(10,1,NULL));

  qDebug()<<QString("new output:");
  folding_tree->mergeChildren(list1,list2);
  for (int i = 0 ; i < list1.size() ; ++ i) {
    qDebug()<<QString("%1").arg(list1[i]->nodeNo);
  }
  qDebug()<<QString("*****");
}*/

void MainWindow::on_checkBox_stateChanged(int newState)
{
    if (newState)
      FoldingTree::displayDetails = true;
    else
      FoldingTree::displayDetails = false;
}

void MainWindow::on_checkBox_2_stateChanged(int newState)
{
  if (newState)
    FoldingTree::displayChildrenDetails = true;
  else
    FoldingTree::displayChildrenDetails = false;
}

void MainWindow::on_checkBox_3_stateChanged(int newState)
{
  if (newState)
    FoldingTree::displayDuplicates = true;
  else
    FoldingTree::displayDuplicates = false;
}
