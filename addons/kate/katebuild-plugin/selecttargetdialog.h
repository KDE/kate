#ifndef SELECT_TARGET_DIALOG_H
#define SELECT_TARGET_DIALOG_H

#include <kdialog.h>

#include <QStringList>

#include <map>

class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;


class SelectTargetDialog : public KDialog
{
    Q_OBJECT
    public:
        SelectTargetDialog(QWidget* parent);

        void setTargets(const std::map<QString, QString>& _targets);
        QString selectedTarget() const;

    protected:
        virtual bool eventFilter(QObject *obj, QEvent *event);

    private slots:
        void slotFilterTargets(const QString& filter);
        void slotCurrentItemChanged(QListWidgetItem* currentItem);

    private:
        QStringList m_allTargets;
        QLineEdit* m_targetName;
        QListWidget* m_targetsList;
        QLabel* m_command;

        const std::map<QString, QString>* m_targets;
};

#endif

// kate: space-indent on; indent-width 4; replace-tabs on;
