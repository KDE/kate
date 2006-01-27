Import('env')

ktexteditor_sources = """
attribute.cpp cursor.cpp cursorfeedback.cpp editorchooser.cpp
editorchooser_ui.ui ktexteditor.cpp range.cpp rangefeedback.cpp
smartcursor.cpp smartrange.cpp codecompletion2.cpp templateinterface.cpp
"""

for i in "configpage.h factory.h editor.h document.h view.h plugin.h".split():
    env.Moc( i )


ktexteditorkabcbridge_sources = """
ktexteditorkabcbridge.cpp
"""

ktexteditorinclude_headers = """
attribute.h codecompletioninterface.h commandinterface.h configpage.h
cursor.h cursorfeedback.h document.h editor.h
editorchooser.h factory.h highlightinginterface.h markinterface.h
modificationinterface.h plugin.h range.h rangefeedback.h
searchinterface.h sessionconfiginterface.h smartcursor.h smartinterface.h
smartrange.h codecompletion2.h templateinterface.h texthintinterface.h variableinterface.h
view.h
"""

noinst_headers = """
attribute_p.h
"""

includes = '../../kio ../../interfaces ../../kabc ../../interfaces/ktexteditor'

obj = env.kdeobj('shlib')
obj.target = 'ktexteditor'
obj.source = ktexteditor_sources
obj.includes = includes
obj.libpaths = '../../kdecore ../../kdeui ../../kparts ../../kio ../kdocument '
obj.libs = 'kdecore kdeui kparts kio kdocument'
obj.uselib = 'QT QTCORE QTGUI QT3SUPPORT KDE4 KPARTS '
if env['WINDOWS']:
	obj.libpaths += '../../win '
	obj.libs += ' kdewin32'
	
obj.execute()

#obj = env.kdeobj('module')
#obj.target = 'ktexteditorkabcbridge'
#obj.source = ktexteditorkabcbridge_sources
#obj.includes = includes
#obj.libpaths = '##/kabc '
#obj.libs = 'kabc '
#obj.uselib = 'QT QTCORE QTGUI QT3SUPPORT KDE4 KDECORE '
#obj.execute()

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

