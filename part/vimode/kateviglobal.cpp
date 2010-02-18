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

#include "kateviglobal.h"
#include "katevikeyparser.h"

#include "kconfiggroup.h"
#include "kdebug.h"
#include <QApplication>
#include <QClipboard>

KateViGlobal::KateViGlobal()
{
  m_numberedRegisters = new QList<QString>;
  m_registers = new QMap<QChar, QString>;
}

KateViGlobal::~KateViGlobal()
{
  delete m_numberedRegisters;
  delete m_registers;
}

void KateViGlobal::writeConfig( KConfigGroup &config ) const
{
  config.writeEntry( "Normal Mode Mapping Keys", getMappings( NormalMode, true ) );
  QStringList l;
  foreach( const QString &s, getMappings( NormalMode ) ) {
    l << KateViKeyParser::getInstance()->decodeKeySequence( getMapping( NormalMode, s ) );
  }
  config.writeEntry( "Normal Mode Mappings", l );
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
}

QString KateViGlobal::getRegisterContent( const QChar &reg ) const
{
  QString regContent;
  QChar _reg = ( reg != '"' ? reg : m_defaultRegister );

  if ( _reg >= '1' && _reg <= '9' ) { // numbered register
    int index = QString( _reg ).toInt()-1;
    if ( m_numberedRegisters->size() > index) {
      regContent = m_numberedRegisters->at( index );
    }
  } else if ( _reg == '+' ) { // system clipboard register
      regContent = QApplication::clipboard()->text( QClipboard::Clipboard );
  } else if ( _reg == '*' ) { // system selection register
      regContent = QApplication::clipboard()->text( QClipboard::Selection );
  } else { // regular, named register
    if ( m_registers->contains( _reg ) ) {
      regContent = m_registers->value( _reg );
    }
  }

  return regContent;
}

void KateViGlobal::addToNumberedRegister( const QString &text )
{
  if ( m_numberedRegisters->size() == 9 ) {
    m_numberedRegisters->removeLast();
  }

  // register 0 is used for the last yank command, so insert at position 1
  m_numberedRegisters->prepend( text );

  kDebug( 13070 ) << "Register 1-9:";
  for ( int i = 0; i < m_numberedRegisters->size(); i++ ) {
      kDebug( 13070 ) << "\t Register " << i+1 << ": " << m_numberedRegisters->at( i );
  }
}

void KateViGlobal::fillRegister( const QChar &reg, const QString &text )
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
    m_registers->insert( reg, text );
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
