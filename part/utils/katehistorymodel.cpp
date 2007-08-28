/* This file is part of the KDE libraries
   Copyright (C) 2007 Sebastian Pipping <webmaster@hartwork.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "katehistorymodel.h"
#include <QStringListModel>



namespace KateHistoryModel {



QStringListModel * getPatternHistoryModel() {
    static QStringListModel * patternHistoryModel = NULL;
    if (patternHistoryModel == NULL) {
        patternHistoryModel = new QStringListModel();
    }
    return patternHistoryModel;
}



QStringListModel * getReplacementHistoryModel() {
    static QStringListModel * replacementHistoryModel = NULL;
    if (replacementHistoryModel == NULL) {
        replacementHistoryModel = new QStringListModel();
    }
    return replacementHistoryModel;
}



}

