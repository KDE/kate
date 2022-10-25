/*
 *
 *  SPDX-FileCopyrightText: 2022 Christoph Cullmann <cullmann@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "katepythonplugin.h"
#include "pythonutils.h"

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON(KatePythonPluginFactory, "katepythonplugin.json", registerPlugin<KatePythonPlugin>();)

KatePythonPlugin::KatePythonPlugin(QObject *application, const QList<QVariant> &)
    : KTextEditor::Plugin(application)
{
    qDebug() << "init res; " << PythonUtils::init();
    PythonUtils::runScript({QStringLiteral("print('hello world')")});
}

QObject *KatePythonPlugin::createView(KTextEditor::MainWindow *)
{
    return nullptr;
}

#include "katepythonplugin.moc"
