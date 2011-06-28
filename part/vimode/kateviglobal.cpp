/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008-2009 Erlend Hamberg <ehamberg@gmail.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include <stdio.h>

#include "kateviglobal.h"
#include "katevikeyparser.h"

#include "kconfiggroup.h"
#include "kdebug.h"
#include <ktexteditor/movingcursor.h>
#include <QApplication>
#include <QClipboard>

KateViGlobal::KateViGlobal()
{
  m_numberedRegisters = new QList<KateViRegister>;
  m_registers = new QMap<QChar, KateViRegister>;
  jump_list = new QList<KateViJump>;
  current_jump = jump_list->begin();
}

KateViGlobal::~KateViGlobal()
{
  delete m_numberedRegisters;
  delete m_registers;
  delete jump_list;
  qDeleteAll( m_marks );
}

void KateViGlobal::writeConfig( KConfigGroup &config ) const
{
  config.writeEntry( "Normal Mode Mapping Keys", getMappings( NormalMode, true ) );
  QStringList l;
  foreach( const QString &s, getMappings( NormalMode ) ) {
    l << KateViKeyParser::getInstance()->decodeKeySequence( getMapping( NormalMode, s ) );
  }
  config.writeEntry( "Normal Mode Mappings", l );

/*
  qDebug() << "\n!!!!!\n!!!!!\n!!!!!\n!!!!!\n!!!!!" << "WRITE CONFIG\n\n\n\!!!\n\n";

  for (int i = 0; i <= jump_list->size(); i++) {
      qDebug() << "\n Write " << i << "\n";
      QStringList l;
      l << QString::number(jump_list->at(i).line) << QString::number(jump_list->at(i).column) <<
           jump_list->at(i).file;
      qDebug() << "L: \n"<< l << "\n";
      config.writeEntry("ViModeJump" + QString::number(i+1),l );
  }*/
}

void KateViGlobal::readConfig( const KConfigGroup &config )
{
    QStringList keys = config.readEntry( "Normal Mode Mapping Keys", QStringList() );
    QStringList mappings = config.readEntry( "Normal Mode Mappings", QStringList() );

    // sanity check
    if ( keys.length() == mappings.length() ) {
      for ( int i = 0; i < keys.length(); i++ ) {
        addMapping( NormalMode, keys.at( i ), mappings.at( i ) );
        kDebug( 13070 ) << "Mapping " << keys.at( i ) << " -> " << mappings.at( i );
      }
    } else {
      kDebug( 13070 ) << "Error when reading mappings from config: number of keys != number of values";
    }

   /* jump_list->clear();
    QString key = "ViModeJump";
    for (int i = 1;config.hasKey(key + QString::number(i)); i++) {
       QStringList jump_line = config.readEntry( key + QString::number(i), QStringList() );

       Q_ASSERT(jump_line.size() == 3);

       int line = jump_line.at(0).toInt();
       int column = jump_line.at(1).toInt();
       QString file = jump_line.at(2);


       KateViJump jump = {line, column, file};

       jump_list->push_back( jump );
       qDebug() << "Line, column, file:" << line << column << file << "\n";

    }
    current_jump = jump_list->begin();

*/
}

KateViRegister KateViGlobal::getRegister( const QChar &reg ) const
{
  KateViRegister regPair;
  QChar _reg = ( reg != '"' ? reg : m_defaultRegister );

  if ( _reg >= '1' && _reg <= '9' ) { // numbered register
    int index = QString( _reg ).toInt()-1;
    if ( m_numberedRegisters->size() > index) {
      regPair = m_numberedRegisters->at( index );
    }
  } else if ( _reg == '+' ) { // system clipboard register
      QString regContent = QApplication::clipboard()->text( QClipboard::Clipboard );
      regPair = KateViRegister( regContent, CharWise );
  } else if ( _reg == '*' ) { // system selection register
      QString regContent = QApplication::clipboard()->text( QClipboard::Selection );
      regPair = KateViRegister( regContent, CharWise );
  } else { // regular, named register
    if ( m_registers->contains( _reg ) ) {
      regPair = m_registers->value( _reg );
    }
  }

  return regPair;
}

QString KateViGlobal::getRegisterContent( const QChar &reg ) const
{
  return getRegister( reg ).first;
}

OperationMode KateViGlobal::getRegisterFlag( const QChar &reg ) const
{
  return getRegister( reg ).second;
}

void KateViGlobal::addToNumberedRegister( const QString &text, OperationMode flag )
{
  if ( m_numberedRegisters->size() == 9 ) {
    m_numberedRegisters->removeLast();
  }

  // register 0 is used for the last yank command, so insert at position 1
  m_numberedRegisters->prepend( KateViRegister(text, flag ) );

  kDebug( 13070 ) << "Register 1-9:";
  for ( int i = 0; i < m_numberedRegisters->size(); i++ ) {
      kDebug( 13070 ) << "\t Register " << i+1 << ": " << m_numberedRegisters->at( i );
  }
}

void KateViGlobal::fillRegister( const QChar &reg, const QString &text, OperationMode flag )
{
  // the specified register is the "black hole register", don't do anything
  if ( reg == '_' ) {
    return;
  }

  if ( reg >= '1' && reg <= '9' ) { // "kill ring" registers
    addToNumberedRegister( text );
  } else if ( reg == '+' ) { // system clipboard register
      QApplication::clipboard()->setText( text,  QClipboard::Clipboard );
  } else if ( reg == '*' ) { // system selection register
      QApplication::clipboard()->setText( text, QClipboard::Selection );
  } else {
    m_registers->insert( reg, KateViRegister(text, flag) );
  }

  kDebug( 13070 ) << "Register " << reg << " set to " << getRegisterContent( reg );

  if ( reg == '0' || reg == '1' || reg == '-' ) {
    m_defaultRegister = reg;
    kDebug( 13070 ) << "Register " << '"' << " set to point to \"" << reg;
  }
}

void KateViGlobal::addMapping( ViMode mode, const QString &from, const QString &to )
{
  if ( !from.isEmpty() ) {
    switch ( mode ) {
    case NormalMode:
      m_normalModeMappings[ KateViKeyParser::getInstance()->encodeKeySequence( from ) ]
        = KateViKeyParser::getInstance()->encodeKeySequence( to );
      break;
    default:
      kDebug( 13070 ) << "Mapping not supported for given mode";
    }
  }
}

const QString KateViGlobal::getMapping( ViMode mode, const QString &from, bool decode ) const
{
    QString ret;
    switch ( mode ) {
    case NormalMode:
      ret = m_normalModeMappings.value( from );
      break;
    default:
      kDebug( 13070 ) << "Mapping not supported for given mode";
    }

    if ( decode ) {
      return KateViKeyParser::getInstance()->decodeKeySequence( ret );
    }
    return ret;
}

const QStringList KateViGlobal::getMappings( ViMode mode, bool decode ) const
{
  QStringList l;
  QHash<QString, QString>::const_iterator i;
  switch (mode ) {
  case NormalMode:
    for (i = m_normalModeMappings.constBegin(); i != m_normalModeMappings.constEnd(); ++i) {
      if ( decode ) {
        l << KateViKeyParser::getInstance()->decodeKeySequence( i.key() );
      } else {
        l << i.key();
      }
    }
    break;
  default:
    kDebug( 13070 ) << "Mapping not supported for given mode";
  }

  return l;
}

void KateViGlobal::clearMappings( ViMode mode )
{
  switch (mode ) {
  case NormalMode:
    m_normalModeMappings.clear();
    break;
  default:
    kDebug( 13070 ) << "Mapping not supported for given mode";
  }
}

void KateViGlobal::addMark( KateDocument* doc, const QChar& mark, const KTextEditor::Cursor& pos )
{
  // delete old cursor if any
  if (KTextEditor::MovingCursor *oldCursor = m_marks.value( mark )) {
    delete oldCursor;
  }

  // create and remember new one
  m_marks.insert( mark, doc->newMovingCursor( pos ) );
}

KTextEditor::Cursor KateViGlobal::getMarkPosition( const QChar& mark ) const
{
  if ( m_marks.contains( mark ) ) {
    KTextEditor::MovingCursor* c = m_marks.value( mark );
    return KTextEditor::Cursor( c->line(), c->column() );
  } else {
    return KTextEditor::Cursor::invalid();
  }
}


 void KateViGlobal::addJump(KTextEditor::Cursor cursor, KateDocument* document) {

    QString thisfile = document->url().path(); // "Thisfile";
    for (QList<KateViJump>::iterator iterator = jump_list->begin();
         iterator != jump_list->end();
         iterator ++){
        if ((*iterator).line == cursor.line() && (*iterator).document == document ){
            jump_list->erase(iterator);
            break;
        }
    }

    KateViJump jump = { cursor.line(), cursor.column(), document };
    jump_list->push_back(jump);
    current_jump = jump_list->end();

    // DEBUG
    PrintJumpList();

 }

KTextEditor::Cursor KateViGlobal::getNextJump(KTextEditor::Cursor cursor, KateDocument* document) {

    QString thisfile = "Thisfile";
    if (current_jump != jump_list->end()) {
        KateViJump jump;
        if (current_jump + 1 != jump_list->end())
            jump = *(++current_jump);
        else
            jump = *(current_jump);

        cursor = KTextEditor::Cursor(jump.line, jump.column);
    }
    // DEBUG
    PrintJumpList();

    return cursor;
}

KTextEditor::Cursor KateViGlobal::getPrevJump(KTextEditor::Cursor cursor, KateDocument* document ) {
    QString thisfile = "Thisfile";
    if (current_jump != jump_list->begin()) {
        KateViJump jump;
            jump = *(--current_jump);
        cursor = KTextEditor::Cursor(jump.line, jump.column);
    }

    // DEBUG
    PrintJumpList();

    return cursor;
}

void KateViGlobal::PrintJumpList(){
    qDebug() << "Jump List\n";
    for (  QList<KateViJump>::iterator iter = jump_list->begin();
         iter != jump_list->end();
         iter++){
            if (iter == current_jump)
                qDebug() << (*iter).line << (*iter).column << (*iter).document->url().path() << "<< Current Jump";
            else
                qDebug() << (*iter).line << (*iter).column << (*iter).document->url().path();
    }
    if (current_jump == jump_list->end())
        qDebug() << "    << Current Jump";

}
