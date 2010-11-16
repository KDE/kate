/****************************************************************************
 **    - This file is part of the Artistic Comment KTextEditor-Plugin -    **
 **  - Copyright 2009-2010 Jonathan Schmidt-Dominé <devel@the-user.org> -  **
 **                                  ----                                  **
 **   - This program is free software; you can redistribute it and/or -    **
 **    - modify it under the terms of the GNU Library General Public -     **
 **    - License version 3, or (at your option) any later version, as -    **
 ** - published by the Free Software Foundation.                         - **
 **                                  ----                                  **
 **  - This library is distributed in the hope that it will be useful, -   **
 **   - but WITHOUT ANY WARRANTY; without even the implied warranty of -   **
 ** - MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU -  **
 **          - Library General Public License for more details. -          **
 **                                  ----                                  **
 ** - You should have received a copy of the GNU Library General Public -  **
 ** - License along with the kdelibs library; see the file COPYING.LIB. -  **
 **  - If not, write to the Free Software Foundation, Inc., 51 Franklin -  **
 **      - Street, Fifth Floor, Boston, MA 02110-1301, USA. or see -       **
 **                  - <http://www.gnu.org/licenses/>. -                   **
 ***************************************************************************/

#include "acommentplugin.h"
#include "acommentview.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocale>
#include <KAction>
#include <KActionCollection>
#include <KActionMenu>
#include <KMenu>
#include <KConfigGroup>
#include <KComponentData>

#include <QMap>

#include "artisticcomment.h"

K_PLUGIN_FACTORY(ACommentPluginFactory, registerPlugin<ACommentPlugin>("ktexteditor_acomment");)
K_EXPORT_PLUGIN(ACommentPluginFactory("ktexteditor_acomment", "ktexteditor_plugins"))

ACommentPlugin::ACommentPlugin(QObject *parent, const QVariantList &args)
        : KTextEditor::Plugin(parent)
{
    Q_UNUSED(args);
    ArtisticComment::readConfig();
}

ACommentPlugin::~ACommentPlugin()
{
}

void ACommentPlugin::addView(KTextEditor::View *view)
{
    ACommentView *nview = new ACommentView(view);
    m_views.append(nview);
}

void ACommentPlugin::removeView(KTextEditor::View *view)
{
    for (int z = 0; z < m_views.size(); z++)
    {
        if (m_views.at(z)->parentClient() == view)
        {
            ACommentView *nview = m_views.at(z);
            m_views.removeAll(nview);
            delete nview;
        }
    }
}

ACommentView::ACommentView(KTextEditor::View *view)
        : QObject(view)
        , KXMLGUIClient(view)
        , m_view(view)
        , m_dialog(new KDialog(view))
{
    setComponentData(ACommentPluginFactory::componentData());

    KActionMenu *action = new KActionMenu(i18n("Insert Artistic Comment"), this);
    m_menu = action->menu();
    QStringList groups = ArtisticComment::styles();
    foreach(QString entry, groups)
        m_menu->addAction(entry);
    actionCollection()->addAction("tools_acomment", action);
    
    connect(m_menu, SIGNAL(triggered(QAction*)), this, SLOT(insertAComment(QAction*)));
    
    KAction *action2 = new KAction(i18n("Configure Artistic Comment styles"), this);
    actionCollection()->addAction("tools_acomment_config", action2);
    
    connect(action2, SIGNAL(triggered()), m_dialog, SLOT(open()));

    setXMLFile("acommentui.rc");
    
    m_ui.setupUi(m_dialog->mainWidget());
    m_dialog->setWindowTitle(i18n("Artistic Comment - Styles"));
    m_dialog->setButtons(KDialog::Ok | KDialog::Apply | KDialog::Cancel);
    m_dialog->setToolTip(i18n("Manage styles for Artistic Comment"));
    m_ui.name->addItems(ArtisticComment::styles());
    m_ui.name->setEditText("");
    static_cast<KLineEdit*>(m_ui.name->lineEdit())->setClickMessage(i18n("Name of my fantastic style"));
    
    // Default-Alignment: Left
    m_ui.textBegin->setEnabled(false);
    m_ui.lfill->setEnabled(false);
    
    connect(m_ui.name, SIGNAL(currentIndexChanged(QString)), this, SLOT(loadStyle(QString)));
    connect(m_ui.type, SIGNAL(activated(int)), this, SLOT(disableOptions(int)));
    connect(m_dialog, SIGNAL(applyClicked()), this, SLOT(changeEntry()));
    connect(m_dialog, SIGNAL(accepted()), this, SLOT(changeEntry()));
}

ACommentView::~ACommentView()
{
}

void ACommentView::insertAComment(QAction *action)
{
    /// @TODO Insert a shortcut here?
    if(m_view->selection())
        m_view->document()->replaceText(m_view->selectionRange(), ArtisticComment::decorate(action->iconText(), m_view->selectionText()), false);
    else
        m_view->document()->insertText(m_view->cursorPosition(), ArtisticComment::decorate(action->iconText(), ""), false);
}

void ACommentView::loadStyle(QString style)
{
    ArtisticComment& ac(ArtisticComment::style(style));
    m_ui.begin->setText(ac.begin);
    m_ui.end->setText(ac.end);
    m_ui.lineBegin->setText(ac.lineBegin);
    m_ui.lineEnd->setText(ac.lineEnd);
    m_ui.textBegin->setText(ac.textBegin);
    m_ui.textEnd->setText(ac.textEnd);
    m_ui.lfill->setText(ac.lfill);
    m_ui.rfill->setText(ac.rfill);
    m_ui.minfill->setValue(ac.minfill);
    m_ui.realWidth->setValue(ac.realWidth);
    if(m_ui.type->currentIndex() != (int)ac.type)
    {
        m_ui.type->setCurrentIndex((int)ac.type);
        disableOptions((int)ac.type);
    }
}

void ACommentView::disableOptions(int type)
{
    switch(type)
    {
        case ArtisticComment::Left:
            m_ui.textBegin->setEnabled(false);
            m_ui.lfill->setEnabled(false);
            m_ui.textEnd->setEnabled(true);
            m_ui.rfill->setEnabled(true);
            m_ui.minfill->setEnabled(true);
            break;
        case ArtisticComment::Center:
            m_ui.textBegin->setEnabled(true);
            m_ui.lfill->setEnabled(true);
            m_ui.textEnd->setEnabled(true);
            m_ui.rfill->setEnabled(true);
            m_ui.minfill->setEnabled(true);
            break;
        case ArtisticComment::Right:
            m_ui.textBegin->setEnabled(true);
            m_ui.lfill->setEnabled(true);
            m_ui.textEnd->setEnabled(false);
            m_ui.rfill->setEnabled(false);
            m_ui.minfill->setEnabled(true);
            break;
        case ArtisticComment::LeftNoFill:
            m_ui.textBegin->setEnabled(false);
            m_ui.lfill->setEnabled(false);
            m_ui.textEnd->setEnabled(false);
            m_ui.rfill->setEnabled(false);
            m_ui.minfill->setEnabled(false);
            break;
    }
}

void ACommentView::changeEntry()
{
    QString style = m_ui.name->currentText();
    if(!m_ui.name->contains(style))
    {
        m_ui.name->addItem(style);
        m_menu->addAction(style);
    }
    ArtisticComment& ac(ArtisticComment::style(style));
    ac.begin = m_ui.begin->text();
    ac.end = m_ui.end->text();
    ac.textBegin = m_ui.textBegin->text();
    ac.textEnd = m_ui.textEnd->text();
    ac.lineBegin = m_ui.lineBegin->text();
    ac.lineEnd = m_ui.lineEnd->text();
    {
        QString tmp = m_ui.lfill->text();
        if(tmp.size() != 0)
            ac.lfill = tmp[0];
        else
            ac.lfill = ' ';
        tmp = m_ui.rfill->text();
        if(tmp.size() != 0)
            ac.rfill = tmp[0];
        else
            ac.rfill = ' ';
    }
    ac.truncate = true;
    ac.type = (ArtisticComment::type_t)m_ui.type->currentIndex();
    ac.minfill = m_ui.minfill->value();
    ac.realWidth = m_ui.realWidth->value();
    ArtisticComment::writeConfig();
}

#include "acommentview.moc"
