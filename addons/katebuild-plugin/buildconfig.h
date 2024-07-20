#pragma once
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

#include <KTextEditor/ConfigPage>

class QCheckBox;

namespace Ui
{
class BuildConfigWidget;
}

class KateBuildPlugin;

class KateBuildConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT
public:
    explicit KateBuildConfigPage(KateBuildPlugin *plugin, QWidget *parent = nullptr);
    ~KateBuildConfigPage() override
    {
    }

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

    void apply() override;
    void reset() override;
    void defaults() override;

public Q_SLOTS:
    void showContextMenuAllowedBlocked(const QPoint &pos);

private:
    Ui::BuildConfigWidget *ui;
    KateBuildPlugin *m_plugin;
};
