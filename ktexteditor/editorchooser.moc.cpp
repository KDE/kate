/****************************************************************************
** KTextEditor::EditorChooser meta object code from reading C++ file 'editorchooser.h'
**
** Created: Thu Jun 6 21:37:13 2002
**      by: The Qt MOC ($Id$)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#undef QT_NO_COMPAT
#include "editorchooser.h"
#include <qmetaobject.h>
#include <qapplication.h>

#include <private/qucomextra_p.h>
#if !defined(Q_MOC_OUTPUT_REVISION) || (Q_MOC_OUTPUT_REVISION != 19)
#error "This file was generated using the moc from 3.0.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

const char *KTextEditor::EditorChooser::className() const
{
    return "KTextEditor::EditorChooser";
}

QMetaObject *KTextEditor::EditorChooser::metaObj = 0;
static QMetaObjectCleanUp cleanUp_KTextEditor__EditorChooser;

#ifndef QT_NO_TRANSLATION
QString KTextEditor::EditorChooser::tr( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "KTextEditor::EditorChooser", s, c, QApplication::DefaultCodec );
    else
	return QString::fromLatin1( s );
}
#ifndef QT_NO_TRANSLATION_UTF8
QString KTextEditor::EditorChooser::trUtf8( const char *s, const char *c )
{
    if ( qApp )
	return qApp->translate( "KTextEditor::EditorChooser", s, c, QApplication::UnicodeUTF8 );
    else
	return QString::fromUtf8( s );
}
#endif // QT_NO_TRANSLATION_UTF8

#endif // QT_NO_TRANSLATION

QMetaObject* KTextEditor::EditorChooser::staticMetaObject()
{
    if ( metaObj )
	return metaObj;
    QMetaObject* parentObject = EditorChooser_UI::staticMetaObject();
    metaObj = QMetaObject::new_metaobject(
	"KTextEditor::EditorChooser", parentObject,
	0, 0,
	0, 0,
#ifndef QT_NO_PROPERTIES
	0, 0,
	0, 0,
#endif // QT_NO_PROPERTIES
	0, 0 );
    cleanUp_KTextEditor__EditorChooser.setMetaObject( metaObj );
    return metaObj;
}

void* KTextEditor::EditorChooser::qt_cast( const char* clname )
{
    if ( !qstrcmp( clname, "KTextEditor::EditorChooser" ) ) return (KTextEditor::EditorChooser*)this;
    return EditorChooser_UI::qt_cast( clname );
}

bool KTextEditor::EditorChooser::qt_invoke( int _id, QUObject* _o )
{
    return EditorChooser_UI::qt_invoke(_id,_o);
}

bool KTextEditor::EditorChooser::qt_emit( int _id, QUObject* _o )
{
    return EditorChooser_UI::qt_emit(_id,_o);
}
#ifndef QT_NO_PROPERTIES

bool KTextEditor::EditorChooser::qt_property( int _id, int _f, QVariant* _v)
{
    return EditorChooser_UI::qt_property( _id, _f, _v);
}
#endif // QT_NO_PROPERTIES
