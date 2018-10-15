#include "tstestapp.h"
#include "../tabswitcherfilesmodel.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QPushButton>
#include <QAbstractTableModel>
#include <QStandardItem>
#include <QListView>
#include <QTreeView>
#include <QStringRef>
#include <QDebug>
#include <QIcon>
#include <QMenu>
#include <QLabel>

// https://www.fluentcpp.com/2017/09/22/make-pimpl-using-unique_ptr/
class TsTestApp::Impl
{
public:
    void insert_1_item()
    {
        auto icon = QIcon::fromTheme(QLatin1String("blurimage"));
        model.insertRow(0, new detail::FilenameListItem(icon, QLatin1String("abc.d"), QLatin1String("/home/user2/folder1/abc.d")));
        treeview1->resizeColumnToContents(0);
    }

    void remove_1_item()
    {
        model.removeRow(0);
        treeview1->resizeColumnToContents(0);
    }

    void set_items_cutoff_bug()
    {
        model.clear();
        auto icon = QIcon::fromTheme(QLatin1String("document-export"));

#define INS__(a, b) model.insertRow(model.rowCount(), new detail::FilenameListItem(icon, QLatin1String(a), QLatin1String(b)));

        INS__("multimedia-system.log", "/home/gregor/logs/notifications/multimedia-system.log")
        INS__("servicemenueditor", "/home/gregor/dev/src/kservicemenueditor-0.2a/servicemenueditor")
        INS__("kdesrc-build", "/home/gregor/kde/src/kdesrc-build/kdesrc-build")
        INS__("README.md (3)", "/home/gregor/node_modules/autolinker/README.md")
        INS__("package.json (3)", "/home/gregor/node_modules/autolinker/package.json")
        INS__("LICENSE (3)", "/home/gregor/node_modules/autolinker/LICENSE")
        INS__("package.json (2)", "/home/gregor/node_modules/asynckit/package.json")

        treeview1->resizeColumnToContents(0);
    }

public:
    detail::TabswitcherFilesModel model;
    QTreeView* treeview1;
};

TsTestApp::TsTestApp(QWidget *parent) :
    QMainWindow(parent),
    impl_(new TsTestApp::Impl)
{
    setGeometry(0, 0, 1024, 800);
    setCentralWidget(new QWidget(this));
    auto l = new QVBoxLayout();
    centralWidget()->setLayout(l);

    auto hl = new QHBoxLayout();
    l->addLayout(hl);

    auto buttonInsert1 = new QPushButton(QLatin1String("Ins 1 item"), this);
    connect(buttonInsert1, &QPushButton::clicked, this, [=] { impl_->insert_1_item(); });
    hl->addWidget(buttonInsert1);

    auto buttonRemove1 = new QPushButton(QLatin1String("Del 1 item"), this);
    connect(buttonRemove1, &QPushButton::clicked, this, [=] { impl_->remove_1_item(); });
    hl->addWidget(buttonRemove1);

    auto buttonSetTestSet1 = new QPushButton(QLatin1String("set_items_cutoff_bug"), this);
    connect(buttonSetTestSet1, &QPushButton::clicked, this, [=] { impl_->set_items_cutoff_bug(); });
    hl->addWidget(buttonSetTestSet1);

    impl_->treeview1 = new QTreeView(this);
    l->addWidget(impl_->treeview1);
    impl_->treeview1->setHeaderHidden(true);
    impl_->treeview1->setRootIsDecorated(false);

    auto icon = QIcon::fromTheme(QLatin1String("edit-undo"));
    impl_->model.insertRow(0, new detail::FilenameListItem(icon, QLatin1String("file1.h"), QLatin1String("/home/gm/projects/proj1/src/file1.h")));
    impl_->model.insertRow(0, new detail::FilenameListItem(icon, QLatin1String("file2.cpp"), QLatin1String("/home/gm/projects/proj1/src/file2.cpp")));
    impl_->model.insertRow(0, new detail::FilenameListItem(icon, QLatin1String("file3.py"), QLatin1String("/home/gm/dev/file3.py")));
    impl_->model.insertRow(0, new detail::FilenameListItem(icon, QLatin1String("file3kjaskdfkljasdfklj089asdfkjklasdjf90asdfsdfkj.py"), QLatin1String("/home/gm/dev/file3kjaskdfkljasdfklj089asdfkjklasdjf90asdfsdfkj.py")));
    impl_->model.insertRow(0, new detail::FilenameListItem(icon, QLatin1String("file3.py"), QLatin1String("/home/gm/dev/proj2/asldfkjasdfk/asdlfkjasd;faf/;ajsdkfgjaskdfgasdf/file3.py")));
    //impl_->insert_a_item();
    //impl_->remove_a_item();

    impl_->model.rowCount();
    impl_->model.item(0);
    impl_->model.index(0, 0);

    impl_->treeview1->setModel(&impl_->model);
    impl_->treeview1->resizeColumnToContents(0);
    impl_->treeview1->resizeColumnToContents(1);

    auto listview1 = new QListView(this);
    l->addWidget(listview1);
    listview1->setModel(&impl_->model);

    auto treeview2 = new QTreeView(this);
    l->addWidget(treeview2);
}

TsTestApp::~TsTestApp()
{
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    TsTestApp w;
    w.show();

    return app.exec();
}
