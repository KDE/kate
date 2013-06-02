#ifndef SELECT_TARGET_DIALOG_H
#define SELECT_TARGET_DIALOG_H

#include <kdialog.h>

#include <QStringList>

class QLineEdit;
class QListWidget;
class QListWidgetItem;


class SelectTargetDialog : public KDialog
{
    Q_OBJECT
    public:
        SelectTargetDialog(QWidget* parent);

        void fillTargets(const QStringList& targets);
        QString selectedTarget() const;
        void setTargetName(const QString& target);

    protected:
        virtual bool eventFilter(QObject *obj, QEvent *event);

    private slots:
        void slotFilterTargets(const QString& filter);

    private:
        QStringList m_allTargets;
        QLineEdit* m_targetName;
        QListWidget* m_targetsList;
};

#endif

// kate: space-indent on; indent-width 4; replace-tabs on;
