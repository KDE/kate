/***************************************************************************
                           ktinytabbarconfigpage.h
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

#ifndef KTINYTABBARCONFIGPAGE_H
#define KTINYTABBARCONFIGPAGE_H

#include <QWidget>
#include <QColor>
#include "ktinytabbar.h"

class KColorButton;
class KTinyTabButton;
class QCheckBox;
class QLabel;
class QRadioButton;
class QSlider;
class QSpinBox;

/**
 * The class @p KTinyTabBarConfigPage provides a config page for
 * @p KTinyTabBar. It provides an interface to set the number of rows,
 * minimum and maximum tab width, and the tab height.
 *
 * @author Dominik Haumann
 */
class KTinyTabBarConfigPage : public QWidget
{
    Q_OBJECT
public:
    KTinyTabBarConfigPage( QWidget *parent = 0 );

    ~KTinyTabBarConfigPage();

    void initialize( QWidget* parent );


    bool locationTop() const;
    void setLocationTop( bool value );


    int minimumTabWidth() const;
    void setMinimumTabWidth( int value );

    int maximumTabWidth() const;
    void setMaximumTabWidth( int value );

    int fixedTabHeight() const;
    void setFixedTabHeight( int value );


    int numberOfRows() const;
    void setNumberOfRows( int value );

    bool followCurrentTab() const;
    void setFollowCurrentTab( bool value );

    void setTabSortType( KTinyTabBar::SortType type );
    KTinyTabBar::SortType tabSortType() const;

    void setTabButtonStyle( KTinyTabBar::ButtonStyle style );
    KTinyTabBar::ButtonStyle tabButtonStyle() const;

    void setPlainColorActivated( const QColor& color );
    void setPlainColorHovered( const QColor& color );
    void setPlainColorPressed( const QColor& color );
    QColor plainColorActivated() const;
    QColor plainColorHovered() const;
    QColor plainColorPressed() const;

    void setHighlightActiveTab( bool value );
    bool highlightActiveTab() const;

    void setActiveTabColor( const QColor& color );
    QColor activeTabColor() const;

    void setHighlightPreviousTab( bool value );
    bool highlightPreviousTab() const;

    void setPreviousTabColor( const QColor& color );
    QColor previousTabColor() const;

    void setHighlightOpacity( int value );
    int highlightOpacity() const;

    void setHighlightModifiedTabs( bool modified );
    bool highlightModifiedTabs() const;

    void setModifiedTabsColor( const QColor& color );
    QColor modifiedTabsColor() const;


signals:
    void changed();
    void removeHighlightMarks();

protected slots:
    void locationTopChanged( bool top );
    void minimumTabWidthChanged( int value );
    void maximumTabWidthChanged( int value );
    void fixedTabHeightChanged( int value );
    void numberOfRowsChanged( int value );
    void followCurrentTabChanged( bool follow );
    void sortTypeChanged( bool checked );
    void buttonStyleChanged( bool checked );
    void plainColorActivatedChanged( const QColor &newColor );
    void plainColorHoveredChanged( const QColor &newColor );
    void plainColorPressedChanged( const QColor &newColor );
    void highlightActiveTabChanged( bool highlight );
    void highlightPreviousTabChanged( bool highlight );
    void activeTabColorChanged( const QColor& newColor );
    void previousTabColorChanged( const QColor& newColor );
    void highlightOpacityChanged( int value );
    void highlightModifiedTabsChanged( bool highlight );
    void modifiedTabsColorChanged( const QColor& newColor );

    void buttonActivated( KTinyTabButton* );

protected:
    void setupGUI( QWidget* parent );
    void setupDefaults();
    void setupConnections();

private:
    KTinyTabButton* m_previewMinimum;
    KTinyTabButton* m_previewMaximum;

    // location
    QRadioButton* m_locationTop;
    QRadioButton* m_locationBottom;

    // rows
    QSpinBox* m_rowsRows;
    QCheckBox* m_rowsFollowCurrentTab;

    // sort by
    QRadioButton* m_sortByOpening;
    QRadioButton* m_sortByName;
    QRadioButton* m_sortByURL;
    QRadioButton* m_sortByExtension;

    // tab sizes
    QSpinBox* m_tabSizesMinimumWidth;
    QSpinBox* m_tabSizesMaximumWidth;
    QSpinBox* m_tabSizesFixedHeight;

    // button style
    QRadioButton* m_buttonStylePush;
    QRadioButton* m_buttonStyleFlat;
    QRadioButton* m_buttonStylePlain;

    // plain button colors
    KColorButton* m_plainButtonColorsActivated;
    KColorButton* m_plainButtonColorsHovered;
    KColorButton* m_plainButtonColorsPressed;

    // tab highlighting
    QCheckBox* m_highlightingModifiedTabs;
    KColorButton* m_highlightingModifiedTabsColor;
    QCheckBox* m_highlightingActiveTab;
    KColorButton* m_highlightingActiveTabColor;
    QCheckBox* m_highlightingPreviousTab;
    KColorButton* m_highlightingPreviousTabColor;
    QLabel* m_highlightingOpacityLabel;
    QSlider* m_highlightingOpacity;
};

#endif

// kate: space-indent on; indent-width 2; tab-width 4; replace-tabs off; eol unix;
