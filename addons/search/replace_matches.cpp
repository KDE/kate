/*   Kate search plugin
 *
 * SPDX-FileCopyrightText: 2011-2013 Kåre Särs <kare.sars@iki.fi>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file called COPYING; if not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include "replace_matches.h"

#include <KLocalizedString>
#include <QTimer>
#include <QTreeWidgetItem>

ReplaceMatches::ReplaceMatches(QObject *parent)
    : QObject(parent)
{
}

void ReplaceMatches::replaceChecked(QTreeWidget *tree, const QRegularExpression &regexp, const QString &replace)
{
    if (m_manager == nullptr)
        return;
    if (m_rootIndex != -1)
        return; // already replacing

    m_tree = tree;
    m_rootIndex = 0;
    m_regExp = regexp;
    m_replaceText = replace;
    m_cancelReplace = false;
    m_terminateReplace = false;
    doReplaceNextMatch();
}

void ReplaceMatches::setDocumentManager(KTextEditor::Application *manager)
{
    m_manager = manager;
}

void ReplaceMatches::cancelReplace()
{
    m_cancelReplace = true;
}

void ReplaceMatches::terminateReplace()
{
    m_cancelReplace = true;
    m_terminateReplace = true;
}

KTextEditor::Document *ReplaceMatches::findNamed(const QString &name)
{
    const QList<KTextEditor::Document *> docs = m_manager->documents();

    for (KTextEditor::Document *it : docs) {
        if (it->documentName() == name) {
            return it;
        }
    }
    return nullptr;
}

bool ReplaceMatches::replaceMatch(KTextEditor::Document *doc, QTreeWidgetItem *item, const KTextEditor::Range &range, const QRegularExpression &regExp, const QString &replaceTxt)
{
    if (!doc || !item) {
        return false;
    }

    // don't replace an already replaced item
    if (item->data(0, ReplaceMatches::ReplacedRole).toBool()) {
        // qDebug() << "not replacing already replaced item";
        return false;
    }

    // Check that the text has not been modified and still matches + get captures for the replace
    QString matchLines = doc->text(range);
    QRegularExpressionMatch match = regExp.match(matchLines);
    if (match.capturedStart() != 0) {
        // qDebug() << matchLines << "Does not match" << regExp.pattern();
        return false;
    }

    // Modify the replace string according to this match
    QString replaceText = replaceTxt;
    replaceText.replace(QLatin1String("\\\\"), QLatin1String("¤Search&Replace¤"));

    // allow captures \0 .. \9
    for (int j = qMin(9, match.lastCapturedIndex()); j >= 0; --j) {
        QString captureLX = QStringLiteral("\\L\\%1").arg(j);
        QString captureUX = QStringLiteral("\\U\\%1").arg(j);
        QString captureX = QStringLiteral("\\%1").arg(j);
        replaceText.replace(captureLX, match.captured(j).toLower());
        replaceText.replace(captureUX, match.captured(j).toUpper());
        replaceText.replace(captureX, match.captured(j));
    }

    // allow captures \{0} .. \{9999999}...
    for (int j = match.lastCapturedIndex(); j >= 0; --j) {
        QString captureLX = QStringLiteral("\\L\\{%1}").arg(j);
        QString captureUX = QStringLiteral("\\U\\{%1}").arg(j);
        QString captureX = QStringLiteral("\\{%1}").arg(j);
        replaceText.replace(captureLX, match.captured(j).toLower());
        replaceText.replace(captureUX, match.captured(j).toUpper());
        replaceText.replace(captureX, match.captured(j));
    }

    replaceText.replace(QLatin1String("\\n"), QLatin1String("\n"));
    replaceText.replace(QLatin1String("\\t"), QLatin1String("\t"));
    replaceText.replace(QLatin1String("¤Search&Replace¤"), QLatin1String("\\"));

    doc->replaceText(range, replaceText);

    int newEndLine = range.start().line() + replaceText.count(QLatin1Char('\n'));
    int lastNL = replaceText.lastIndexOf(QLatin1Char('\n'));
    int newEndColumn = lastNL == -1 ? range.start().column() + replaceText.length() : replaceText.length() - lastNL - 1;

    item->setData(0, ReplaceMatches::ReplacedRole, true);
    item->setData(0, ReplaceMatches::StartLineRole, range.start().line());
    item->setData(0, ReplaceMatches::StartColumnRole, range.start().column());
    item->setData(0, ReplaceMatches::EndLineRole, newEndLine);
    item->setData(0, ReplaceMatches::EndColumnRole, newEndColumn);
    item->setData(0, ReplaceMatches::ReplacedTextRole, replaceText);

    // Convert replace text back to "html"
    replaceText.replace(QLatin1Char('\n'), QStringLiteral("\\n"));
    replaceText.replace(QLatin1Char('\t'), QStringLiteral("\\t"));
    QString html = item->data(0, ReplaceMatches::PreMatchRole).toString();
    html += QLatin1String("<i><s>") + item->data(0, ReplaceMatches::MatchRole).toString() + QLatin1String("</s></i> ");
    html += QLatin1String("<b>") + replaceText + QLatin1String("</b>");
    html += item->data(0, ReplaceMatches::PostMatchRole).toString();
    item->setData(0, Qt::DisplayRole, QStringLiteral("(<b>%1:%2</b>): %3").arg(range.start().line() + 1).arg(range.start().column() + 1).arg(html));

    return true;
}

bool ReplaceMatches::replaceSingleMatch(KTextEditor::Document *doc, QTreeWidgetItem *item, const QRegularExpression &regExp, const QString &replaceTxt)
{
    if (!doc || !item) {
        return false;
    }

    QTreeWidgetItem *rootItem = item->parent();
    if (!rootItem) {
        return false;
    }

    // Create a vector of moving ranges for updating the tree-view after replace
    QVector<KTextEditor::MovingRange *> matches;
    QVector<bool> replaced;
    KTextEditor::MovingInterface *miface = qobject_cast<KTextEditor::MovingInterface *>(doc);

    int i = 0;

    // Only add items after "item"
    for (; i < rootItem->childCount(); i++) {
        if (item == rootItem->child(i))
            break;
    }
    for (int j = i; j < rootItem->childCount(); j++) {
        QTreeWidgetItem *tmp = rootItem->child(j);
        int startLine = tmp->data(0, ReplaceMatches::StartLineRole).toInt();
        int startColumn = tmp->data(0, ReplaceMatches::StartColumnRole).toInt();
        int endLine = tmp->data(0, ReplaceMatches::EndLineRole).toInt();
        int endColumn = tmp->data(0, ReplaceMatches::EndColumnRole).toInt();
        KTextEditor::Range range(startLine, startColumn, endLine, endColumn);
        KTextEditor::MovingRange *mr = miface->newMovingRange(range);
        matches.append(mr);
    }

    if (matches.isEmpty()) {
        return false;
    }

    // The first range in the vector is for this match
    if (!replaceMatch(doc, item, matches[0]->toRange(), regExp, replaceTxt)) {
        return false;
    }

    delete matches.takeFirst();

    // Update the remaining tree-view-items
    for (int j = i + 1; j < rootItem->childCount() && !matches.isEmpty(); j++) {
        QTreeWidgetItem *tmp = rootItem->child(j);
        tmp->setData(0, ReplaceMatches::StartLineRole, matches.first()->start().line());
        tmp->setData(0, ReplaceMatches::StartColumnRole, matches.first()->start().column());
        tmp->setData(0, ReplaceMatches::EndLineRole, matches.first()->end().line());
        tmp->setData(0, ReplaceMatches::EndColumnRole, matches.first()->end().column());
        delete matches.takeFirst();
    }
    qDeleteAll(matches);
    return true;
}

void ReplaceMatches::doReplaceNextMatch()
{
    if (m_terminateReplace) {
        return;
    }

    if (!m_manager || m_tree->topLevelItemCount() != 1) {
        updateTreeViewItems(nullptr);
        m_rootIndex = -1;
        emit replaceDone();
        return;
    }

    // NOTE The document managers signal documentWillBeDeleted() must be connected to
    // cancelReplace(). A closed file could lead to a crash if it is not handled.

    // Open the file
    const auto fileItemIndex = m_rootIndex;
    QTreeWidgetItem *fileItem = m_tree->topLevelItem(0)->child(fileItemIndex);
    if (!fileItem) {
        updateTreeViewItems(nullptr);
        m_rootIndex = -1;
        emit replaceDone();
        return;
    }

    bool isSearchAsYouType = false;
    if (!fileItem->data(0, StartColumnRole).toString().isEmpty()) {
        // this is a search as you type replace
        fileItem = m_tree->topLevelItem(0);
        isSearchAsYouType = true;
    }

    if (m_cancelReplace) {
        updateTreeViewItems(fileItem);
        m_rootIndex = -1;
        emit replaceDone();
        return;
    }

    if (fileItem->checkState(0) == Qt::Unchecked) {
        updateTreeViewItems(fileItem);
        QTimer::singleShot(0, this, &ReplaceMatches::doReplaceNextMatch);
        return;
    }

    KTextEditor::Document *doc;
    QString docUrl = fileItem->data(0, FileUrlRole).toString();
    if (docUrl.isEmpty()) {
        doc = findNamed(fileItem->data(0, FileNameRole).toString());
    } else {
        doc = m_manager->findUrl(QUrl::fromUserInput(docUrl));
        if (!doc) {
            doc = m_manager->openUrl(QUrl::fromUserInput(fileItem->data(0, FileUrlRole).toString()));
        }
    }

    if (!doc) {
        updateTreeViewItems(fileItem);
        QTimer::singleShot(0, this, &ReplaceMatches::doReplaceNextMatch);
        return;
    }

    emit replaceStatus(doc->url(), 0, 0);

    // Create a vector of moving ranges for updating the tree-view after replace
    QVector<KTextEditor::MovingRange *> matches(fileItem->childCount(), nullptr);
    KTextEditor::MovingInterface *miface = qobject_cast<KTextEditor::MovingInterface *>(doc);
    for (int j = 0; j < fileItem->childCount(); ++j) {
        QTreeWidgetItem *item = fileItem->child(j);
        int startLine = item->data(0, ReplaceMatches::StartLineRole).toInt();
        int startColumn = item->data(0, ReplaceMatches::StartColumnRole).toInt();
        int endLine = item->data(0, ReplaceMatches::EndLineRole).toInt();
        int endColumn = item->data(0, ReplaceMatches::EndColumnRole).toInt();
        KTextEditor::Range range(startLine, startColumn, endLine, endColumn);
        matches[j] = miface->newMovingRange(range);
    }

    // Make one transaction for the whole replace to speed up things
    // and get all replacements in one "undo"
    QVector<bool> replaced(fileItem->childCount(), false);
    {
        // now do the replaces
        KTextEditor::Document::EditingTransaction transaction(doc);
        for (int i = 0; i < fileItem->childCount(); ++i) {
            QTreeWidgetItem *item = fileItem->child(i);
            if (item->checkState(0) == Qt::Checked) {
                replaced[i] = replaceMatch(doc, item, matches[i]->toRange(), m_regExp, m_replaceText);
            }
        }
    }

    // hack to avoid update costs, remove item from widget,
    // otherwise the item->setXXX actions will trigger update cascades
    fileItem = m_tree->topLevelItem(0)->takeChild(fileItemIndex);

    // update stuff: important -> only trigger here checked updates
    updateTreeViewItems(fileItem, matches, replaced);

    // insert stuff back after updates
    m_tree->topLevelItem(0)->insertChild(fileItemIndex, fileItem);

    // free our moving ranges
    qDeleteAll(matches);

    if (isSearchAsYouType) {
        m_rootIndex = -1;
        emit replaceDone();
        return;
    }

    QTimer::singleShot(0, this, &ReplaceMatches::doReplaceNextMatch);
}

void ReplaceMatches::updateTreeViewItems(QTreeWidgetItem *fileItem, const QVector<KTextEditor::MovingRange *> &matches, const QVector<bool> &replaced)
{
    // if we have a non-empty matches, we need to update stuff
    // we always pass only matching sized vectors if non-empty!
    if (fileItem && !matches.isEmpty()) {
        qDebug() << fileItem->childCount() << replaced.size() << matches.size();
        Q_ASSERT(fileItem->childCount() == replaced.size());
        Q_ASSERT(matches.size() == replaced.size());
        for (int j = 0; j < fileItem->childCount(); ++j) {
            QTreeWidgetItem *item = fileItem->child(j);
            if (!replaced[j]) {
                item->setData(0, ReplaceMatches::StartLineRole, matches[j]->start().line());
                item->setData(0, ReplaceMatches::StartColumnRole, matches[j]->start().column());
                item->setData(0, ReplaceMatches::EndLineRole, matches[j]->end().line());
                item->setData(0, ReplaceMatches::EndColumnRole, matches[j]->end().column());
                item->setCheckState(0, Qt::PartiallyChecked);
            }
        }
    }

    m_rootIndex++;
}
