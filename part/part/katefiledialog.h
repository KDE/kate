/***************************************************************************
                          katefiledialog.h  -  description
                             -------------------
    begin                : Mon Feb 5 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KATEFILEDIALOG_H_
#define _KATEFILEDIALOG_H_

#include "kateglobal.h"

#include <kfiledialog.h>
#include <qtextcodec.h>
#include <kurl.h>

class KateFileDialogData
{
  public:
	  KateFileDialogData () { ; }
		~KateFileDialogData () { ; }

    KURL::List urls;
		KURL url;
	  QString encoding;
};

class KateFileDialog : public KFileDialog
{
  Q_OBJECT

  public:
	  KateFileDialog (const QString& startDir = QString::null,
		                const QString& encoding = QString::fromLatin1(QTextCodec::codecForLocale()->name()),
			              QWidget *parent= 0,
			              const QString& caption = QString::null, int type = KateFileDialog::openDialog);

    ~KateFileDialog ();

	  virtual KateFileDialogData exec ();

  protected slots:
    virtual void slotApply();

	public:
	  enum KateFileDialogType
		{
      openDialog,
			saveDialog
		};
		
	private:
	  class KComboBox *encoding;
};

#endif
