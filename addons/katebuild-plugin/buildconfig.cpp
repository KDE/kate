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

#include <QCheckBox>
#include <QDebug>
#include <QVBoxLayout>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

/******************************************************************/
KateBuildConfigPage::KateBuildConfigPage(QWidget *parent)
    : KTextEditor::ConfigPage(parent)
{
    m_useDiagnosticsCB = new QCheckBox(i18n("Add errors and warnings to Diagnostics"), this);
    m_autoSwitchToOutput = new QCheckBox(i18n("Automatically switch to output pane on executing the selected target"), this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_useDiagnosticsCB);
    layout->addWidget(m_autoSwitchToOutput);
    layout->addStretch(1);
    reset();
    for (const auto *item : {m_useDiagnosticsCB, m_autoSwitchToOutput})
#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
        connect(item, &QCheckBox::stateChanged, this, &KateBuildConfigPage::changed);
#else
        connect(item, &QCheckBox::checkStateChanged, this, &KateBuildConfigPage::changed);
#endif
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
    Q_ASSERT(m_useDiagnosticsCB);
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("BuildConfig"));
    config.writeEntry("UseDiagnosticsOutput", m_useDiagnosticsCB->isChecked());
    config.writeEntry("AutoSwitchToOutput", m_autoSwitchToOutput->isChecked());
    config.sync();
    Q_EMIT configChanged();
}

/******************************************************************/
void KateBuildConfigPage::reset()
{
    Q_ASSERT(m_useDiagnosticsCB);
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("BuildConfig"));
    m_useDiagnosticsCB->setChecked(config.readEntry(QStringLiteral("UseDiagnosticsOutput"), true));
    m_autoSwitchToOutput->setChecked(config.readEntry(QStringLiteral("AutoSwitchToOutput"), true));
}

/******************************************************************/
void KateBuildConfigPage::defaults()
{
    Q_ASSERT(m_useDiagnosticsCB);
    m_useDiagnosticsCB->setCheckState(Qt::CheckState::Checked);
    m_autoSwitchToOutput->setCheckState(Qt::CheckState::Checked);
}

#include "moc_buildconfig.cpp"
