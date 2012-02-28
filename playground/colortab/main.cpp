#include <QString>
#include <QDebug>
#include <KApplication>
#include <QWidget>
#include <KCmdLineArgs>
#include <QCheckBox>
#include <QPushButton>
#include <QGridLayout>

#include "katecolortreewidget.h"


int main(int argc, char *argv[])
{
  KCmdLineArgs::init(argc, argv, QByteArray("foo"), QByteArray("bar"), KLocalizedString(), QByteArray());
  KCmdLineOptions options;
  KCmdLineArgs::addCmdLineOptions( options );
  KApplication app;

  QWidget* top = new QWidget();
  top->resize(500, 400);
  top->show();
  
  QGridLayout* l = new QGridLayout(top);
  top->setLayout(l);

  KateColorTreeWidget* w = new KateColorTreeWidget();
  
  QPushButton* btnSelectAll = new QPushButton("Use System Colors", top);
  QPushButton* btnSave = new QPushButton("Save to /tmp/test.cfg", top);
  QPushButton* btnLoad = new QPushButton("Load from /tmp/test.cfg", top);

  l->addWidget(w, 0, 0, 1, 3);
  l->addWidget(btnLoad, 1, 0);
  l->addWidget(btnSelectAll, 1, 1);
  l->addWidget(btnSave, 1, 2);


  QObject::connect(btnSelectAll, SIGNAL(clicked()), w, SLOT(selectDefaults()));
  QObject::connect(btnSave, SIGNAL(clicked()), w, SLOT(testWriteConfig()));
  QObject::connect(btnLoad, SIGNAL(clicked()), w, SLOT(testReadConfig()));


  return app.exec();
}

// kate: replace-tabs on; indent-width 2;
