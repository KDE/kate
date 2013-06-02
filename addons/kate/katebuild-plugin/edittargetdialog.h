#ifndef EDIT_TARGET_DIALOG_H
#define EDIT_TARGET_DIALOG_H

#include <kdialog.h>


#include <klineedit.h>

#include <QLabel>
#include <QStringList>
#include <QToolButton>


class EditTargetDialog : public KDialog
{
    Q_OBJECT
    public:
        EditTargetDialog(const QStringList& existingTargets, QWidget* parent = NULL);

        void setTargetCommand(const QString& cmd);
        void setTargetName(const QString& name);
        void setTargetDir(const QString& dir);

        QString targetCommand() const;
        QString targetName() const;
        QString targetDir() const;

    private Q_SLOTS:
        void checkTargetName(const QString& name);

    private:
        const QStringList& m_existingTargets;
        QString m_initialTargetName;
        QLabel      * m_nameLabel;
        KLineEdit   * m_name;
        QLabel      * m_cmdLabel;
        KLineEdit   * m_cmd;
        QLabel      * m_dirLabel;
        KLineEdit   * m_buildDir;
        QToolButton * m_browseButton;
};

#endif

// kate: space-indent on; indent-width 4; replace-tabs on;
