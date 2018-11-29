/*   Kate search plugin
 *
 * Copyright (C) 2011-2013 by Kåre Särs <kare.sars@iki.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include <QTreeWidgetItem>
#include <QTimer>
#include <ktexteditor/movinginterface.h>
#include <ktexteditor/movingrange.h>
#include <klocalizedstring.h>

ReplaceMatches::ReplaceMatches(QObject *parent) : QObject(parent),
m_manager(nullptr),
m_tree(nullptr),
m_rootIndex(-1)
{
    connect(this, &ReplaceMatches::replaceNextMatch, this, &ReplaceMatches::doReplaceNextMatch, Qt::QueuedConnection);
}

void ReplaceMatches::replaceChecked(QTreeWidget *tree, const QRegularExpression &regexp, const QString &replace)
{
    if (m_manager == nullptr) return;
    if (m_rootIndex != -1) return;

    m_tree = tree;
    m_rootIndex = 0;
    m_regExp = regexp;
    m_replaceText = replace;
    m_cancelReplace = false;
    m_progressTime.restart();
    emit replaceNextMatch();
}

void ReplaceMatches::setDocumentManager(KTextEditor::Application *manager)
{
    m_manager = manager;
}

void ReplaceMatches::cancelReplace()
{
    m_cancelReplace = true;
}

KTextEditor::Document *ReplaceMatches::findNamed(const QString &name)
{
    QList<KTextEditor::Document*> docs = m_manager->documents();

    foreach (KTextEditor::Document* it, docs) {
        if ( it->documentName() == name) {
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
        qDebug() << "not replacing";
        return false;
    }

    // Check that the text has not been modified and still matches + get captures for the replace
    QString matchLines = doc->text(range);
    QRegularExpressionMatch match = regExp.match(matchLines);
    if (match.capturedStart() != 0) {
        qDebug() << matchLines << "Does not match" << regExp.pattern();
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
    int newEndColumn = lastNL == -1 ? range.start().column() + replaceText.length() : replaceText.length() - lastNL-1;

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
    html += QStringLiteral("<i><s>") + item->data(0, ReplaceMatches::MatchRole).toString() + QStringLiteral("</s></i> ");
    html += QStringLiteral("<b>") + replaceText + QStringLiteral("</b>");
    html += item->data(0, ReplaceMatches::PostMatchRole).toString();
    item->setData(0, Qt::DisplayRole, i18n("Line: <b>%1</b>: %2",range.start().line()+1, html));

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
    QVector<KTextEditor::MovingRange*> matches;
    QVector<bool> replaced;
    KTextEditor::MovingInterface* miface = qobject_cast<KTextEditor::MovingInterface*>(doc);

    int i = 0;

    // Only add items after "item"
    for (; i<rootItem->childCount(); i++) {
        if (item == rootItem->child(i)) break;
    }
    for (int j=i; j<rootItem->childCount(); j++) {
        QTreeWidgetItem *tmp = rootItem->child(j);
        int startLine = tmp->data(0, ReplaceMatches::StartLineRole).toInt();
        int startColumn = tmp->data(0, ReplaceMatches::StartColumnRole).toInt();
        int endLine = tmp->data(0, ReplaceMatches::EndLineRole).toInt();
        int endColumn = tmp->data(0, ReplaceMatches::EndColumnRole).toInt();
        KTextEditor::Range range(startLine, startColumn, endLine, endColumn);
        KTextEditor::MovingRange* mr = miface->newMovingRange(range);
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
    for (int j=i+1; j<rootItem->childCount() && !matches.isEmpty(); j++) {
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
    if ((!m_manager) || (m_cancelReplace) || (m_tree->topLevelItemCount() != 1)) {
        m_rootIndex = -1;
        emit replaceDone();
        return;
    }

    // NOTE The document managers signal documentWillBeDeleted() must be connected to
    // cancelReplace(). A closed file could lead to a crash if it is not handled.

    // Open the file
    QTreeWidgetItem *rootItem = m_tree->topLevelItem(0)->child(m_rootIndex);
    if (!rootItem) {
        m_rootIndex = -1;
        emit replaceDone();
        return;
    }

    if (!rootItem->data(0, StartColumnRole).toString().isEmpty()) {
        // this is a search as you type replace
        rootItem = m_tree->topLevelItem(0);
        m_cancelReplace = true; // only one document...
    }

    if (rootItem->checkState(0) == Qt::Unchecked) {
        m_rootIndex++;
        emit replaceNextMatch();
        return;
    }

    KTextEditor::Document *doc;
    QString docUrl = rootItem->data(0, FileUrlRole).toString();
    if (docUrl.isEmpty()) {
        doc = findNamed(rootItem->data(0, FileNameRole).toString());
    }
    else {
        doc = m_manager->findUrl(QUrl::fromUserInput(docUrl));
        if (!doc) {
            doc = m_manager->openUrl(QUrl::fromUserInput(rootItem->data(0, FileUrlRole).toString()));
        }
    }

    if (!doc) {
        m_rootIndex++;
        emit replaceNextMatch();
        return;
    }

    if (m_progressTime.elapsed() > 100) {
        m_progressTime.restart();
        emit replaceStatus(doc->url());
    }

    // Make one transaction for the whole replace to speed up things
    // and get all replacements in one "undo"
    KTextEditor::Document::EditingTransaction transaction(doc);

    // Create a vector of moving ranges for updating the tree-view after replace
    QVector<KTextEditor::MovingRange*> matches;
    QVector<bool> replaced;
    KTextEditor::MovingInterface* miface = qobject_cast<KTextEditor::MovingInterface*>(doc);


    for (int i=0; i<rootItem->childCount(); i++) {
        QTreeWidgetItem *item = rootItem->child(i);
        int startLine = item->data(0, ReplaceMatches::StartLineRole).toInt();
        int startColumn = item->data(0, ReplaceMatches::StartColumnRole).toInt();
        int endLine = item->data(0, ReplaceMatches::EndLineRole).toInt();
        int endColumn = item->data(0, ReplaceMatches::EndColumnRole).toInt();
        KTextEditor::Range range(startLine, startColumn, endLine, endColumn);
        KTextEditor::MovingRange* mr = miface->newMovingRange(range);
        matches.append(mr);
    }

    for (int i=0; i<rootItem->childCount(); i++) {
        QTreeWidgetItem *item = rootItem->child(i);

        if (item->checkState(0) == Qt::Unchecked) {
            replaced << false;
        }
        else {
            replaced << replaceMatch(doc, item, matches[i]->toRange(), m_regExp, m_replaceText);
        }
    }

    // Update the tree-view-items
    for (int i=0; i<replaced.size() && i<matches.size(); i++) {
        QTreeWidgetItem *item = rootItem->child(i);

        if (!replaced[i]) {
            item->setData(0, ReplaceMatches::StartLineRole, matches[i]->start().line());
            item->setData(0, ReplaceMatches::StartColumnRole, matches[i]->start().column());
            item->setData(0, ReplaceMatches::EndLineRole, matches[i]->end().line());
            item->setData(0, ReplaceMatches::EndColumnRole, matches[i]->end().column());
        }
    }

    qDeleteAll(matches);

    m_rootIndex++;
    emit replaceNextMatch();
}
