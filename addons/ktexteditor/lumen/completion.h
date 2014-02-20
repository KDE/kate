/*
 * Copyright 2014  David Herberth kde@dav1d.de
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
**/

#ifndef LUMEN_COMPLETION_H
#define LUMEN_COMPLETION_H
#include <ktexteditor/codecompletionmodel.h>
#include <ktexteditor/codecompletionmodelcontrollerinterface.h>
#include "dcd.h"

using namespace KTextEditor;

class LumenCompletionModel: public CodeCompletionModel2,
    public KTextEditor::CodeCompletionModelControllerInterface3
{
    Q_OBJECT
    Q_INTERFACES(KTextEditor::CodeCompletionModelControllerInterface3)
public:
    LumenCompletionModel(QObject* parent, DCD* dcd);
    virtual ~LumenCompletionModel();

    virtual bool shouldStartCompletion(View* view, const QString& insertedText, bool userInsertion, const Cursor& position);
    virtual void completionInvoked(View* view, const Range& range, InvocationType invocationType);
    virtual void executeCompletionItem2(Document* document, const Range& word, const QModelIndex& index) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
private:
    DCD* m_dcd;
    DCDCompletion m_data;
};



#endif