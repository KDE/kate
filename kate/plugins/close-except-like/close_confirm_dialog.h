/**
 * \file
 *
 * \brief Class \c kate::CloseConfirmDialog (interface)
 *
 * \date Sun Jun 24 16:29:13 MSK 2012 -- Initial design
 */
/*
 * KateCloseExceptPlugin is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * KateCloseExceptPlugin is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __SRC__CLOSE_CONFIRM_DIALOG_H__
# define __SRC__CLOSE_CONFIRM_DIALOG_H__

// Project specific includes

// Standard includes
# include <KDialog>
# include <KTextEditor/Document>
# include <KToggleAction>
# include <QtCore/QList>
# include <QtGui/QTreeWidget>
# include <QtGui/QTreeWidgetItem>
# include <QtGui/QCheckBox>

namespace kate {

/**
 * \brief [Type brief class description here]
 *
 * [More detailed description here]
 *
 */
class CloseConfirmDialog : public KDialog
{
    Q_OBJECT
public:
    /// Default constructor
    explicit CloseConfirmDialog(QList<KTextEditor::Document*>&, KToggleAction*, QWidget* const = 0);
    ~CloseConfirmDialog();

private Q_SLOTS:
    void updateDocsList();

private:
    QList<KTextEditor::Document*>& m_docs;
    QTreeWidget* m_docs_tree;
    QCheckBox* m_dont_ask_again;
};

}                                                           // namespace kate
#endif                                                      // __SRC__CLOSE_CONFIRM_DIALOG_H__
