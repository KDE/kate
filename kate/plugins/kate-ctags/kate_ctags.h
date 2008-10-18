#ifndef KATE_CTAGS_H
#define KATE_CTAGS_H
/* Description : Kate CTags plugin
 *
 * Copyright (C) 2008 by Kare Sars <kare dot sars at iki dot fi>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

#include <kate/plugin.h>
#include <kate/application.h>
#include <kate/documentmanager.h>
#include <kate/mainwindow.h>

#include <kprocess.h>
#include <kactionmenu.h>

#include <QStack>
#include <QTimer>

#include "tags.h"

#include "ui_kate_ctags.h"

typedef struct
{
    KUrl url;
    KTextEditor::Cursor cursor;
} TagJump;

class KAction;

//******************************************************************/
class KateCTagsPlugin : public Kate::Plugin
{
    Q_OBJECT

    public:
        explicit KateCTagsPlugin(QObject* parent = 0, const QStringList& = QStringList());
        virtual ~KateCTagsPlugin() {}

        Kate::PluginView *createView(Kate::MainWindow *mainWindow);

};

/******************************************************************/
class KateCTagsView : public Kate::PluginView, public KXMLGUIClient
{
    Q_OBJECT

    public:
        KateCTagsView(Kate::MainWindow *mw);
        ~KateCTagsView();

        // overwritten: read and write session config
        void readSessionConfig (KConfigBase* config, const QString& groupPrefix);
        void writeSessionConfig (KConfigBase* config, const QString& groupPrefix);

        QWidget *toolView() const;

    public slots:
        void gotoDefinition();
        void gotoDeclaration();
        void lookupTag();
        void stepBack();
        void editLookUp();
        void startEditTmr();
        void tagHitClicked(QTreeWidgetItem *);

        void setTagsFile(const QString &fname);
        void selectTagFile();
        void startTagFileTmr();
        void setTagsFile();

        void aboutToShow();

        void newTagsDB();
        void updateDB();
        void generateTagsDB(const QString &file);
        void generateDone();

    private:
        QString currentWord();

        bool ctagsDBExists();
        void clearInput();
        void displayHits(const Tags::TagList &list);

        void gotoTagForTypes(const QString &tag, QStringList const &types);
        void jumpToTag(const QString &file, const QString &pattern, const QString &word);


        Kate::MainWindow *m_mWin;
        QWidget          *m_toolView;
        Ui::kateCtags     m_ctagsUi;

        KProcess          m_proc;

        QPointer<KActionMenu> m_menu;
        QAction              *m_gotoDef;
        QAction              *m_gotoDec;
        QAction              *m_lookup;

        QTimer               m_dbTimer;
        QTimer               m_editTimer;
        QStack<TagJump>      m_jumpStack;

};


#endif

