// SPDX-FileCopyrightText: 2023 Kåre Särs <kare.sars@iki.fi>
//
// SPDX-License-Identifier: LGPL-2.0-or-later                            *
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
// You should have received a copy of the GNU General Public
// License along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "buildconfig.h"
#include "plugin_katebuild.h"
#include "ui_buildconfigwidget.h"

#include <QDebug>
#include <QMenu>

/******************************************************************/
KateBuildConfigPage::KateBuildConfigPage(KateBuildPlugin *plugin, QWidget *parent)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    ui = new Ui::BuildConfigWidget();
    ui->setupUi(this);
    ui->tabWidget->setDocumentMode(true);

    reset();

    for (const auto *item : {ui->useDiagnosticsCB, ui->autoSwitchToOutput, ui->u_showProgressCB})
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
        connect(item, &QCheckBox::stateChanged, this, &KateBuildConfigPage::changed);
#else
        connect(item, &QCheckBox::checkStateChanged, this, &KateBuildConfigPage::changed);
#endif

    connect(ui->allowedAndBlockedCommands, &QListWidget::itemChanged, this, &KateBuildConfigPage::changed);

    // own context menu to delete entries
    ui->allowedAndBlockedCommands->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->allowedAndBlockedCommands, &QWidget::customContextMenuRequested, this, &KateBuildConfigPage::showContextMenuAllowedBlocked);
}

/******************************************************************/
QString KateBuildConfigPage::name() const
{
    return i18n("Build & Run");
}

/******************************************************************/
QString KateBuildConfigPage::fullName() const
{
    return i18n("Build & Run Settings");
}

/******************************************************************/
QIcon KateBuildConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("run-build-clean"));
}

/******************************************************************/
void KateBuildConfigPage::apply()
{
    m_plugin->m_addDiagnostics = ui->useDiagnosticsCB->isChecked();
    m_plugin->m_autoSwitchToOutput = ui->autoSwitchToOutput->isChecked();
    m_plugin->m_showBuildProgress = ui->u_showProgressCB->isChecked();

    m_plugin->m_commandLineToAllowedState.clear();
    for (int i = 0; i < ui->allowedAndBlockedCommands->count(); ++i) {
        const auto item = ui->allowedAndBlockedCommands->item(i);
        m_plugin->m_commandLineToAllowedState.emplace(item->text(), item->checkState() == Qt::Checked);
    }

    m_plugin->writeConfig();
}

/******************************************************************/
void KateBuildConfigPage::reset()
{
    ui->useDiagnosticsCB->setChecked(m_plugin->m_addDiagnostics);
    ui->autoSwitchToOutput->setChecked(m_plugin->m_autoSwitchToOutput);
    ui->u_showProgressCB->setChecked(m_plugin->m_showBuildProgress);

    ui->allowedAndBlockedCommands->clear();
    for (const auto &[command, isCommandAllowed] : m_plugin->m_commandLineToAllowedState) {
        auto item = new QListWidgetItem(command, ui->allowedAndBlockedCommands);
        item->setCheckState(isCommandAllowed ? Qt::Checked : Qt::Unchecked);
    }
}

/******************************************************************/
void KateBuildConfigPage::defaults()
{
    ui->useDiagnosticsCB->setCheckState(Qt::CheckState::Checked);
    ui->autoSwitchToOutput->setCheckState(Qt::CheckState::Checked);
    ui->u_showProgressCB->setCheckState(Qt::CheckState::Checked);
    ui->allowedAndBlockedCommands->clear();
}

void KateBuildConfigPage::showContextMenuAllowedBlocked(const QPoint &pos)
{
    // allow deletion of stuff
    QMenu myMenu(this);

    auto currentDelete = myMenu.addAction(i18n("Delete selected entries"), this, [this]() {
        qDeleteAll(ui->allowedAndBlockedCommands->selectedItems());
    });
    currentDelete->setEnabled(!ui->allowedAndBlockedCommands->selectedItems().isEmpty());

    auto allDelete = myMenu.addAction(i18n("Delete all entries"), this, [this]() {
        ui->allowedAndBlockedCommands->clear();
    });
    allDelete->setEnabled(ui->allowedAndBlockedCommands->count() > 0);

    myMenu.exec(ui->allowedAndBlockedCommands->mapToGlobal(pos));
}
