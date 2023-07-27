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
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_useDiagnosticsCB);
    layout->addStretch(1);
    reset();
    connect(m_useDiagnosticsCB, &QCheckBox::stateChanged, this, &KateBuildConfigPage::changed);
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
    config.writeEntry("UseDiagnosticsOutput", m_useDiagnosticsCB->checkState() == Qt::CheckState::Checked);
    config.sync();
    Q_EMIT configChanged();
}

/******************************************************************/
void KateBuildConfigPage::reset()
{
    Q_ASSERT(m_useDiagnosticsCB);
    KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("BuildConfig"));
    bool use = config.readEntry(QStringLiteral("UseDiagnosticsOutput"), true);
    m_useDiagnosticsCB->setCheckState(use ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
}

/******************************************************************/
void KateBuildConfigPage::defaults()
{
    Q_ASSERT(m_useDiagnosticsCB);
    m_useDiagnosticsCB->setCheckState(Qt::CheckState::Checked);
}

#include "moc_buildconfig.cpp"
