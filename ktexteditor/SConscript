#! /usr/bin/env python
#generated from Makefile.am by am2bksys.py
Import('env')

ktexteditor_sources = """
attribute.cpp cursor.cpp cursorfeedback.cpp editorchooser.cpp
editorchooser_ui.ui ktexteditor.cpp range.cpp rangefeedback.cpp
smartcursor.cpp smartrange.cpp templateinterface.cpp
"""

ktexteditorkabcbridge_sources = """
ktexteditorkabcbridge.cpp
"""

ktexteditorinclude_headers = """
attribute.h codecompletioninterface.h commandinterface.h configpage.h
cursor.h cursorfeedback.h document.h editor.h
editorchooser.h factory.h highlightinginterface.h markinterface.h
modificationinterface.h plugin.h range.h rangefeedback.h
searchinterface.h sessionconfiginterface.h smartcursor.h smartinterface.h
smartrange.h templateinterface.h texthintinterface.h variableinterface.h
view.h
"""

noinst_headers = """
attribute_p.h
"""

includes = '#/kio #/interfaces #/interfaces #/kabc #/kabc '

obj = env.kdeobj('shlib')
#obj.it_is_a_kdelib()
obj.target = 'ktexteditor'
obj.source = ktexteditor_sources
obj.includes = includes
obj.libpaths = '##/interfaces/kdocument '
obj.libs = 'kdocument '
obj.uselib = 'QT QTCORE QTGUI QT3SUPPORT KDE4 KPARTS '
obj.execute()

obj = env.kdeobj('module')
obj.target = 'ktexteditorkabcbridge'
obj.source = ktexteditorkabcbridge_sources
obj.includes = includes
obj.libpaths = '##/kabc '
obj.libs = 'kabc '
obj.uselib = 'QT QTCORE QTGUI QT3SUPPORT KDE4 KDECORE '
obj.execute()

env.bksys_insttype('KDEDATA', 'kcm_componentchooser', 'kcm_ktexteditor.desktop')

env.bksys_insttype('KDESERVTYPES', '', 'ktexteditor.desktop ktexteditorplugin.desktop')
env.bksys_insttype('KDEINCLUDE', 'ktexteditor', ktexteditorinclude_headers)


"""Unhandled Defines 
ktexteditorincludedir = $(includedir)/ktexteditor
DOXYGEN_REFERENCES = kdecore kdeui kparts
"""

"""Unhandled Targets 
templateinterface.lo = $(top_builddir)/kabc/addressee.h
ktexteditorkabcbridge.lo = $(top_builddir)/kabc/addressee.h
"""

