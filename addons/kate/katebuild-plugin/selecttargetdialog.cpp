
#include "selecttargetdialog.h"


#include <QCoreApplication>
#include <QGridLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>


SelectTargetDialog::SelectTargetDialog(QWidget* parent)
:KDialog(parent)
,m_targetName(0)
,m_targetsList(0)
{
    setButtons( KDialog::Ok | KDialog::Cancel);

    QWidget* container = new QWidget();

    QLabel* label = new QLabel("Target:");
    m_targetName = new QLineEdit();
    m_targetsList = new QListWidget();

    QGridLayout* layout = new QGridLayout(this);

    layout->addWidget(label, 0, 0);
    layout->addWidget(m_targetName, 0, 1);

    layout->addWidget(m_targetsList, 1, 0, 1, 2);

    container->setLayout(layout);

    this->setMainWidget(container);

    connect(m_targetName, SIGNAL(textEdited(const QString&)), this, SLOT(slotFilterTargets(const QString&)));
    connect(m_targetsList, SIGNAL(itemActivated(QListWidgetItem*)), this, SLOT(accept()));
    m_targetName->installEventFilter(this);
    m_targetsList->installEventFilter(this);
}


void SelectTargetDialog::slotFilterTargets(const QString& filter)
{
    QStringList filteredTargets;
    if (filter.isEmpty()) {
        filteredTargets = m_allTargets;
    }
    else {
        filteredTargets = m_allTargets.filter(filter, Qt::CaseInsensitive);
    }
    m_targetsList->clear();
    m_targetsList->addItems(filteredTargets);
}


void SelectTargetDialog::fillTargets(const QStringList& targets)
{
    m_allTargets = targets;
    m_targetsList->clear();
    for(int i=0; i<targets.size(); i++) {
        m_targetsList->addItem(targets.at(i));
        if (targets.at(i) == "all") {
            setTargetName(targets.at(i));
        }
    }
}


void SelectTargetDialog::setTargetName(const QString& target)
{
    m_targetName->setText(target);
    m_targetName->selectAll();
    m_targetName->setFocus();
}


QString SelectTargetDialog::selectedTarget() const
{
    if (m_targetsList->currentItem() == 0) {
        return m_targetName->text();
    }
    return m_targetsList->currentItem()->text();
}


bool SelectTargetDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type()==QEvent::KeyPress) {
        QKeyEvent *keyEvent=static_cast<QKeyEvent*>(event);
        if (obj==m_targetName) {
            const bool forward2list = (keyEvent->key()==Qt::Key_Up)
            || (keyEvent->key()==Qt::Key_Down)
            || (keyEvent->key()==Qt::Key_PageUp)
            || (keyEvent->key()==Qt::Key_PageDown);
            if (forward2list) {
                QCoreApplication::sendEvent(m_targetsList,event);
                return true;
            }
        }
        else {
            const bool forward2input = (keyEvent->key()!=Qt::Key_Up)
            && (keyEvent->key()!=Qt::Key_Down)
            && (keyEvent->key()!=Qt::Key_PageUp)
            && (keyEvent->key()!=Qt::Key_PageDown)
            && (keyEvent->key()!=Qt::Key_Tab)
            && (keyEvent->key()!=Qt::Key_Backtab);
            if (forward2input) {
                QCoreApplication::sendEvent(m_targetName,event);
                return true;
            }
        }
    }
    return KDialog::eventFilter(obj, event);
}

// kate: space-indent on; indent-width 4; replace-tabs on;
