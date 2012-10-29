#include "katemessagewidget.h"
#include "messageinterface.h"

#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kdebug.h>

#include <QtGui/QVBoxLayout>

int main(int argc, char **argv)
{
    KCmdLineArgs::init(argc, argv, "demo", 0, KLocalizedString(), "version", KLocalizedString());

    KApplication app;
    app.setQuitOnLastWindowClosed( false );

    QWidget *mainWidget = new QWidget;
    QVBoxLayout* l = new QVBoxLayout;

    KTextEditor::Message* m = new KTextEditor::Message(KTextEditor::Message::Information, "This is an information message, awesome!");

    KateMessageWidget* mw = new KateMessageWidget(m);
    l->addWidget(mw);
    mw = new KateMessageWidget(m);
    l->addWidget(mw);
    mw = new KateMessageWidget(m);
    l->addWidget(mw);

    mainWidget->setLayout(l);
    mainWidget->show();

    return app.exec();
}

// kate: space-indent on; indent-width 4; encoding utf-8; replace-tabs on;
