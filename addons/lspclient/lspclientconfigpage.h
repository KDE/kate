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

#ifndef LSPCLIENTCONFIGPAGE_H
#define LSPCLIENTCONFIGPAGE_H

#include <KTextEditor/ConfigPage>
#include <KSyntaxHighlighting/Repository>

class LSPClientPlugin;

namespace Ui
{
class LspConfigWidget;
}

class LSPClientConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT

public:
    explicit LSPClientConfigPage(QWidget *parent = nullptr, LSPClientPlugin *plugin = nullptr);
    ~LSPClientConfigPage() override;

    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public Q_SLOTS:
    void apply() override;
    void defaults() override;
    void reset() override;
    void configUrlChanged();

private:
    void readUserConfig(const QString &fileName);

    Ui::LspConfigWidget *ui;
    LSPClientPlugin *m_plugin;
    KSyntaxHighlighting::Repository m_repository;
};

#endif
