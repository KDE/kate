/* Description : Kate CTags plugin
 *
 * SPDX-FileCopyrightText: 2008-2011 Kare Sars <kare.sars@iki.fi>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "kate_ctags_plugin.h"

#include "hostprocess.h"

#include <QCheckBox>
#include <QFileDialog>

#include <KConfigGroup>
#include <KSharedConfig>

#include <KLocalizedString>
#include <KMessageBox>
#include <ktexteditor/editor.h>

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(KateCTagsPluginFactory, "katectagsplugin.json", registerPlugin<KateCTagsPlugin>();)

/******************************************************************/
KateCTagsPlugin::KateCTagsPlugin(QObject *parent, const QList<QVariant> &)
    : KTextEditor::Plugin(parent)
{
    // FIXME KF5
    // KGlobal::locale()->insertCatalog("kate-ctags-plugin");
}

/******************************************************************/
QObject *KateCTagsPlugin::createView(KTextEditor::MainWindow *mainWindow)
{
    m_view = new KateCTagsView(this, mainWindow);
    return m_view;
}

/******************************************************************/
KTextEditor::ConfigPage *KateCTagsPlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }
    return new KateCTagsConfigPage(parent);
}

/******************************************************************/
void KateCTagsPlugin::readConfig()
{
}

/******************************************************************/
KateCTagsConfigPage::KateCTagsConfigPage(QWidget *parent)
    : KTextEditor::ConfigPage(parent)
{
    m_confUi.setupUi(this);
    m_confUi.cmdEdit->setText(DEFAULT_CTAGS_CMD);
    connect(m_confUi.cmdEdit, &QLineEdit::textEdited, this, &KateCTagsConfigPage::changed);

    m_confUi.addButton->setToolTip(i18n("Add a directory to index."));
    m_confUi.addButton->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));

    m_confUi.delButton->setToolTip(i18n("Remove a directory."));
    m_confUi.delButton->setIcon(QIcon::fromTheme(QStringLiteral("list-remove")));

    m_confUi.updateDB->setToolTip(i18n("(Re-)generate the common CTags database."));
    m_confUi.updateDB->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));

    connect(m_confUi.updateDB, &QPushButton::clicked, this, &KateCTagsConfigPage::updateGlobalDB);
    connect(m_confUi.addButton, &QPushButton::clicked, this, &KateCTagsConfigPage::addGlobalTagTarget);
    connect(m_confUi.delButton, &QPushButton::clicked, this, &KateCTagsConfigPage::delGlobalTagTarget);

    connect(&m_proc, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), this, &KateCTagsConfigPage::updateDone);
    connect(&m_proc, &QProcess::readyReadStandardError, this, [this]() {
        QString error = QString::fromLocal8Bit(m_proc.readAllStandardError());
        KMessageBox::error(nullptr, error);
    });
    reset();
}

/******************************************************************/
QString KateCTagsConfigPage::name() const
{
    return i18n("CTags");
}

/******************************************************************/
QString KateCTagsConfigPage::fullName() const
{
    return i18n("CTags Settings");
}

/******************************************************************/
QIcon KateCTagsConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("text-x-csrc"));
}

/******************************************************************/
void KateCTagsConfigPage::apply()
{
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("CTags"));
    config.writeEntry("GlobalCommand", m_confUi.cmdEdit->text());

    config.writeEntry("GlobalNumTargets", m_confUi.targetList->count());

    QString nr;
    for (int i = 0; i < m_confUi.targetList->count(); i++) {
        nr = QStringLiteral("%1").arg(i, 3);
        config.writeEntry(QStringLiteral("GlobalTarget_") + nr, m_confUi.targetList->item(i)->text());
    }
    config.sync();
}

/******************************************************************/
void KateCTagsConfigPage::reset()
{
    KConfigGroup config(KSharedConfig::openConfig(), "CTags");
    m_confUi.cmdEdit->setText(config.readEntry(QStringLiteral("GlobalCommand"), DEFAULT_CTAGS_CMD));

    int numEntries = config.readEntry(QStringLiteral("GlobalNumTargets"), 0);
    QString nr;
    QString target;
    for (int i = 0; i < numEntries; i++) {
        nr = QStringLiteral("%1").arg(i, 3);
        target = config.readEntry(QLatin1String("GlobalTarget_") + nr, QString());
        if (!listContains(target)) {
            new QListWidgetItem(target, m_confUi.targetList);
        }
    }
    config.sync();
}

/******************************************************************/
void KateCTagsConfigPage::addGlobalTagTarget()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::Directory);
    // dialog.setMode(KFile::Directory | KFile::Files | KFile::ExistingOnly | KFile::LocalOnly);

    QString dir;
    if (m_confUi.targetList->currentItem()) {
        dir = m_confUi.targetList->currentItem()->text();
    } else if (m_confUi.targetList->item(0)) {
        dir = m_confUi.targetList->item(0)->text();
    }
    dialog.setDirectory(dir);

    // i18n("CTags Database Location"));
    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    QStringList urls = dialog.selectedFiles();

    for (int i = 0; i < urls.size(); i++) {
        if (!listContains(urls[i])) {
            new QListWidgetItem(urls[i], m_confUi.targetList);
            Q_EMIT changed();
        }
    }
}

/******************************************************************/
void KateCTagsConfigPage::delGlobalTagTarget()
{
    delete m_confUi.targetList->currentItem();
    Q_EMIT changed();
}

/******************************************************************/
bool KateCTagsConfigPage::listContains(const QString &target)
{
    for (int i = 0; i < m_confUi.targetList->count(); i++) {
        if (m_confUi.targetList->item(i)->text() == target) {
            return true;
        }
    }
    return false;
}

/******************************************************************/
void KateCTagsConfigPage::updateGlobalDB()
{
    if (m_proc.state() != QProcess::NotRunning) {
        return;
    }

    QStringList targets;
    for (int i = 0; i < m_confUi.targetList->count(); i++) {
        auto target = m_confUi.targetList->item(i)->text();
        if (target.endsWith(QLatin1Char('/')) || target.endsWith(QLatin1Char('\\'))) {
            target = target.left(target.size() - 1);
        }
        targets << target;
    }

    QString file = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + QLatin1String("/katectags");
    QDir().mkpath(file);
    file += QLatin1String("/common_db");

    if (targets.isEmpty()) {
        QFile::remove(file);
        return;
    }

    QStringList arguments = m_proc.splitCommand(m_confUi.cmdEdit->text());
    const QString command = arguments.takeFirst();
    arguments << QStringLiteral("-f") << file << targets;
    startHostProcess(m_proc, command, arguments);

    if (!m_proc.waitForStarted(500)) {
        KMessageBox::error(nullptr, i18n("Failed to run. Error: %1, exit code: %2", m_proc.errorString(), m_proc.exitCode()));
        return;
    }
    m_confUi.updateDB->setDisabled(true);
    QApplication::setOverrideCursor(QCursor(Qt::BusyCursor));
}

/******************************************************************/
void KateCTagsConfigPage::updateDone(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::CrashExit) {
        KMessageBox::error(this, i18n("The CTags executable crashed."));
    } else if (exitCode != 0) {
        KMessageBox::error(this, i18n("The CTags command exited with code %1", exitCode));
    }

    m_confUi.updateDB->setDisabled(false);
    QApplication::restoreOverrideCursor();
}

#include "kate_ctags_plugin.moc"
#include "moc_kate_ctags_plugin.cpp"
