/*  This file is part of the KDE libraries and the Kate part.
 *
 *  Copyright (C) 2008 Erlend Hamberg <ehamberg@gmail.com>
 *  Copyright (C) 2008 Evgeniy Ivanov <powerfox@kde.ru>
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

#include "katevikeyparser.h"
#include <QStringList>
#include <QKeyEvent>

KateViKeyParser* KateViKeyParser::m_instance = NULL;

KateViKeyParser::KateViKeyParser()
{
  m_qt2katevi = new QHash<int, QString>;
  m_katevi2qt = new QHash<QString, int>;
  m_nameToKeyCode = new QHash<QString, int>;
  m_keyCodeToName = new QHash<int, QString>;
  m_altGrPressed = false;

  initKeyTables();
}

KateViKeyParser* KateViKeyParser::self()
{
  if ( m_instance == NULL ) {
    m_instance = new KateViKeyParser();
  }

  return m_instance;
}

void KateViKeyParser::initKeyTables()
{
  m_qt2katevi->insert( Qt::Key_Escape, QLatin1String( "esc" ) );
  m_qt2katevi->insert( Qt::Key_Tab, QLatin1String( "tab" ) );
  m_qt2katevi->insert( Qt::Key_Backtab, QLatin1String( "backtab" ) );
  m_qt2katevi->insert( Qt::Key_Backspace, QLatin1String( "backspace" ) );
  m_qt2katevi->insert( Qt::Key_Return, QLatin1String( "return" ) );
  m_qt2katevi->insert( Qt::Key_Enter, QLatin1String( "enter" ) );
  m_qt2katevi->insert( Qt::Key_Insert, QLatin1String( "insert" ) );
  m_qt2katevi->insert( Qt::Key_Delete, QLatin1String( "delete" ) );
  m_qt2katevi->insert( Qt::Key_Pause, QLatin1String( "pause" ) );
  m_qt2katevi->insert( Qt::Key_Print, QLatin1String( "print" ) );
  m_qt2katevi->insert( Qt::Key_SysReq, QLatin1String( "sysreq" ) );
  m_qt2katevi->insert( Qt::Key_Clear, QLatin1String( "clear" ) );
  m_qt2katevi->insert( Qt::Key_Home, QLatin1String( "home" ) );
  m_qt2katevi->insert( Qt::Key_End, QLatin1String( "end" ) );
  m_qt2katevi->insert( Qt::Key_Left, QLatin1String( "left" ) );
  m_qt2katevi->insert( Qt::Key_Up, QLatin1String( "up" ) );
  m_qt2katevi->insert( Qt::Key_Right, QLatin1String( "right" ) );
  m_qt2katevi->insert( Qt::Key_Down, QLatin1String( "down" ) );
  m_qt2katevi->insert( Qt::Key_PageUp, QLatin1String( "pageup" ) );
  m_qt2katevi->insert( Qt::Key_PageDown, QLatin1String( "pagedown" ) );
  m_qt2katevi->insert( Qt::Key_Shift, QLatin1String( "shift" ) );
  m_qt2katevi->insert( Qt::Key_Control, QLatin1String( "control" ) );
  m_qt2katevi->insert( Qt::Key_Meta, QLatin1String( "meta" ) );
  m_qt2katevi->insert( Qt::Key_Alt, QLatin1String( "alt" ) );
  m_qt2katevi->insert( Qt::Key_AltGr, QLatin1String( "altgr" ) );
  m_qt2katevi->insert( Qt::Key_CapsLock, QLatin1String( "capslock" ) );
  m_qt2katevi->insert( Qt::Key_NumLock, QLatin1String( "numlock" ) );
  m_qt2katevi->insert( Qt::Key_ScrollLock, QLatin1String( "scrolllock" ) );
  m_qt2katevi->insert( Qt::Key_F1, QLatin1String( "f1" ) );
  m_qt2katevi->insert( Qt::Key_F2, QLatin1String( "f2" ) );
  m_qt2katevi->insert( Qt::Key_F3, QLatin1String( "f3" ) );
  m_qt2katevi->insert( Qt::Key_F4, QLatin1String( "f4" ) );
  m_qt2katevi->insert( Qt::Key_F5, QLatin1String( "f5" ) );
  m_qt2katevi->insert( Qt::Key_F6, QLatin1String( "f6" ) );
  m_qt2katevi->insert( Qt::Key_F7, QLatin1String( "f7" ) );
  m_qt2katevi->insert( Qt::Key_F8, QLatin1String( "f8" ) );
  m_qt2katevi->insert( Qt::Key_F9, QLatin1String( "f9" ) );
  m_qt2katevi->insert( Qt::Key_F10, QLatin1String( "f10" ) );
  m_qt2katevi->insert( Qt::Key_F11, QLatin1String( "f11" ) );
  m_qt2katevi->insert( Qt::Key_F12, QLatin1String( "f12" ) );
  m_qt2katevi->insert( Qt::Key_F13, QLatin1String( "f13" ) );
  m_qt2katevi->insert( Qt::Key_F14, QLatin1String( "f14" ) );
  m_qt2katevi->insert( Qt::Key_F15, QLatin1String( "f15" ) );
  m_qt2katevi->insert( Qt::Key_F16, QLatin1String( "f16" ) );
  m_qt2katevi->insert( Qt::Key_F17, QLatin1String( "f17" ) );
  m_qt2katevi->insert( Qt::Key_F18, QLatin1String( "f18" ) );
  m_qt2katevi->insert( Qt::Key_F19, QLatin1String( "f19" ) );
  m_qt2katevi->insert( Qt::Key_F20, QLatin1String( "f20" ) );
  m_qt2katevi->insert( Qt::Key_F21, QLatin1String( "f21" ) );
  m_qt2katevi->insert( Qt::Key_F22, QLatin1String( "f22" ) );
  m_qt2katevi->insert( Qt::Key_F23, QLatin1String( "f23" ) );
  m_qt2katevi->insert( Qt::Key_F24, QLatin1String( "f24" ) );
  m_qt2katevi->insert( Qt::Key_F25, QLatin1String( "f25" ) );
  m_qt2katevi->insert( Qt::Key_F26, QLatin1String( "f26" ) );
  m_qt2katevi->insert( Qt::Key_F27, QLatin1String( "f27" ) );
  m_qt2katevi->insert( Qt::Key_F28, QLatin1String( "f28" ) );
  m_qt2katevi->insert( Qt::Key_F29, QLatin1String( "f29" ) );
  m_qt2katevi->insert( Qt::Key_F30, QLatin1String( "f30" ) );
  m_qt2katevi->insert( Qt::Key_F31, QLatin1String( "f31" ) );
  m_qt2katevi->insert( Qt::Key_F32, QLatin1String( "f32" ) );
  m_qt2katevi->insert( Qt::Key_F33, QLatin1String( "f33" ) );
  m_qt2katevi->insert( Qt::Key_F34, QLatin1String( "f34" ) );
  m_qt2katevi->insert( Qt::Key_F35, QLatin1String( "f35" ) );
  m_qt2katevi->insert( Qt::Key_Super_L, QLatin1String( "super_l" ) );
  m_qt2katevi->insert( Qt::Key_Super_R, QLatin1String( "super_r" ) );
  m_qt2katevi->insert( Qt::Key_Menu, QLatin1String( "menu" ) );
  m_qt2katevi->insert( Qt::Key_Hyper_L, QLatin1String( "hyper_l" ) );
  m_qt2katevi->insert( Qt::Key_Hyper_R, QLatin1String( "hyper_r" ) );
  m_qt2katevi->insert( Qt::Key_Help, QLatin1String( "help" ) );
  m_qt2katevi->insert( Qt::Key_Direction_L, QLatin1String( "direction_l" ) );
  m_qt2katevi->insert( Qt::Key_Direction_R, QLatin1String( "direction_r" ) );
  m_qt2katevi->insert( Qt::Key_Multi_key, QLatin1String( "multi_key" ) );
  m_qt2katevi->insert( Qt::Key_Codeinput, QLatin1String( "codeinput" ) );
  m_qt2katevi->insert( Qt::Key_SingleCandidate, QLatin1String( "singlecandidate" ) );
  m_qt2katevi->insert( Qt::Key_MultipleCandidate, QLatin1String( "multiplecandidate" ) );
  m_qt2katevi->insert( Qt::Key_PreviousCandidate, QLatin1String( "previouscandidate" ) );
  m_qt2katevi->insert( Qt::Key_Mode_switch, QLatin1String( "mode_switch" ) );
  m_qt2katevi->insert( Qt::Key_Kanji, QLatin1String( "kanji" ) );
  m_qt2katevi->insert( Qt::Key_Muhenkan, QLatin1String( "muhenkan" ) );
  m_qt2katevi->insert( Qt::Key_Henkan, QLatin1String( "henkan" ) );
  m_qt2katevi->insert( Qt::Key_Romaji, QLatin1String( "romaji" ) );
  m_qt2katevi->insert( Qt::Key_Hiragana, QLatin1String( "hiragana" ) );
  m_qt2katevi->insert( Qt::Key_Katakana, QLatin1String( "katakana" ) );
  m_qt2katevi->insert( Qt::Key_Hiragana_Katakana, QLatin1String( "hiragana_katakana" ) );
  m_qt2katevi->insert( Qt::Key_Zenkaku, QLatin1String( "zenkaku" ) );
  m_qt2katevi->insert( Qt::Key_Hankaku, QLatin1String( "hankaku" ) );
  m_qt2katevi->insert( Qt::Key_Zenkaku_Hankaku, QLatin1String( "zenkaku_hankaku" ) );
  m_qt2katevi->insert( Qt::Key_Touroku, QLatin1String( "touroku" ) );
  m_qt2katevi->insert( Qt::Key_Massyo, QLatin1String( "massyo" ) );
  m_qt2katevi->insert( Qt::Key_Kana_Lock, QLatin1String( "kana_lock" ) );
  m_qt2katevi->insert( Qt::Key_Kana_Shift, QLatin1String( "kana_shift" ) );
  m_qt2katevi->insert( Qt::Key_Eisu_Shift, QLatin1String( "eisu_shift" ) );
  m_qt2katevi->insert( Qt::Key_Eisu_toggle, QLatin1String( "eisu_toggle" ) );
  m_qt2katevi->insert( Qt::Key_Hangul, QLatin1String( "hangul" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Start, QLatin1String( "hangul_start" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_End, QLatin1String( "hangul_end" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Hanja, QLatin1String( "hangul_hanja" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Jamo, QLatin1String( "hangul_jamo" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Romaja, QLatin1String( "hangul_romaja" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Jeonja, QLatin1String( "hangul_jeonja" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Banja, QLatin1String( "hangul_banja" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_PreHanja, QLatin1String( "hangul_prehanja" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_PostHanja, QLatin1String( "hangul_posthanja" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Special, QLatin1String( "hangul_special" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Grave, QLatin1String( "dead_grave" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Acute, QLatin1String( "dead_acute" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Circumflex, QLatin1String( "dead_circumflex" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Tilde, QLatin1String( "dead_tilde" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Macron, QLatin1String( "dead_macron" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Breve, QLatin1String( "dead_breve" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Abovedot, QLatin1String( "dead_abovedot" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Diaeresis, QLatin1String( "dead_diaeresis" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Abovering, QLatin1String( "dead_abovering" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Doubleacute, QLatin1String( "dead_doubleacute" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Caron, QLatin1String( "dead_caron" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Cedilla, QLatin1String( "dead_cedilla" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Ogonek, QLatin1String( "dead_ogonek" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Iota, QLatin1String( "dead_iota" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Voiced_Sound, QLatin1String( "dead_voiced_sound" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Semivoiced_Sound, QLatin1String( "dead_semivoiced_sound" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Belowdot, QLatin1String( "dead_belowdot" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Hook, QLatin1String( "dead_hook" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Horn, QLatin1String( "dead_horn" ) );
  m_qt2katevi->insert( Qt::Key_Back, QLatin1String( "back" ) );
  m_qt2katevi->insert( Qt::Key_Forward, QLatin1String( "forward" ) );
  m_qt2katevi->insert( Qt::Key_Stop, QLatin1String( "stop" ) );
  m_qt2katevi->insert( Qt::Key_Refresh, QLatin1String( "refresh" ) );
  m_qt2katevi->insert( Qt::Key_VolumeDown, QLatin1String( "volumedown" ) );
  m_qt2katevi->insert( Qt::Key_VolumeMute, QLatin1String( "volumemute" ) );
  m_qt2katevi->insert( Qt::Key_VolumeUp, QLatin1String( "volumeup" ) );
  m_qt2katevi->insert( Qt::Key_BassBoost, QLatin1String( "bassboost" ) );
  m_qt2katevi->insert( Qt::Key_BassUp, QLatin1String( "bassup" ) );
  m_qt2katevi->insert( Qt::Key_BassDown, QLatin1String( "bassdown" ) );
  m_qt2katevi->insert( Qt::Key_TrebleUp, QLatin1String( "trebleup" ) );
  m_qt2katevi->insert( Qt::Key_TrebleDown, QLatin1String( "trebledown" ) );
  m_qt2katevi->insert( Qt::Key_MediaPlay, QLatin1String( "mediaplay" ) );
  m_qt2katevi->insert( Qt::Key_MediaStop, QLatin1String( "mediastop" ) );
  m_qt2katevi->insert( Qt::Key_MediaPrevious, QLatin1String( "mediaprevious" ) );
  m_qt2katevi->insert( Qt::Key_MediaNext, QLatin1String( "medianext" ) );
  m_qt2katevi->insert( Qt::Key_MediaRecord, QLatin1String( "mediarecord" ) );
  m_qt2katevi->insert( Qt::Key_HomePage, QLatin1String( "homepage" ) );
  m_qt2katevi->insert( Qt::Key_Favorites, QLatin1String( "favorites" ) );
  m_qt2katevi->insert( Qt::Key_Search, QLatin1String( "search" ) );
  m_qt2katevi->insert( Qt::Key_Standby, QLatin1String( "standby" ) );
  m_qt2katevi->insert( Qt::Key_OpenUrl, QLatin1String( "openurl" ) );
  m_qt2katevi->insert( Qt::Key_LaunchMail, QLatin1String( "launchmail" ) );
  m_qt2katevi->insert( Qt::Key_LaunchMedia, QLatin1String( "launchmedia" ) );
  m_qt2katevi->insert( Qt::Key_Launch0, QLatin1String( "launch0" ) );
  m_qt2katevi->insert( Qt::Key_Launch1, QLatin1String( "launch1" ) );
  m_qt2katevi->insert( Qt::Key_Launch2, QLatin1String( "launch2" ) );
  m_qt2katevi->insert( Qt::Key_Launch3, QLatin1String( "launch3" ) );
  m_qt2katevi->insert( Qt::Key_Launch4, QLatin1String( "launch4" ) );
  m_qt2katevi->insert( Qt::Key_Launch5, QLatin1String( "launch5" ) );
  m_qt2katevi->insert( Qt::Key_Launch6, QLatin1String( "launch6" ) );
  m_qt2katevi->insert( Qt::Key_Launch7, QLatin1String( "launch7" ) );
  m_qt2katevi->insert( Qt::Key_Launch8, QLatin1String( "launch8" ) );
  m_qt2katevi->insert( Qt::Key_Launch9, QLatin1String( "launch9" ) );
  m_qt2katevi->insert( Qt::Key_LaunchA, QLatin1String( "launcha" ) );
  m_qt2katevi->insert( Qt::Key_LaunchB, QLatin1String( "launchb" ) );
  m_qt2katevi->insert( Qt::Key_LaunchC, QLatin1String( "launchc" ) );
  m_qt2katevi->insert( Qt::Key_LaunchD, QLatin1String( "launchd" ) );
  m_qt2katevi->insert( Qt::Key_LaunchE, QLatin1String( "launche" ) );
  m_qt2katevi->insert( Qt::Key_LaunchF, QLatin1String( "launchf" ) );
  m_qt2katevi->insert( Qt::Key_MediaLast, QLatin1String( "medialast" ) );
  m_qt2katevi->insert( Qt::Key_unknown, QLatin1String( "unknown" ) );
  m_qt2katevi->insert( Qt::Key_Call, QLatin1String( "call" ) );
  m_qt2katevi->insert( Qt::Key_Context1, QLatin1String( "context1" ) );
  m_qt2katevi->insert( Qt::Key_Context2, QLatin1String( "context2" ) );
  m_qt2katevi->insert( Qt::Key_Context3, QLatin1String( "context3" ) );
  m_qt2katevi->insert( Qt::Key_Context4, QLatin1String( "context4" ) );
  m_qt2katevi->insert( Qt::Key_Flip, QLatin1String( "flip" ) );
  m_qt2katevi->insert( Qt::Key_Hangup, QLatin1String( "hangup" ) );
  m_qt2katevi->insert( Qt::Key_No, QLatin1String( "no" ) );
  m_qt2katevi->insert( Qt::Key_Select, QLatin1String( "select" ) );
  m_qt2katevi->insert( Qt::Key_Yes, QLatin1String( "yes" ) );
  m_qt2katevi->insert( Qt::Key_Execute, QLatin1String( "execute" ) );
  m_qt2katevi->insert( Qt::Key_Printer, QLatin1String( "printer" ) );
  m_qt2katevi->insert( Qt::Key_Play, QLatin1String( "play" ) );
  m_qt2katevi->insert( Qt::Key_Sleep, QLatin1String( "sleep" ) );
  m_qt2katevi->insert( Qt::Key_Zoom, QLatin1String( "zoom" ) );
  m_qt2katevi->insert( Qt::Key_Cancel, QLatin1String( "cancel" ) );

  for (QHash<int, QString>::const_iterator i = m_qt2katevi->constBegin();
      i != m_qt2katevi->constEnd(); ++i) {
    m_katevi2qt->insert( i.value(), i.key() );
  }

  m_nameToKeyCode->insert( QLatin1String( "invalid" ), -1 );
  m_nameToKeyCode->insert( QLatin1String( "esc" ), 1 );
  m_nameToKeyCode->insert( QLatin1String( "tab" ), 2 );
  m_nameToKeyCode->insert( QLatin1String( "backtab" ), 3 );
  m_nameToKeyCode->insert( QLatin1String( "backspace" ), 4 );
  m_nameToKeyCode->insert( QLatin1String( "return" ), 5 );
  m_nameToKeyCode->insert( QLatin1String( "enter" ), 6 );
  m_nameToKeyCode->insert( QLatin1String( "insert" ), 7 );
  m_nameToKeyCode->insert( QLatin1String( "delete" ), 8 );
  m_nameToKeyCode->insert( QLatin1String( "pause" ), 9 );
  m_nameToKeyCode->insert( QLatin1String( "print" ), 10 );
  m_nameToKeyCode->insert( QLatin1String( "sysreq" ), 11 );
  m_nameToKeyCode->insert( QLatin1String( "clear" ), 12 );
  m_nameToKeyCode->insert( QLatin1String( "home" ), 13 );
  m_nameToKeyCode->insert( QLatin1String( "end" ), 14 );
  m_nameToKeyCode->insert( QLatin1String( "left" ), 15 );
  m_nameToKeyCode->insert( QLatin1String( "up" ), 16 );
  m_nameToKeyCode->insert( QLatin1String( "right" ), 17 );
  m_nameToKeyCode->insert( QLatin1String( "down" ), 18 );
  m_nameToKeyCode->insert( QLatin1String( "pageup" ), 19 );
  m_nameToKeyCode->insert( QLatin1String( "pagedown" ), 20 );
  m_nameToKeyCode->insert( QLatin1String( "shift" ), 21 );
  m_nameToKeyCode->insert( QLatin1String( "control" ), 22 );
  m_nameToKeyCode->insert( QLatin1String( "meta" ), 23 );
  m_nameToKeyCode->insert( QLatin1String( "alt" ), 24 );
  m_nameToKeyCode->insert( QLatin1String( "altgr" ), 25 );
  m_nameToKeyCode->insert( QLatin1String( "capslock" ), 26 );
  m_nameToKeyCode->insert( QLatin1String( "numlock" ), 27 );
  m_nameToKeyCode->insert( QLatin1String( "scrolllock" ), 28 );
  m_nameToKeyCode->insert( QLatin1String( "f1" ), 29 );
  m_nameToKeyCode->insert( QLatin1String( "f2" ), 30 );
  m_nameToKeyCode->insert( QLatin1String( "f3" ), 31 );
  m_nameToKeyCode->insert( QLatin1String( "f4" ), 32 );
  m_nameToKeyCode->insert( QLatin1String( "f5" ), 33 );
  m_nameToKeyCode->insert( QLatin1String( "f6" ), 34 );
  m_nameToKeyCode->insert( QLatin1String( "f7" ), 35 );
  m_nameToKeyCode->insert( QLatin1String( "f8" ), 36 );
  m_nameToKeyCode->insert( QLatin1String( "f9" ), 37 );
  m_nameToKeyCode->insert( QLatin1String( "f10" ), 38 );
  m_nameToKeyCode->insert( QLatin1String( "f11" ), 39 );
  m_nameToKeyCode->insert( QLatin1String( "f12" ), 40 );
  m_nameToKeyCode->insert( QLatin1String( "f13" ), 41 );
  m_nameToKeyCode->insert( QLatin1String( "f14" ), 42 );
  m_nameToKeyCode->insert( QLatin1String( "f15" ), 43 );
  m_nameToKeyCode->insert( QLatin1String( "f16" ), 44 );
  m_nameToKeyCode->insert( QLatin1String( "f17" ), 45 );
  m_nameToKeyCode->insert( QLatin1String( "f18" ), 46 );
  m_nameToKeyCode->insert( QLatin1String( "f19" ), 47 );
  m_nameToKeyCode->insert( QLatin1String( "f20" ), 48 );
  m_nameToKeyCode->insert( QLatin1String( "f21" ), 49 );
  m_nameToKeyCode->insert( QLatin1String( "f22" ), 50 );
  m_nameToKeyCode->insert( QLatin1String( "f23" ), 51 );
  m_nameToKeyCode->insert( QLatin1String( "f24" ), 52 );
  m_nameToKeyCode->insert( QLatin1String( "f25" ), 53 );
  m_nameToKeyCode->insert( QLatin1String( "f26" ), 54 );
  m_nameToKeyCode->insert( QLatin1String( "f27" ), 55 );
  m_nameToKeyCode->insert( QLatin1String( "f28" ), 56 );
  m_nameToKeyCode->insert( QLatin1String( "f29" ), 57 );
  m_nameToKeyCode->insert( QLatin1String( "f30" ), 58 );
  m_nameToKeyCode->insert( QLatin1String( "f31" ), 59 );
  m_nameToKeyCode->insert( QLatin1String( "f32" ), 60 );
  m_nameToKeyCode->insert( QLatin1String( "f33" ), 61 );
  m_nameToKeyCode->insert( QLatin1String( "f34" ), 62 );
  m_nameToKeyCode->insert( QLatin1String( "f35" ), 63 );
  m_nameToKeyCode->insert( QLatin1String( "super_l" ), 64 );
  m_nameToKeyCode->insert( QLatin1String( "super_r" ), 65 );
  m_nameToKeyCode->insert( QLatin1String( "menu" ), 66 );
  m_nameToKeyCode->insert( QLatin1String( "hyper_l" ), 67 );
  m_nameToKeyCode->insert( QLatin1String( "hyper_r" ), 68 );
  m_nameToKeyCode->insert( QLatin1String( "help" ), 69 );
  m_nameToKeyCode->insert( QLatin1String( "direction_l" ), 70 );
  m_nameToKeyCode->insert( QLatin1String( "direction_r" ), 71 );
  m_nameToKeyCode->insert( QLatin1String( "multi_key" ), 172 );
  m_nameToKeyCode->insert( QLatin1String( "codeinput" ), 173 );
  m_nameToKeyCode->insert( QLatin1String( "singlecandidate" ), 174 );
  m_nameToKeyCode->insert( QLatin1String( "multiplecandidate" ), 175 );
  m_nameToKeyCode->insert( QLatin1String( "previouscandidate" ), 176 );
  m_nameToKeyCode->insert( QLatin1String( "mode_switch" ), 177 );
  m_nameToKeyCode->insert( QLatin1String( "kanji" ), 178 );
  m_nameToKeyCode->insert( QLatin1String( "muhenkan" ), 179 );
  m_nameToKeyCode->insert( QLatin1String( "henkan" ), 180 );
  m_nameToKeyCode->insert( QLatin1String( "romaji" ), 181 );
  m_nameToKeyCode->insert( QLatin1String( "hiragana" ), 182 );
  m_nameToKeyCode->insert( QLatin1String( "katakana" ), 183 );
  m_nameToKeyCode->insert( QLatin1String( "hiragana_katakana" ), 184 );
  m_nameToKeyCode->insert( QLatin1String( "zenkaku" ), 185 );
  m_nameToKeyCode->insert( QLatin1String( "hankaku" ), 186 );
  m_nameToKeyCode->insert( QLatin1String( "zenkaku_hankaku" ), 187 );
  m_nameToKeyCode->insert( QLatin1String( "touroku" ), 188 );
  m_nameToKeyCode->insert( QLatin1String( "massyo" ), 189 );
  m_nameToKeyCode->insert( QLatin1String( "kana_lock" ), 190 );
  m_nameToKeyCode->insert( QLatin1String( "kana_shift" ), 191 );
  m_nameToKeyCode->insert( QLatin1String( "eisu_shift" ), 192 );
  m_nameToKeyCode->insert( QLatin1String( "eisu_toggle" ), 193 );
  m_nameToKeyCode->insert( QLatin1String( "hangul" ), 194 );
  m_nameToKeyCode->insert( QLatin1String( "hangul_start" ), 195 );
  m_nameToKeyCode->insert( QLatin1String( "hangul_end" ), 196 );
  m_nameToKeyCode->insert( QLatin1String( "hangul_hanja" ), 197 );
  m_nameToKeyCode->insert( QLatin1String( "hangul_jamo" ), 198 );
  m_nameToKeyCode->insert( QLatin1String( "hangul_romaja" ), 199 );
  m_nameToKeyCode->insert( QLatin1String( "hangul_jeonja" ), 200 );
  m_nameToKeyCode->insert( QLatin1String( "hangul_banja" ), 201 );
  m_nameToKeyCode->insert( QLatin1String( "hangul_prehanja" ), 202 );
  m_nameToKeyCode->insert( QLatin1String( "hangul_posthanja" ), 203 );
  m_nameToKeyCode->insert( QLatin1String( "hangul_special" ), 204 );
  m_nameToKeyCode->insert( QLatin1String( "dead_grave" ), 205 );
  m_nameToKeyCode->insert( QLatin1String( "dead_acute" ), 206 );
  m_nameToKeyCode->insert( QLatin1String( "dead_circumflex" ), 207 );
  m_nameToKeyCode->insert( QLatin1String( "dead_tilde" ), 208 );
  m_nameToKeyCode->insert( QLatin1String( "dead_macron" ), 209 );
  m_nameToKeyCode->insert( QLatin1String( "dead_breve" ), 210 );
  m_nameToKeyCode->insert( QLatin1String( "dead_abovedot" ), 211 );
  m_nameToKeyCode->insert( QLatin1String( "dead_diaeresis" ), 212 );
  m_nameToKeyCode->insert( QLatin1String( "dead_abovering" ), 213 );
  m_nameToKeyCode->insert( QLatin1String( "dead_doubleacute" ), 214 );
  m_nameToKeyCode->insert( QLatin1String( "dead_caron" ), 215 );
  m_nameToKeyCode->insert( QLatin1String( "dead_cedilla" ), 216 );
  m_nameToKeyCode->insert( QLatin1String( "dead_ogonek" ), 217 );
  m_nameToKeyCode->insert( QLatin1String( "dead_iota" ), 218 );
  m_nameToKeyCode->insert( QLatin1String( "dead_voiced_sound" ), 219 );
  m_nameToKeyCode->insert( QLatin1String( "dead_semivoiced_sound" ), 220 );
  m_nameToKeyCode->insert( QLatin1String( "dead_belowdot" ), 221 );
  m_nameToKeyCode->insert( QLatin1String( "dead_hook" ), 222 );
  m_nameToKeyCode->insert( QLatin1String( "dead_horn" ), 223 );
  m_nameToKeyCode->insert( QLatin1String( "back" ), 224 );
  m_nameToKeyCode->insert( QLatin1String( "forward" ), 225 );
  m_nameToKeyCode->insert( QLatin1String( "stop" ), 226 );
  m_nameToKeyCode->insert( QLatin1String( "refresh" ), 227 );
  m_nameToKeyCode->insert( QLatin1String( "volumedown" ), 228 );
  m_nameToKeyCode->insert( QLatin1String( "volumemute" ), 229 );
  m_nameToKeyCode->insert( QLatin1String( "volumeup" ), 230 );
  m_nameToKeyCode->insert( QLatin1String( "bassboost" ), 231 );
  m_nameToKeyCode->insert( QLatin1String( "bassup" ), 232 );
  m_nameToKeyCode->insert( QLatin1String( "bassdown" ), 233 );
  m_nameToKeyCode->insert( QLatin1String( "trebleup" ), 234 );
  m_nameToKeyCode->insert( QLatin1String( "trebledown" ), 235 );
  m_nameToKeyCode->insert( QLatin1String( "mediaplay" ), 236 );
  m_nameToKeyCode->insert( QLatin1String( "mediastop" ), 237 );
  m_nameToKeyCode->insert( QLatin1String( "mediaprevious" ), 238 );
  m_nameToKeyCode->insert( QLatin1String( "medianext" ), 239 );
  m_nameToKeyCode->insert( QLatin1String( "mediarecord" ), 240 );
  m_nameToKeyCode->insert( QLatin1String( "homepage" ), 241 );
  m_nameToKeyCode->insert( QLatin1String( "favorites" ), 242 );
  m_nameToKeyCode->insert( QLatin1String( "search" ), 243 );
  m_nameToKeyCode->insert( QLatin1String( "standby" ), 244 );
  m_nameToKeyCode->insert( QLatin1String( "openurl" ), 245 );
  m_nameToKeyCode->insert( QLatin1String( "launchmail" ), 246 );
  m_nameToKeyCode->insert( QLatin1String( "launchmedia" ), 247 );
  m_nameToKeyCode->insert( QLatin1String( "launch0" ), 248 );
  m_nameToKeyCode->insert( QLatin1String( "launch1" ), 249 );
  m_nameToKeyCode->insert( QLatin1String( "launch2" ), 250 );
  m_nameToKeyCode->insert( QLatin1String( "launch3" ), 251 );
  m_nameToKeyCode->insert( QLatin1String( "launch4" ), 252 );
  m_nameToKeyCode->insert( QLatin1String( "launch5" ), 253 );
  m_nameToKeyCode->insert( QLatin1String( "launch6" ), 254 );
  m_nameToKeyCode->insert( QLatin1String( "launch7" ), 255 );
  m_nameToKeyCode->insert( QLatin1String( "launch8" ), 256 );
  m_nameToKeyCode->insert( QLatin1String( "launch9" ), 257 );
  m_nameToKeyCode->insert( QLatin1String( "launcha" ), 258 );
  m_nameToKeyCode->insert( QLatin1String( "launchb" ), 259 );
  m_nameToKeyCode->insert( QLatin1String( "launchc" ), 260 );
  m_nameToKeyCode->insert( QLatin1String( "launchd" ), 261 );
  m_nameToKeyCode->insert( QLatin1String( "launche" ), 262 );
  m_nameToKeyCode->insert( QLatin1String( "launchf" ), 263 );
  m_nameToKeyCode->insert( QLatin1String( "medialast" ), 264 );
  m_nameToKeyCode->insert( QLatin1String( "unknown" ), 265 );
  m_nameToKeyCode->insert( QLatin1String( "call" ), 266 );
  m_nameToKeyCode->insert( QLatin1String( "context1" ), 267 );
  m_nameToKeyCode->insert( QLatin1String( "context2" ), 268 );
  m_nameToKeyCode->insert( QLatin1String( "context3" ), 269 );
  m_nameToKeyCode->insert( QLatin1String( "context4" ), 270 );
  m_nameToKeyCode->insert( QLatin1String( "flip" ), 271 );
  m_nameToKeyCode->insert( QLatin1String( "hangup" ), 272 );
  m_nameToKeyCode->insert( QLatin1String( "no" ), 273 );
  m_nameToKeyCode->insert( QLatin1String( "select" ), 274 );
  m_nameToKeyCode->insert( QLatin1String( "yes" ), 275 );
  m_nameToKeyCode->insert( QLatin1String( "execute" ), 276 );
  m_nameToKeyCode->insert( QLatin1String( "printer" ), 277 );
  m_nameToKeyCode->insert( QLatin1String( "play" ), 278 );
  m_nameToKeyCode->insert( QLatin1String( "sleep" ), 279 );
  m_nameToKeyCode->insert( QLatin1String( "zoom" ), 280 );
  m_nameToKeyCode->insert( QLatin1String( "cancel" ), 281 );

  m_nameToKeyCode->insert( QLatin1String( "a" ), 282 );
  m_nameToKeyCode->insert( QLatin1String( "b" ), 283 );
  m_nameToKeyCode->insert( QLatin1String( "c" ), 284 );
  m_nameToKeyCode->insert( QLatin1String( "d" ), 285 );
  m_nameToKeyCode->insert( QLatin1String( "e" ), 286 );
  m_nameToKeyCode->insert( QLatin1String( "f" ), 287 );
  m_nameToKeyCode->insert( QLatin1String( "g" ), 288 );
  m_nameToKeyCode->insert( QLatin1String( "h" ), 289 );
  m_nameToKeyCode->insert( QLatin1String( "i" ), 290 );
  m_nameToKeyCode->insert( QLatin1String( "j" ), 291 );
  m_nameToKeyCode->insert( QLatin1String( "k" ), 292 );
  m_nameToKeyCode->insert( QLatin1String( "l" ), 293 );
  m_nameToKeyCode->insert( QLatin1String( "m" ), 294 );
  m_nameToKeyCode->insert( QLatin1String( "n" ), 295 );
  m_nameToKeyCode->insert( QLatin1String( "o" ), 296 );
  m_nameToKeyCode->insert( QLatin1String( "p" ), 297 );
  m_nameToKeyCode->insert( QLatin1String( "q" ), 298 );
  m_nameToKeyCode->insert( QLatin1String( "r" ), 299 );
  m_nameToKeyCode->insert( QLatin1String( "s" ), 300 );
  m_nameToKeyCode->insert( QLatin1String( "t" ), 301 );
  m_nameToKeyCode->insert( QLatin1String( "u" ), 302 );
  m_nameToKeyCode->insert( QLatin1String( "v" ), 303 );
  m_nameToKeyCode->insert( QLatin1String( "w" ), 304 );
  m_nameToKeyCode->insert( QLatin1String( "x" ), 305 );
  m_nameToKeyCode->insert( QLatin1String( "y" ), 306 );
  m_nameToKeyCode->insert( QLatin1String( "z" ), 307 );
  m_nameToKeyCode->insert( QLatin1String( "`" ), 308 );
  m_nameToKeyCode->insert( QLatin1String( "!" ), 309 );
  m_nameToKeyCode->insert( QLatin1String( "\"" ), 310 );
  m_nameToKeyCode->insert( QLatin1String( "$" ), 311 );
  m_nameToKeyCode->insert( QLatin1String( "%" ), 312 );
  m_nameToKeyCode->insert( QLatin1String( "^" ), 313 );
  m_nameToKeyCode->insert( QLatin1String( "&" ), 314 );
  m_nameToKeyCode->insert( QLatin1String( "*" ), 315 );
  m_nameToKeyCode->insert( QLatin1String( "(" ), 316 );
  m_nameToKeyCode->insert( QLatin1String( ")" ), 317 );
  m_nameToKeyCode->insert( QLatin1String( "-" ), 318 );
  m_nameToKeyCode->insert( QLatin1String( "_" ), 319 );
  m_nameToKeyCode->insert( QLatin1String( "=" ), 320 );
  m_nameToKeyCode->insert( QLatin1String( "+" ), 321 );
  m_nameToKeyCode->insert( QLatin1String( "[" ), 322 );
  m_nameToKeyCode->insert( QLatin1String( "]" ), 323 );
  m_nameToKeyCode->insert( QLatin1String( "{" ), 324 );
  m_nameToKeyCode->insert( QLatin1String( "}" ), 325 );
  m_nameToKeyCode->insert( QLatin1String( ":" ), 326 );
  m_nameToKeyCode->insert( QLatin1String( ";" ), 327 );
  m_nameToKeyCode->insert( QLatin1String( "@" ), 328 );
  m_nameToKeyCode->insert( QLatin1String( "'" ), 329 );
  m_nameToKeyCode->insert( QLatin1String( "#" ), 330 );
  m_nameToKeyCode->insert( QLatin1String( "~" ), 331 );
  m_nameToKeyCode->insert( QLatin1String( "\\" ), 332 );
  m_nameToKeyCode->insert( QLatin1String( "|" ), 333 );
  m_nameToKeyCode->insert( QLatin1String( "," ), 334 );
  m_nameToKeyCode->insert( QLatin1String( "." ), 335 );
  //m_nameToKeyCode->insert( QLatin1String( ">" ), 336 );
  m_nameToKeyCode->insert( QLatin1String( "/" ), 337 );
  m_nameToKeyCode->insert( QLatin1String( "?" ), 338 );
  m_nameToKeyCode->insert( QLatin1String( " " ), 339 );
  //m_nameToKeyCode->insert( QLatin1String( "<" ), 341 );
  m_nameToKeyCode->insert( QLatin1String("0"), 340);
  m_nameToKeyCode->insert( QLatin1String("1"), 341);
  m_nameToKeyCode->insert( QLatin1String("2"), 342);
  m_nameToKeyCode->insert( QLatin1String("3"), 343);
  m_nameToKeyCode->insert( QLatin1String("4"), 344);
  m_nameToKeyCode->insert( QLatin1String("5"), 345);
  m_nameToKeyCode->insert( QLatin1String("6"), 346);
  m_nameToKeyCode->insert( QLatin1String("7"), 347);
  m_nameToKeyCode->insert( QLatin1String("8"), 348);
  m_nameToKeyCode->insert( QLatin1String("9"), 349);


  for (QHash<QString, int>::const_iterator i = m_nameToKeyCode->constBegin();
      i != m_nameToKeyCode->constEnd(); ++i) {
    m_keyCodeToName->insert( i.value(), i.key() );
  }
}

QString KateViKeyParser::qt2vi( int key ) const
{
  return ( m_qt2katevi->contains( key ) ? m_qt2katevi->value( key ) : QLatin1String("invalid") );
}

int KateViKeyParser::vi2qt( const QString &keypress ) const
{
  return ( m_katevi2qt->contains( keypress ) ? m_katevi2qt->value( keypress ) : -1 );
}


const QString KateViKeyParser::encodeKeySequence( const QString &keys ) const
{
  QString encodedSequence;
  unsigned int keyCodeTemp = 0;

  bool insideTag = false;
  QChar c;
  for ( int i = 0; i < keys.length(); i++ ) {
    c = keys.at( i );
    if ( insideTag ) {
      if ( c == QLatin1Char('>') ) {
        QString temp;
        temp.setNum( 0xE000+keyCodeTemp, 16);
        QChar code(0xE000+keyCodeTemp );
        encodedSequence.append( code );
        keyCodeTemp = 0;
        insideTag = false;
        continue;
      }
      else {
        // contains modifiers
        if ( keys.mid( i ).indexOf( QLatin1Char('-') ) != -1 && keys.mid( i ).indexOf( QLatin1Char('-') ) < keys.mid( i ).indexOf( QLatin1Char('>') ) ) {
          // Perform something similar to a split on '-', except that we want to keep the occurrences of '-'
          // e.g. <c-s-a> will equate to the list of tokens "c-", "s-", "a".
          // A straight split on '-' would give us "c", "s", "a", in which case we lose the piece of information that
          // 'a' is just the 'a' key, not the 'alt' modifier.
          const QString untilClosing = keys.mid( i, keys.mid( i ).indexOf( QLatin1Char('>') ) ).toLower();
          QStringList tokens;
          int currentPos = 0;
          int nextHypen = -1;
          while ((nextHypen = untilClosing.indexOf(QLatin1Char('-'), currentPos)) != -1)
          {
            tokens << untilClosing.mid(currentPos, nextHypen - currentPos + 1);
            currentPos = nextHypen + 1;
          }
          tokens << untilClosing.mid(currentPos);

          foreach ( const QString& str, tokens ) {
            if ( str == QLatin1String("s-") && ( keyCodeTemp & 0x01 ) != 0x1  )
              keyCodeTemp += 0x1;
            else if ( str == QLatin1String("c-") && ( keyCodeTemp & 0x02 ) != 0x2 )
              keyCodeTemp += 0x2;
            else if ( str == QLatin1String("a-") && ( keyCodeTemp & 0x04 ) != 0x4 )
              keyCodeTemp += 0x4;
            else if ( str == QLatin1String("m-") && ( keyCodeTemp & 0x08 ) != 0x8 )
              keyCodeTemp += 0x8;
            else {
              if ( m_nameToKeyCode->contains( str ) || ( str.length() == 1 && str.at( 0 ).isLetterOrNumber() ) ) {
                if ( m_nameToKeyCode->contains( str ) ) {
                  keyCodeTemp += m_nameToKeyCode->value( str ) * 0x10;
                } else {
                  keyCodeTemp += str.at( 0 ).unicode() * 0x10;
                }
              } else {
                int endOfBlock = keys.indexOf( QLatin1Char('>') );
                if ( endOfBlock -= -1 ) {
                  endOfBlock = keys.length()-1;
                }
                encodedSequence.clear();
                encodedSequence.append( m_nameToKeyCode->value( QLatin1String("invalid") ) );
                break;
              }
            }
          }
        }
        else {
          QString str = keys.mid( i, keys.indexOf( QLatin1Char('>'), i )-i ).toLower();
          if ( keys.indexOf( QLatin1Char('>'), i ) != -1 && ( m_nameToKeyCode->contains( str ) || ( str.length() == 1 && str.at( 0 ).isLetterOrNumber() ) ) ) {
            if ( m_nameToKeyCode->contains( str ) ) {
              keyCodeTemp += m_nameToKeyCode->value( str ) * 0x10;
            } else {
              keyCodeTemp += str.at( 0 ).unicode() * 0x10;
            }
          } else {
            int endOfBlock = keys.indexOf( QLatin1Char('>') );
            if ( endOfBlock -= -1 ) {
              endOfBlock = keys.length()-1;
            }
            encodedSequence.clear();
            keyCodeTemp = m_nameToKeyCode->value( QLatin1String("invalid") ) * 0x10;
          }

        }
        i += keys.mid( i, keys.mid( i ).indexOf( QLatin1Char('>') ) ).length()-1;
      }
    }
    else {
      if ( c == QLatin1Char('<') ) {
        // If there's no closing '>', or if there is an opening '<' before the next '>', interpret as a literal '<'
        // If we are <space>, encode as a literal " ".
        QString rest = keys.mid( i );
        if (rest.indexOf(QLatin1Char('>'), 1) != -1 && rest.mid(1, rest.indexOf(QLatin1Char('>'), 1) - 1) == QLatin1String("space"))
        {
          encodedSequence.append(QLatin1String(" "));
          i += rest.indexOf(QLatin1Char('>'), 1);
          continue;
        }
        else if ( rest.indexOf( QLatin1Char('>'), 1 ) == -1 || ( rest.indexOf( QLatin1Char('<'), 1 ) < rest.indexOf( QLatin1Char('>'), 1 ) && rest.indexOf( QLatin1Char('<'), 1 ) != -1 ) ) {
          encodedSequence.append( c );
          continue;
        }
        insideTag = true;
        continue;
      } else {
        encodedSequence.append( c );
      }
    }
  }

  return encodedSequence;
}

const QString KateViKeyParser::decodeKeySequence( const QString &keys ) const
{
  QString ret;

  for ( int i = 0; i < keys.length(); i++ ) {
    QChar c = keys.at( i );
    int keycode = c.unicode();

    if ( ( keycode & 0xE000 ) != 0xE000  ) {
      ret.append( c );
    } else {
      ret.append( QLatin1Char('<') );

      if ( ( keycode & 0x1 ) == 0x1 ) {
        ret.append( QLatin1String( "s-") );
        //keycode -= 0x1;
      }
      if ( ( keycode & 0x2 ) == 0x2 ) {
        ret.append( QLatin1String( "c-") );
        //keycode -= 0x2;
      }
      if ( ( keycode & 0x4 ) == 0x4 ) {
        ret.append( QLatin1String( "a-") );
        //keycode -= 0x4;
      }
      if ( ( keycode & 0x8 ) == 0x8 ) {
        ret.append( QLatin1String( "m-") );
        //keycode -= 0x8;
      }

      if ( ( keycode & 0xE000 ) == 0xE000  ) {
        ret.append( m_keyCodeToName->value( ( keycode-0xE000 )/0x10 ) );
      } else {
        ret.append( QChar( keycode ) );
      }
      ret.append( QLatin1Char('>') );
    }
  }

  return ret;
}

const QChar KateViKeyParser::KeyEventToQChar(const QKeyEvent& keyEvent)
{
  const int keyCode = keyEvent.key();
  const QString &text = keyEvent.text();
  const Qt::KeyboardModifiers mods = keyEvent.modifiers();

  // If previous key press was AltGr, return key value right away and don't go
  // down the "handle modifiers" code path. AltGr is really confusing...
  if ( m_altGrPressed ) {
    return ( !text.isEmpty() ) ? text.at(0) : QChar();
  }

  if ( text.isEmpty() || ( text.length() == 1 && text.at(0) < 0x20 ) || keyCode == Qt::Key_Delete
      || ( mods != Qt::NoModifier && mods != Qt::ShiftModifier && mods != Qt::KeypadModifier && mods != Qt::GroupSwitchModifier) ) {
    QString keyPress;

    keyPress.append( QLatin1Char('<') );
    keyPress.append( ( mods & Qt::ShiftModifier )   ? QLatin1String("s-") : QString() );
    keyPress.append( ( mods & Qt::ControlModifier ) ? QLatin1String("c-") : QString() );
    keyPress.append( ( mods & Qt::AltModifier )     ? QLatin1String("a-") : QString() );
    keyPress.append( ( mods & Qt::MetaModifier )    ? QLatin1String("m-") : QString() );
    keyPress.append( keyCode <= 0xFF ? QChar( keyCode ) : qt2vi( keyCode ) );
    keyPress.append( QLatin1Char('>') );

    return encodeKeySequence( keyPress ).at( 0 );
  } else {
    return text.at(0);
  }
}
