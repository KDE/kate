/*
    SPDX-FileCopyrightText: 2022 Waqar Ahmed <waqar.17a@gmail.com>
    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "diffeditor.h"
#include <QPlainTextEdit>
#include <QWidget>

#include <KTextEditor/Document>

namespace KSyntaxHighlighting
{
class SyntaxHighlighter;
}

class DiffWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DiffWidget(QWidget *parent = nullptr);

    void diffDocs(KTextEditor::Document *l, KTextEditor::Document *r);

    void openDiff(const QByteArray &diff);

    Q_INVOKABLE bool shouldClose()
    {
        return true;
    }

private:
    void clearData();
    void handleStyleChange(int);
    void onTextReceived(const QByteArray &text);
    void onError(const QByteArray &error, int code);
    void parseAndShowDiff(const QByteArray &raw);
    void parseAndShowDiffUnified(const QByteArray &raw);

    class DiffEditor *m_left;
    class DiffEditor *m_right;
    KSyntaxHighlighting::SyntaxHighlighter *leftHl;
    KSyntaxHighlighting::SyntaxHighlighter *rightHl;
    DiffStyle m_style = SideBySide;
    QByteArray m_rawDiff; // Raw diff saved as is
};
