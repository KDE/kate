/*  SPDX-License-Identifier: MIT

    Copyright (C) 2019 Mark Nauwelaerts <mark.nauwelaerts@gmail.com>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef LSPCLIENTPLUGIN_H
#define LSPCLIENTPLUGIN_H

#include <QMap>
#include <QUrl>
#include <QVariant>

#include <KTextEditor/Plugin>

class LSPClientPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit LSPClientPlugin(QObject *parent = nullptr, const QList<QVariant> & = QList<QVariant>());
    ~LSPClientPlugin() override;

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    int configPages() const override;
    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

    void readConfig();
    void writeConfig() const;

    // path for local setting files, auto-created on load
    const QString m_settingsPath;

    // default config path
    const QUrl m_defaultConfigPath;

    // settings
    bool m_symbolDetails = false;
    bool m_symbolExpand = false;
    bool m_symbolTree = false;
    bool m_symbolSort = false;
    bool m_complDoc = false;
    bool m_refDeclaration = false;
    bool m_diagnostics = false;
    bool m_diagnosticsHighlight = false;
    bool m_diagnosticsMark = false;
    bool m_messages = false;
    int m_messagesAutoSwitch = 0;
    bool m_autoHover = false;
    bool m_onTypeFormatting = false;
    bool m_incrementalSync = false;
    QUrl m_configPath;
    bool m_semanticHighlighting = false;

    // debug mode?
    bool m_debugMode = false;

    // get current config path
    QUrl configPath() const
    {
        return m_configPath.isEmpty() ? m_defaultConfigPath : m_configPath;
    }

private:
Q_SIGNALS:
    // signal settings update
    void update() const;
};

#endif
