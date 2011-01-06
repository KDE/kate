//
// Description: Widget that local variables of the gdb inferior
//
// Copyright (c) 2010 Kåre Särs <kare.sars@iki.fi>
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License version 2 as published by the Free Software Foundation.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public License
//  along with this library; see the file COPYING.LIB.  If not, write to
//  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  Boston, MA 02110-1301, USA.

#include "localsview.h"
#include <klocale.h>
#include <kdebug.h>


LocalsView::LocalsView(QWidget *parent)
:   QTreeWidget(parent), m_allAdded(true)
{
    QStringList headers;
    headers << i18n("Symbol");
    headers << i18n("Value");
    setHeaderLabels(headers);
    setAutoScroll(false);
}

LocalsView::~LocalsView()
{
}

void LocalsView::addLocal(const QString &vString)
{
    static QRegExp isValue("(\\S*)\\s=\\s(.*)");
    static QRegExp isStruct("\\{\\S*\\s=\\s.*");
    static QRegExp isStartPartial("\\S*\\s=\\s\\{");

    if (m_allAdded) {
        clear();
        m_allAdded = false;
    }
    
    if (vString.isEmpty()) {
        m_allAdded = true;
        resizeColumnToContents(1);
        return;
    }
    
    if (isStartPartial.exactMatch(vString)) {
        m_local = vString;
        return;
    }
    
    if (!isValue.exactMatch(vString)) {
        if (!m_local.isEmpty()) {
            m_local += vString;
        }
        if (vString != "}") {
            return;
        }
    }

    QStringList symbolVal;
    symbolVal << isValue.cap(1);
    QString val = isValue.cap(2);
    QTreeWidgetItem *item;
    if (val[0] == '{') {
        if (val[1] == '{') {
            item = new QTreeWidgetItem(this, symbolVal);
            addArray(item, val.mid(1, val.size()-2));
        }
        else {
            if (isStruct.exactMatch(val)) {
                item = new QTreeWidgetItem(this, symbolVal);
                addStruct(item, val.mid(1, val.size()-2));
            }
            else {
                symbolVal << val;
                new QTreeWidgetItem(this, symbolVal);
            }
        }
    }
    else {
        symbolVal << val;
        new QTreeWidgetItem(this, symbolVal);
    }
    
    m_local.clear();
}


void LocalsView::addStruct(QTreeWidgetItem *parent, const QString &vString)
{
    static QRegExp isArray("\\{\\S*\\s=\\s.*");
    static QRegExp isStruct("\\S*\\s=\\s.*");
    QTreeWidgetItem *item;
    QStringList symbolVal;
    QString subValue;
    int start = 0;
    int end;
    while (start < vString.size()) {
        // Symbol
        symbolVal.clear();
        end = vString.indexOf(" = ", start);
        symbolVal << vString.mid(start, end-start);
        //kDebug() << symbolVal;
        
        // Value
        start = end + 3;
        end = start+1;
        if (vString[start] == '{') {
            start++;
            int count = 1;
            bool inComment = false;
            // search for the matching }
            while(end < vString.size()) {
                if (!inComment) {
                    if (vString[end] == '"') inComment = true;
                    else if (vString[end] == '}') count--;
                    else if (vString[end] == '{') count++;
                    if (count == 0) break;
                }
                else {
                    if ((vString[end] == '"') && (vString[end-1] != '\\')) {
                        inComment = false;
                    }
                }
                end++;
            }
            subValue = vString.mid(start, end-start);
            if (isArray.exactMatch(subValue)) {
                item = new QTreeWidgetItem(parent, symbolVal);
                addArray(item, subValue);
            }
            else if (isStruct.exactMatch(subValue)) {
                item = new QTreeWidgetItem(parent, symbolVal);
                addStruct(item, subValue);
            }
            else {
                symbolVal << vString.mid(start, end-start);
                new QTreeWidgetItem(parent, symbolVal);
            }
            start = end + 3; // },_
        }
        else {
            // look for the end of the vString
            bool inComment = false;
            while(end < vString.size()) {
                if (!inComment) {
                    if (vString[end] == '"') inComment = true;
                    else if (vString[end] == ',') break;
                }
                else {
                    if ((vString[end] == '"') && (vString[end-1] != '\\')) {
                        inComment = false;
                    }
                }
                end++;
            }
            symbolVal << vString.mid(start, end-start);
            new QTreeWidgetItem(parent, symbolVal);
            start = end + 2; // ,_
        }
    }
}

void LocalsView::addArray(QTreeWidgetItem *parent, const QString &vString)
{
    // getting here we have this kind of string:
    // "{...}" or "{...}, {...}" or ...
    QTreeWidgetItem *item;
    int count = 1;
    bool inComment = false;
    int index = 0;
    int start = 1;
    int end = 1;

    while (end < vString.size()) {
        if (!inComment) {
            if (vString[end] == '"') inComment = true;
            else if (vString[end] == '}') count--;
            else if (vString[end] == '{') count++;
            if (count == 0) {
                QStringList name;
                name << QString("[%1]").arg(index);
                index++;
                item = new QTreeWidgetItem(parent, name);
                addStruct(item, vString.mid(start, end-start));
                end += 4; // "}, {"
                start = end;
                count = 1;
            }
        }
        else {
            if ((vString[end] == '"') && (vString[end-1] != '\\')) {
                inComment = false;
            }
        }
        end++;
    }
}