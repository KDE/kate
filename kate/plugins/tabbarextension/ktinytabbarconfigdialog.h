/***************************************************************************
                           ktinytabbarconfigdialog.h
                           -------------------
    begin                : 2005-06-19
    copyright            : (C) 2005 by Dominik Haumann
    email                : dhdev@gmx.de
 ***************************************************************************/

/***************************************************************************
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ***************************************************************************/

#ifndef KTINYTABBARCONFIGDIALOG_H
#define KTINYTABBARCONFIGDIALOG_H

#include <kdialog.h>

class KTinyTabBar;
class KTinyTabBarConfigPage;

/**
 * The class @p KTinyTabBarConfigDialog provides a configuration dialog
 * for the @p KTinyTabBar widget. It is only a wrapper dialog for
 * @p KTinyTabBarConfigPage. It's mainly for private usage. If the return code
 * is @p KDialog::Accepted an option changed for sure.
 *
 * @author Dominik Haumann
 */
class KTinyTabBarConfigDialog : public KDialog
{
    Q_OBJECT
public:
    explicit KTinyTabBarConfigDialog( const KTinyTabBar* tabbar, QWidget *parent = 0 );
    ~KTinyTabBarConfigDialog();

    KTinyTabBarConfigPage* configPage();

protected slots:
    void configChanged();

private:
    KTinyTabBarConfigPage* m_configPage;
};

#endif

// kate: space-indent on; tab-width 4; replace-tabs off; eol unix;
