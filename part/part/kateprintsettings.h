/*
 *  This file is part of the KDE libraries
 *  Copyright (c) 2002 Michael Goffioul <goffioul@imec.be>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License version 2 as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 **/
 
#ifndef KATEPRINTSETTINGS_H
#define KATEPRINTSETTINGS_H

#include <kprintdialogpage.h>

class KPrinter;
class QCheckBox;
class KColorButton;
class QSpinBox;
class QLineEdit;

class KatePrintSettings : public KPrintDialogPage
{
  Q_OBJECT

  public:
    KatePrintSettings(KPrinter *printer, QWidget *parent = 0, const char *name = 0);
    ~KatePrintSettings();

    void getOptions(QMap<QString,QString>& opts, bool incldef = false);
    void setOptions(const QMap<QString,QString>& opts);

  private:
    QCheckBox  *m_usebox, *m_useheader;
    KColorButton  *m_boxcolor, *m_headercolor, *m_fontcolor;
    QLineEdit  *m_headerright, *m_headercenter, *m_headerleft;
    QSpinBox  *m_boxwidth;
    KPrinter  *m_printer;
};

#endif
