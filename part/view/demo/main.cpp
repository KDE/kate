#include "katemessagewidget.h"
#include "messageinterface.h"

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>

#include <QtGui/QVBoxLayout>
#include <QtGui/QLabel>

int main(int argc, char **argv)
{
    KCmdLineArgs::init(argc, argv, "demo", 0, KLocalizedString(), "version", KLocalizedString());

    KApplication app;
    app.setQuitOnLastWindowClosed( true );

    QWidget *mainWidget = new QWidget;
    QVBoxLayout* l = new QVBoxLayout;

    KTextEditor::Message* m = new KTextEditor::Message(KTextEditor::Message::Information, "This is an information message, awesome!");
    QAction* a = new QAction("Action 1", m);
    m->addAction(a);
    a = new QAction("Action 2", m);
    m->addAction(a, false);

    KateMessageWidget* mw = new KateMessageWidget(m);
    l->addWidget(mw);
    mw = new KateMessageWidget(m);
    l->addWidget(mw);
    l->addWidget(new QLabel("some text in the middle", mw));
    mw = new KateMessageWidget(m);
    l->addWidget(mw);
    l->addWidget(new QLabel("some text at the bottom", mw));

    mainWidget->setLayout(l);
    mainWidget->show();

    return app.exec();
}

// kate: space-indent on; indent-width 4; encoding utf-8; replace-tabs on;
