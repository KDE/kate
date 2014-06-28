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
  m_qt2katevi->insert( Qt::Key_Escape, QString( "esc" ) );
  m_qt2katevi->insert( Qt::Key_Tab, QString( "tab" ) );
  m_qt2katevi->insert( Qt::Key_Backtab, QString( "backtab" ) );
  m_qt2katevi->insert( Qt::Key_Backspace, QString( "backspace" ) );
  m_qt2katevi->insert( Qt::Key_Return, QString( "return" ) );
  m_qt2katevi->insert( Qt::Key_Enter, QString( "enter" ) );
  m_qt2katevi->insert( Qt::Key_Insert, QString( "insert" ) );
  m_qt2katevi->insert( Qt::Key_Delete, QString( "delete" ) );
  m_qt2katevi->insert( Qt::Key_Pause, QString( "pause" ) );
  m_qt2katevi->insert( Qt::Key_Print, QString( "print" ) );
  m_qt2katevi->insert( Qt::Key_SysReq, QString( "sysreq" ) );
  m_qt2katevi->insert( Qt::Key_Clear, QString( "clear" ) );
  m_qt2katevi->insert( Qt::Key_Home, QString( "home" ) );
  m_qt2katevi->insert( Qt::Key_End, QString( "end" ) );
  m_qt2katevi->insert( Qt::Key_Left, QString( "left" ) );
  m_qt2katevi->insert( Qt::Key_Up, QString( "up" ) );
  m_qt2katevi->insert( Qt::Key_Right, QString( "right" ) );
  m_qt2katevi->insert( Qt::Key_Down, QString( "down" ) );
  m_qt2katevi->insert( Qt::Key_PageUp, QString( "pageup" ) );
  m_qt2katevi->insert( Qt::Key_PageDown, QString( "pagedown" ) );
  m_qt2katevi->insert( Qt::Key_Shift, QString( "shift" ) );
  m_qt2katevi->insert( Qt::Key_Control, QString( "control" ) );
  m_qt2katevi->insert( Qt::Key_Meta, QString( "meta" ) );
  m_qt2katevi->insert( Qt::Key_Alt, QString( "alt" ) );
  m_qt2katevi->insert( Qt::Key_AltGr, QString( "altgr" ) );
  m_qt2katevi->insert( Qt::Key_CapsLock, QString( "capslock" ) );
  m_qt2katevi->insert( Qt::Key_NumLock, QString( "numlock" ) );
  m_qt2katevi->insert( Qt::Key_ScrollLock, QString( "scrolllock" ) );
  m_qt2katevi->insert( Qt::Key_F1, QString( "f1" ) );
  m_qt2katevi->insert( Qt::Key_F2, QString( "f2" ) );
  m_qt2katevi->insert( Qt::Key_F3, QString( "f3" ) );
  m_qt2katevi->insert( Qt::Key_F4, QString( "f4" ) );
  m_qt2katevi->insert( Qt::Key_F5, QString( "f5" ) );
  m_qt2katevi->insert( Qt::Key_F6, QString( "f6" ) );
  m_qt2katevi->insert( Qt::Key_F7, QString( "f7" ) );
  m_qt2katevi->insert( Qt::Key_F8, QString( "f8" ) );
  m_qt2katevi->insert( Qt::Key_F9, QString( "f9" ) );
  m_qt2katevi->insert( Qt::Key_F10, QString( "f10" ) );
  m_qt2katevi->insert( Qt::Key_F11, QString( "f11" ) );
  m_qt2katevi->insert( Qt::Key_F12, QString( "f12" ) );
  m_qt2katevi->insert( Qt::Key_F13, QString( "f13" ) );
  m_qt2katevi->insert( Qt::Key_F14, QString( "f14" ) );
  m_qt2katevi->insert( Qt::Key_F15, QString( "f15" ) );
  m_qt2katevi->insert( Qt::Key_F16, QString( "f16" ) );
  m_qt2katevi->insert( Qt::Key_F17, QString( "f17" ) );
  m_qt2katevi->insert( Qt::Key_F18, QString( "f18" ) );
  m_qt2katevi->insert( Qt::Key_F19, QString( "f19" ) );
  m_qt2katevi->insert( Qt::Key_F20, QString( "f20" ) );
  m_qt2katevi->insert( Qt::Key_F21, QString( "f21" ) );
  m_qt2katevi->insert( Qt::Key_F22, QString( "f22" ) );
  m_qt2katevi->insert( Qt::Key_F23, QString( "f23" ) );
  m_qt2katevi->insert( Qt::Key_F24, QString( "f24" ) );
  m_qt2katevi->insert( Qt::Key_F25, QString( "f25" ) );
  m_qt2katevi->insert( Qt::Key_F26, QString( "f26" ) );
  m_qt2katevi->insert( Qt::Key_F27, QString( "f27" ) );
  m_qt2katevi->insert( Qt::Key_F28, QString( "f28" ) );
  m_qt2katevi->insert( Qt::Key_F29, QString( "f29" ) );
  m_qt2katevi->insert( Qt::Key_F30, QString( "f30" ) );
  m_qt2katevi->insert( Qt::Key_F31, QString( "f31" ) );
  m_qt2katevi->insert( Qt::Key_F32, QString( "f32" ) );
  m_qt2katevi->insert( Qt::Key_F33, QString( "f33" ) );
  m_qt2katevi->insert( Qt::Key_F34, QString( "f34" ) );
  m_qt2katevi->insert( Qt::Key_F35, QString( "f35" ) );
  m_qt2katevi->insert( Qt::Key_Super_L, QString( "super_l" ) );
  m_qt2katevi->insert( Qt::Key_Super_R, QString( "super_r" ) );
  m_qt2katevi->insert( Qt::Key_Menu, QString( "menu" ) );
  m_qt2katevi->insert( Qt::Key_Hyper_L, QString( "hyper_l" ) );
  m_qt2katevi->insert( Qt::Key_Hyper_R, QString( "hyper_r" ) );
  m_qt2katevi->insert( Qt::Key_Help, QString( "help" ) );
  m_qt2katevi->insert( Qt::Key_Direction_L, QString( "direction_l" ) );
  m_qt2katevi->insert( Qt::Key_Direction_R, QString( "direction_r" ) );
  m_qt2katevi->insert( Qt::Key_Multi_key, QString( "multi_key" ) );
  m_qt2katevi->insert( Qt::Key_Codeinput, QString( "codeinput" ) );
  m_qt2katevi->insert( Qt::Key_SingleCandidate, QString( "singlecandidate" ) );
  m_qt2katevi->insert( Qt::Key_MultipleCandidate, QString( "multiplecandidate" ) );
  m_qt2katevi->insert( Qt::Key_PreviousCandidate, QString( "previouscandidate" ) );
  m_qt2katevi->insert( Qt::Key_Mode_switch, QString( "mode_switch" ) );
  m_qt2katevi->insert( Qt::Key_Kanji, QString( "kanji" ) );
  m_qt2katevi->insert( Qt::Key_Muhenkan, QString( "muhenkan" ) );
  m_qt2katevi->insert( Qt::Key_Henkan, QString( "henkan" ) );
  m_qt2katevi->insert( Qt::Key_Romaji, QString( "romaji" ) );
  m_qt2katevi->insert( Qt::Key_Hiragana, QString( "hiragana" ) );
  m_qt2katevi->insert( Qt::Key_Katakana, QString( "katakana" ) );
  m_qt2katevi->insert( Qt::Key_Hiragana_Katakana, QString( "hiragana_katakana" ) );
  m_qt2katevi->insert( Qt::Key_Zenkaku, QString( "zenkaku" ) );
  m_qt2katevi->insert( Qt::Key_Hankaku, QString( "hankaku" ) );
  m_qt2katevi->insert( Qt::Key_Zenkaku_Hankaku, QString( "zenkaku_hankaku" ) );
  m_qt2katevi->insert( Qt::Key_Touroku, QString( "touroku" ) );
  m_qt2katevi->insert( Qt::Key_Massyo, QString( "massyo" ) );
  m_qt2katevi->insert( Qt::Key_Kana_Lock, QString( "kana_lock" ) );
  m_qt2katevi->insert( Qt::Key_Kana_Shift, QString( "kana_shift" ) );
  m_qt2katevi->insert( Qt::Key_Eisu_Shift, QString( "eisu_shift" ) );
  m_qt2katevi->insert( Qt::Key_Eisu_toggle, QString( "eisu_toggle" ) );
  m_qt2katevi->insert( Qt::Key_Hangul, QString( "hangul" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Start, QString( "hangul_start" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_End, QString( "hangul_end" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Hanja, QString( "hangul_hanja" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Jamo, QString( "hangul_jamo" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Romaja, QString( "hangul_romaja" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Jeonja, QString( "hangul_jeonja" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Banja, QString( "hangul_banja" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_PreHanja, QString( "hangul_prehanja" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_PostHanja, QString( "hangul_posthanja" ) );
  m_qt2katevi->insert( Qt::Key_Hangul_Special, QString( "hangul_special" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Grave, QString( "dead_grave" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Acute, QString( "dead_acute" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Circumflex, QString( "dead_circumflex" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Tilde, QString( "dead_tilde" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Macron, QString( "dead_macron" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Breve, QString( "dead_breve" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Abovedot, QString( "dead_abovedot" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Diaeresis, QString( "dead_diaeresis" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Abovering, QString( "dead_abovering" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Doubleacute, QString( "dead_doubleacute" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Caron, QString( "dead_caron" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Cedilla, QString( "dead_cedilla" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Ogonek, QString( "dead_ogonek" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Iota, QString( "dead_iota" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Voiced_Sound, QString( "dead_voiced_sound" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Semivoiced_Sound, QString( "dead_semivoiced_sound" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Belowdot, QString( "dead_belowdot" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Hook, QString( "dead_hook" ) );
  m_qt2katevi->insert( Qt::Key_Dead_Horn, QString( "dead_horn" ) );
  m_qt2katevi->insert( Qt::Key_Back, QString( "back" ) );
  m_qt2katevi->insert( Qt::Key_Forward, QString( "forward" ) );
  m_qt2katevi->insert( Qt::Key_Stop, QString( "stop" ) );
  m_qt2katevi->insert( Qt::Key_Refresh, QString( "refresh" ) );
  m_qt2katevi->insert( Qt::Key_VolumeDown, QString( "volumedown" ) );
  m_qt2katevi->insert( Qt::Key_VolumeMute, QString( "volumemute" ) );
  m_qt2katevi->insert( Qt::Key_VolumeUp, QString( "volumeup" ) );
  m_qt2katevi->insert( Qt::Key_BassBoost, QString( "bassboost" ) );
  m_qt2katevi->insert( Qt::Key_BassUp, QString( "bassup" ) );
  m_qt2katevi->insert( Qt::Key_BassDown, QString( "bassdown" ) );
  m_qt2katevi->insert( Qt::Key_TrebleUp, QString( "trebleup" ) );
  m_qt2katevi->insert( Qt::Key_TrebleDown, QString( "trebledown" ) );
  m_qt2katevi->insert( Qt::Key_MediaPlay, QString( "mediaplay" ) );
  m_qt2katevi->insert( Qt::Key_MediaStop, QString( "mediastop" ) );
  m_qt2katevi->insert( Qt::Key_MediaPrevious, QString( "mediaprevious" ) );
  m_qt2katevi->insert( Qt::Key_MediaNext, QString( "medianext" ) );
  m_qt2katevi->insert( Qt::Key_MediaRecord, QString( "mediarecord" ) );
  m_qt2katevi->insert( Qt::Key_HomePage, QString( "homepage" ) );
  m_qt2katevi->insert( Qt::Key_Favorites, QString( "favorites" ) );
  m_qt2katevi->insert( Qt::Key_Search, QString( "search" ) );
  m_qt2katevi->insert( Qt::Key_Standby, QString( "standby" ) );
  m_qt2katevi->insert( Qt::Key_OpenUrl, QString( "openurl" ) );
  m_qt2katevi->insert( Qt::Key_LaunchMail, QString( "launchmail" ) );
  m_qt2katevi->insert( Qt::Key_LaunchMedia, QString( "launchmedia" ) );
  m_qt2katevi->insert( Qt::Key_Launch0, QString( "launch0" ) );
  m_qt2katevi->insert( Qt::Key_Launch1, QString( "launch1" ) );
  m_qt2katevi->insert( Qt::Key_Launch2, QString( "launch2" ) );
  m_qt2katevi->insert( Qt::Key_Launch3, QString( "launch3" ) );
  m_qt2katevi->insert( Qt::Key_Launch4, QString( "launch4" ) );
  m_qt2katevi->insert( Qt::Key_Launch5, QString( "launch5" ) );
  m_qt2katevi->insert( Qt::Key_Launch6, QString( "launch6" ) );
  m_qt2katevi->insert( Qt::Key_Launch7, QString( "launch7" ) );
  m_qt2katevi->insert( Qt::Key_Launch8, QString( "launch8" ) );
  m_qt2katevi->insert( Qt::Key_Launch9, QString( "launch9" ) );
  m_qt2katevi->insert( Qt::Key_LaunchA, QString( "launcha" ) );
  m_qt2katevi->insert( Qt::Key_LaunchB, QString( "launchb" ) );
  m_qt2katevi->insert( Qt::Key_LaunchC, QString( "launchc" ) );
  m_qt2katevi->insert( Qt::Key_LaunchD, QString( "launchd" ) );
  m_qt2katevi->insert( Qt::Key_LaunchE, QString( "launche" ) );
  m_qt2katevi->insert( Qt::Key_LaunchF, QString( "launchf" ) );
  m_qt2katevi->insert( Qt::Key_MediaLast, QString( "medialast" ) );
  m_qt2katevi->insert( Qt::Key_unknown, QString( "unknown" ) );
  m_qt2katevi->insert( Qt::Key_Call, QString( "call" ) );
  m_qt2katevi->insert( Qt::Key_Context1, QString( "context1" ) );
  m_qt2katevi->insert( Qt::Key_Context2, QString( "context2" ) );
  m_qt2katevi->insert( Qt::Key_Context3, QString( "context3" ) );
  m_qt2katevi->insert( Qt::Key_Context4, QString( "context4" ) );
  m_qt2katevi->insert( Qt::Key_Flip, QString( "flip" ) );
  m_qt2katevi->insert( Qt::Key_Hangup, QString( "hangup" ) );
  m_qt2katevi->insert( Qt::Key_No, QString( "no" ) );
  m_qt2katevi->insert( Qt::Key_Select, QString( "select" ) );
  m_qt2katevi->insert( Qt::Key_Yes, QString( "yes" ) );
  m_qt2katevi->insert( Qt::Key_Execute, QString( "execute" ) );
  m_qt2katevi->insert( Qt::Key_Printer, QString( "printer" ) );
  m_qt2katevi->insert( Qt::Key_Play, QString( "play" ) );
  m_qt2katevi->insert( Qt::Key_Sleep, QString( "sleep" ) );
  m_qt2katevi->insert( Qt::Key_Zoom, QString( "zoom" ) );
  m_qt2katevi->insert( Qt::Key_Cancel, QString( "cancel" ) );

  for (QHash<int, QString>::const_iterator i = m_qt2katevi->constBegin();
      i != m_qt2katevi->constEnd(); ++i) {
    m_katevi2qt->insert( i.value(), i.key() );
  }
  m_katevi2qt->insert( QString("cr"), Qt::Key_Enter);

  m_nameToKeyCode->insert( QString( "invalid" ), -1 );
  m_nameToKeyCode->insert( QString( "esc" ), 1 );
  m_nameToKeyCode->insert( QString( "tab" ), 2 );
  m_nameToKeyCode->insert( QString( "backtab" ), 3 );
  m_nameToKeyCode->insert( QString( "backspace" ), 4 );
  m_nameToKeyCode->insert( QString( "return" ), 5 );
  m_nameToKeyCode->insert( QString( "enter" ), 6 );
  m_nameToKeyCode->insert( QString( "insert" ), 7 );
  m_nameToKeyCode->insert( QString( "delete" ), 8 );
  m_nameToKeyCode->insert( QString( "pause" ), 9 );
  m_nameToKeyCode->insert( QString( "print" ), 10 );
  m_nameToKeyCode->insert( QString( "sysreq" ), 11 );
  m_nameToKeyCode->insert( QString( "clear" ), 12 );
  m_nameToKeyCode->insert( QString( "home" ), 13 );
  m_nameToKeyCode->insert( QString( "end" ), 14 );
  m_nameToKeyCode->insert( QString( "left" ), 15 );
  m_nameToKeyCode->insert( QString( "up" ), 16 );
  m_nameToKeyCode->insert( QString( "right" ), 17 );
  m_nameToKeyCode->insert( QString( "down" ), 18 );
  m_nameToKeyCode->insert( QString( "pageup" ), 19 );
  m_nameToKeyCode->insert( QString( "pagedown" ), 20 );
  m_nameToKeyCode->insert( QString( "shift" ), 21 );
  m_nameToKeyCode->insert( QString( "control" ), 22 );
  m_nameToKeyCode->insert( QString( "meta" ), 23 );
  m_nameToKeyCode->insert( QString( "alt" ), 24 );
  m_nameToKeyCode->insert( QString( "altgr" ), 25 );
  m_nameToKeyCode->insert( QString( "capslock" ), 26 );
  m_nameToKeyCode->insert( QString( "numlock" ), 27 );
  m_nameToKeyCode->insert( QString( "scrolllock" ), 28 );
  m_nameToKeyCode->insert( QString( "f1" ), 29 );
  m_nameToKeyCode->insert( QString( "f2" ), 30 );
  m_nameToKeyCode->insert( QString( "f3" ), 31 );
  m_nameToKeyCode->insert( QString( "f4" ), 32 );
  m_nameToKeyCode->insert( QString( "f5" ), 33 );
  m_nameToKeyCode->insert( QString( "f6" ), 34 );
  m_nameToKeyCode->insert( QString( "f7" ), 35 );
  m_nameToKeyCode->insert( QString( "f8" ), 36 );
  m_nameToKeyCode->insert( QString( "f9" ), 37 );
  m_nameToKeyCode->insert( QString( "f10" ), 38 );
  m_nameToKeyCode->insert( QString( "f11" ), 39 );
  m_nameToKeyCode->insert( QString( "f12" ), 40 );
  m_nameToKeyCode->insert( QString( "f13" ), 41 );
  m_nameToKeyCode->insert( QString( "f14" ), 42 );
  m_nameToKeyCode->insert( QString( "f15" ), 43 );
  m_nameToKeyCode->insert( QString( "f16" ), 44 );
  m_nameToKeyCode->insert( QString( "f17" ), 45 );
  m_nameToKeyCode->insert( QString( "f18" ), 46 );
  m_nameToKeyCode->insert( QString( "f19" ), 47 );
  m_nameToKeyCode->insert( QString( "f20" ), 48 );
  m_nameToKeyCode->insert( QString( "f21" ), 49 );
  m_nameToKeyCode->insert( QString( "f22" ), 50 );
  m_nameToKeyCode->insert( QString( "f23" ), 51 );
  m_nameToKeyCode->insert( QString( "f24" ), 52 );
  m_nameToKeyCode->insert( QString( "f25" ), 53 );
  m_nameToKeyCode->insert( QString( "f26" ), 54 );
  m_nameToKeyCode->insert( QString( "f27" ), 55 );
  m_nameToKeyCode->insert( QString( "f28" ), 56 );
  m_nameToKeyCode->insert( QString( "f29" ), 57 );
  m_nameToKeyCode->insert( QString( "f30" ), 58 );
  m_nameToKeyCode->insert( QString( "f31" ), 59 );
  m_nameToKeyCode->insert( QString( "f32" ), 60 );
  m_nameToKeyCode->insert( QString( "f33" ), 61 );
  m_nameToKeyCode->insert( QString( "f34" ), 62 );
  m_nameToKeyCode->insert( QString( "f35" ), 63 );
  m_nameToKeyCode->insert( QString( "super_l" ), 64 );
  m_nameToKeyCode->insert( QString( "super_r" ), 65 );
  m_nameToKeyCode->insert( QString( "menu" ), 66 );
  m_nameToKeyCode->insert( QString( "hyper_l" ), 67 );
  m_nameToKeyCode->insert( QString( "hyper_r" ), 68 );
  m_nameToKeyCode->insert( QString( "help" ), 69 );
  m_nameToKeyCode->insert( QString( "direction_l" ), 70 );
  m_nameToKeyCode->insert( QString( "direction_r" ), 71 );
  m_nameToKeyCode->insert( QString( "multi_key" ), 172 );
  m_nameToKeyCode->insert( QString( "codeinput" ), 173 );
  m_nameToKeyCode->insert( QString( "singlecandidate" ), 174 );
  m_nameToKeyCode->insert( QString( "multiplecandidate" ), 175 );
  m_nameToKeyCode->insert( QString( "previouscandidate" ), 176 );
  m_nameToKeyCode->insert( QString( "mode_switch" ), 177 );
  m_nameToKeyCode->insert( QString( "kanji" ), 178 );
  m_nameToKeyCode->insert( QString( "muhenkan" ), 179 );
  m_nameToKeyCode->insert( QString( "henkan" ), 180 );
  m_nameToKeyCode->insert( QString( "romaji" ), 181 );
  m_nameToKeyCode->insert( QString( "hiragana" ), 182 );
  m_nameToKeyCode->insert( QString( "katakana" ), 183 );
  m_nameToKeyCode->insert( QString( "hiragana_katakana" ), 184 );
  m_nameToKeyCode->insert( QString( "zenkaku" ), 185 );
  m_nameToKeyCode->insert( QString( "hankaku" ), 186 );
  m_nameToKeyCode->insert( QString( "zenkaku_hankaku" ), 187 );
  m_nameToKeyCode->insert( QString( "touroku" ), 188 );
  m_nameToKeyCode->insert( QString( "massyo" ), 189 );
  m_nameToKeyCode->insert( QString( "kana_lock" ), 190 );
  m_nameToKeyCode->insert( QString( "kana_shift" ), 191 );
  m_nameToKeyCode->insert( QString( "eisu_shift" ), 192 );
  m_nameToKeyCode->insert( QString( "eisu_toggle" ), 193 );
  m_nameToKeyCode->insert( QString( "hangul" ), 194 );
  m_nameToKeyCode->insert( QString( "hangul_start" ), 195 );
  m_nameToKeyCode->insert( QString( "hangul_end" ), 196 );
  m_nameToKeyCode->insert( QString( "hangul_hanja" ), 197 );
  m_nameToKeyCode->insert( QString( "hangul_jamo" ), 198 );
  m_nameToKeyCode->insert( QString( "hangul_romaja" ), 199 );
  m_nameToKeyCode->insert( QString( "hangul_jeonja" ), 200 );
  m_nameToKeyCode->insert( QString( "hangul_banja" ), 201 );
  m_nameToKeyCode->insert( QString( "hangul_prehanja" ), 202 );
  m_nameToKeyCode->insert( QString( "hangul_posthanja" ), 203 );
  m_nameToKeyCode->insert( QString( "hangul_special" ), 204 );
  m_nameToKeyCode->insert( QString( "dead_grave" ), 205 );
  m_nameToKeyCode->insert( QString( "dead_acute" ), 206 );
  m_nameToKeyCode->insert( QString( "dead_circumflex" ), 207 );
  m_nameToKeyCode->insert( QString( "dead_tilde" ), 208 );
  m_nameToKeyCode->insert( QString( "dead_macron" ), 209 );
  m_nameToKeyCode->insert( QString( "dead_breve" ), 210 );
  m_nameToKeyCode->insert( QString( "dead_abovedot" ), 211 );
  m_nameToKeyCode->insert( QString( "dead_diaeresis" ), 212 );
  m_nameToKeyCode->insert( QString( "dead_abovering" ), 213 );
  m_nameToKeyCode->insert( QString( "dead_doubleacute" ), 214 );
  m_nameToKeyCode->insert( QString( "dead_caron" ), 215 );
  m_nameToKeyCode->insert( QString( "dead_cedilla" ), 216 );
  m_nameToKeyCode->insert( QString( "dead_ogonek" ), 217 );
  m_nameToKeyCode->insert( QString( "dead_iota" ), 218 );
  m_nameToKeyCode->insert( QString( "dead_voiced_sound" ), 219 );
  m_nameToKeyCode->insert( QString( "dead_semivoiced_sound" ), 220 );
  m_nameToKeyCode->insert( QString( "dead_belowdot" ), 221 );
  m_nameToKeyCode->insert( QString( "dead_hook" ), 222 );
  m_nameToKeyCode->insert( QString( "dead_horn" ), 223 );
  m_nameToKeyCode->insert( QString( "back" ), 224 );
  m_nameToKeyCode->insert( QString( "forward" ), 225 );
  m_nameToKeyCode->insert( QString( "stop" ), 226 );
  m_nameToKeyCode->insert( QString( "refresh" ), 227 );
  m_nameToKeyCode->insert( QString( "volumedown" ), 228 );
  m_nameToKeyCode->insert( QString( "volumemute" ), 229 );
  m_nameToKeyCode->insert( QString( "volumeup" ), 230 );
  m_nameToKeyCode->insert( QString( "bassboost" ), 231 );
  m_nameToKeyCode->insert( QString( "bassup" ), 232 );
  m_nameToKeyCode->insert( QString( "bassdown" ), 233 );
  m_nameToKeyCode->insert( QString( "trebleup" ), 234 );
  m_nameToKeyCode->insert( QString( "trebledown" ), 235 );
  m_nameToKeyCode->insert( QString( "mediaplay" ), 236 );
  m_nameToKeyCode->insert( QString( "mediastop" ), 237 );
  m_nameToKeyCode->insert( QString( "mediaprevious" ), 238 );
  m_nameToKeyCode->insert( QString( "medianext" ), 239 );
  m_nameToKeyCode->insert( QString( "mediarecord" ), 240 );
  m_nameToKeyCode->insert( QString( "homepage" ), 241 );
  m_nameToKeyCode->insert( QString( "favorites" ), 242 );
  m_nameToKeyCode->insert( QString( "search" ), 243 );
  m_nameToKeyCode->insert( QString( "standby" ), 244 );
  m_nameToKeyCode->insert( QString( "openurl" ), 245 );
  m_nameToKeyCode->insert( QString( "launchmail" ), 246 );
  m_nameToKeyCode->insert( QString( "launchmedia" ), 247 );
  m_nameToKeyCode->insert( QString( "launch0" ), 248 );
  m_nameToKeyCode->insert( QString( "launch1" ), 249 );
  m_nameToKeyCode->insert( QString( "launch2" ), 250 );
  m_nameToKeyCode->insert( QString( "launch3" ), 251 );
  m_nameToKeyCode->insert( QString( "launch4" ), 252 );
  m_nameToKeyCode->insert( QString( "launch5" ), 253 );
  m_nameToKeyCode->insert( QString( "launch6" ), 254 );
  m_nameToKeyCode->insert( QString( "launch7" ), 255 );
  m_nameToKeyCode->insert( QString( "launch8" ), 256 );
  m_nameToKeyCode->insert( QString( "launch9" ), 257 );
  m_nameToKeyCode->insert( QString( "launcha" ), 258 );
  m_nameToKeyCode->insert( QString( "launchb" ), 259 );
  m_nameToKeyCode->insert( QString( "launchc" ), 260 );
  m_nameToKeyCode->insert( QString( "launchd" ), 261 );
  m_nameToKeyCode->insert( QString( "launche" ), 262 );
  m_nameToKeyCode->insert( QString( "launchf" ), 263 );
  m_nameToKeyCode->insert( QString( "medialast" ), 264 );
  m_nameToKeyCode->insert( QString( "unknown" ), 265 );
  m_nameToKeyCode->insert( QString( "call" ), 266 );
  m_nameToKeyCode->insert( QString( "context1" ), 267 );
  m_nameToKeyCode->insert( QString( "context2" ), 268 );
  m_nameToKeyCode->insert( QString( "context3" ), 269 );
  m_nameToKeyCode->insert( QString( "context4" ), 270 );
  m_nameToKeyCode->insert( QString( "flip" ), 271 );
  m_nameToKeyCode->insert( QString( "hangup" ), 272 );
  m_nameToKeyCode->insert( QString( "no" ), 273 );
  m_nameToKeyCode->insert( QString( "select" ), 274 );
  m_nameToKeyCode->insert( QString( "yes" ), 275 );
  m_nameToKeyCode->insert( QString( "execute" ), 276 );
  m_nameToKeyCode->insert( QString( "printer" ), 277 );
  m_nameToKeyCode->insert( QString( "play" ), 278 );
  m_nameToKeyCode->insert( QString( "sleep" ), 279 );
  m_nameToKeyCode->insert( QString( "zoom" ), 280 );
  m_nameToKeyCode->insert( QString( "cancel" ), 281 );

  m_nameToKeyCode->insert( QString( "a" ), 282 );
  m_nameToKeyCode->insert( QString( "b" ), 283 );
  m_nameToKeyCode->insert( QString( "c" ), 284 );
  m_nameToKeyCode->insert( QString( "d" ), 285 );
  m_nameToKeyCode->insert( QString( "e" ), 286 );
  m_nameToKeyCode->insert( QString( "f" ), 287 );
  m_nameToKeyCode->insert( QString( "g" ), 288 );
  m_nameToKeyCode->insert( QString( "h" ), 289 );
  m_nameToKeyCode->insert( QString( "i" ), 290 );
  m_nameToKeyCode->insert( QString( "j" ), 291 );
  m_nameToKeyCode->insert( QString( "k" ), 292 );
  m_nameToKeyCode->insert( QString( "l" ), 293 );
  m_nameToKeyCode->insert( QString( "m" ), 294 );
  m_nameToKeyCode->insert( QString( "n" ), 295 );
  m_nameToKeyCode->insert( QString( "o" ), 296 );
  m_nameToKeyCode->insert( QString( "p" ), 297 );
  m_nameToKeyCode->insert( QString( "q" ), 298 );
  m_nameToKeyCode->insert( QString( "r" ), 299 );
  m_nameToKeyCode->insert( QString( "s" ), 300 );
  m_nameToKeyCode->insert( QString( "t" ), 301 );
  m_nameToKeyCode->insert( QString( "u" ), 302 );
  m_nameToKeyCode->insert( QString( "v" ), 303 );
  m_nameToKeyCode->insert( QString( "w" ), 304 );
  m_nameToKeyCode->insert( QString( "x" ), 305 );
  m_nameToKeyCode->insert( QString( "y" ), 306 );
  m_nameToKeyCode->insert( QString( "z" ), 307 );
  m_nameToKeyCode->insert( QString( "`" ), 308 );
  m_nameToKeyCode->insert( QString( "!" ), 309 );
  m_nameToKeyCode->insert( QString( "\"" ), 310 );
  m_nameToKeyCode->insert( QString( "$" ), 311 );
  m_nameToKeyCode->insert( QString( "%" ), 312 );
  m_nameToKeyCode->insert( QString( "^" ), 313 );
  m_nameToKeyCode->insert( QString( "&" ), 314 );
  m_nameToKeyCode->insert( QString( "*" ), 315 );
  m_nameToKeyCode->insert( QString( "(" ), 316 );
  m_nameToKeyCode->insert( QString( ")" ), 317 );
  m_nameToKeyCode->insert( QString( "-" ), 318 );
  m_nameToKeyCode->insert( QString( "_" ), 319 );
  m_nameToKeyCode->insert( QString( "=" ), 320 );
  m_nameToKeyCode->insert( QString( "+" ), 321 );
  m_nameToKeyCode->insert( QString( "[" ), 322 );
  m_nameToKeyCode->insert( QString( "]" ), 323 );
  m_nameToKeyCode->insert( QString( "{" ), 324 );
  m_nameToKeyCode->insert( QString( "}" ), 325 );
  m_nameToKeyCode->insert( QString( ":" ), 326 );
  m_nameToKeyCode->insert( QString( ";" ), 327 );
  m_nameToKeyCode->insert( QString( "@" ), 328 );
  m_nameToKeyCode->insert( QString( "'" ), 329 );
  m_nameToKeyCode->insert( QString( "#" ), 330 );
  m_nameToKeyCode->insert( QString( "~" ), 331 );
  m_nameToKeyCode->insert( QString( "\\" ), 332 );
  m_nameToKeyCode->insert( QString( "|" ), 333 );
  m_nameToKeyCode->insert( QString( "," ), 334 );
  m_nameToKeyCode->insert( QString( "." ), 335 );
  //m_nameToKeyCode->insert( QString( ">" ), 336 );
  m_nameToKeyCode->insert( QString( "/" ), 337 );
  m_nameToKeyCode->insert( QString( "?" ), 338 );
  m_nameToKeyCode->insert( QString( " " ), 339 );
  //m_nameToKeyCode->insert( QString( "<" ), 341 );
  m_nameToKeyCode->insert( QString("0"), 340);
  m_nameToKeyCode->insert( QString("1"), 341);
  m_nameToKeyCode->insert( QString("2"), 342);
  m_nameToKeyCode->insert( QString("3"), 343);
  m_nameToKeyCode->insert( QString("4"), 344);
  m_nameToKeyCode->insert( QString("5"), 345);
  m_nameToKeyCode->insert( QString("6"), 346);
  m_nameToKeyCode->insert( QString("7"), 347);
  m_nameToKeyCode->insert( QString("8"), 348);
  m_nameToKeyCode->insert( QString("9"), 349);
  m_nameToKeyCode->insert( QString("cr"), 350);


  for (QHash<QString, int>::const_iterator i = m_nameToKeyCode->constBegin();
      i != m_nameToKeyCode->constEnd(); ++i) {
    m_keyCodeToName->insert( i.value(), i.key() );
  }
}

QString KateViKeyParser::qt2vi( int key ) const
{
  return ( m_qt2katevi->contains( key ) ? m_qt2katevi->value( key ) : "invalid" );
}

int KateViKeyParser::vi2qt( const QString &keypress ) const
{
  return ( m_katevi2qt->contains( keypress ) ? m_katevi2qt->value( keypress ) : -1 );
}


int KateViKeyParser::encoded2qt(const QString &keypress) const
{
    QString key = KateViKeyParser::self()->decodeKeySequence(keypress);
    if (key.length() > 2 && key[0] == QLatin1Char('<') &&
        key[key.length() - 1] == QLatin1Char('>')) {

        key = key.mid(1, key.length() - 2);
    }
    return (m_katevi2qt->contains(key) ? m_katevi2qt->value(key) : -1);
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
      if ( c == '>' ) {
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
        if ( keys.mid( i ).indexOf( '-' ) != -1 && keys.mid( i ).indexOf( '-' ) < keys.mid( i ).indexOf( '>' ) ) {
          // Perform something similar to a split on '-', except that we want to keep the occurrences of '-'
          // e.g. <c-s-a> will equate to the list of tokens "c-", "s-", "a".
          // A straight split on '-' would give us "c", "s", "a", in which case we lose the piece of information that
          // 'a' is just the 'a' key, not the 'alt' modifier.
          const QString untilClosing = keys.mid( i, keys.mid( i ).indexOf( '>' ) ).toLower();
          QStringList tokens;
          int currentPos = 0;
          int nextHypen = -1;
          while ((nextHypen = untilClosing.indexOf(QChar('-'), currentPos)) != -1)
          {
            tokens << untilClosing.mid(currentPos, nextHypen - currentPos + 1);
            currentPos = nextHypen + 1;
          }
          tokens << untilClosing.mid(currentPos);

          foreach ( const QString& str, tokens ) {
            if ( str == "s-" && ( keyCodeTemp & 0x01 ) != 0x1  )
              keyCodeTemp += 0x1;
            else if ( str == "c-" && ( keyCodeTemp & 0x02 ) != 0x2 )
              keyCodeTemp += 0x2;
            else if ( str == "a-" && ( keyCodeTemp & 0x04 ) != 0x4 )
              keyCodeTemp += 0x4;
            else if ( str == "m-" && ( keyCodeTemp & 0x08 ) != 0x8 )
              keyCodeTemp += 0x8;
            else {
              if ( m_nameToKeyCode->contains( str ) || ( str.length() == 1 && str.at( 0 ).isLetterOrNumber() ) ) {
                if ( m_nameToKeyCode->contains( str ) ) {
                  keyCodeTemp += m_nameToKeyCode->value( str ) * 0x10;
                } else {
                  keyCodeTemp += str.at( 0 ).unicode() * 0x10;
                }
              } else {
                int endOfBlock = keys.indexOf( '>' );
                if ( endOfBlock -= -1 ) {
                  endOfBlock = keys.length()-1;
                }
                encodedSequence.clear();
                encodedSequence.append( m_nameToKeyCode->value( "invalid" ) );
                break;
              }
            }
          }
        }
        else {
          QString str = keys.mid( i, keys.indexOf( '>', i )-i ).toLower();
          if ( keys.indexOf( '>', i ) != -1 && ( m_nameToKeyCode->contains( str ) || ( str.length() == 1 && str.at( 0 ).isLetterOrNumber() ) ) ) {
            if ( m_nameToKeyCode->contains( str ) ) {
              keyCodeTemp += m_nameToKeyCode->value( str ) * 0x10;
            } else {
              keyCodeTemp += str.at( 0 ).unicode() * 0x10;
            }
          } else {
            int endOfBlock = keys.indexOf( '>' );
            if ( endOfBlock -= -1 ) {
              endOfBlock = keys.length()-1;
            }
            encodedSequence.clear();
            keyCodeTemp = m_nameToKeyCode->value( "invalid" ) * 0x10;
          }

        }
        i += keys.mid( i, keys.mid( i ).indexOf( '>' ) ).length()-1;
      }
    }
    else {
      if ( c == '<' ) {
        // If there's no closing '>', or if there is an opening '<' before the next '>', interpret as a literal '<'
        // If we are <space>, encode as a literal " ".
        QString rest = keys.mid( i );
        if (rest.indexOf('>', 1) != -1 && rest.mid(1, rest.indexOf('>', 1) - 1) == "space")
        {
          encodedSequence.append(" ");
          i += rest.indexOf('>', 1);
          continue;
        }
        else if ( rest.indexOf( '>', 1 ) == -1 || ( rest.indexOf( '<', 1 ) < rest.indexOf( '>', 1 ) && rest.indexOf( '<', 1 ) != -1 ) ) {
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
      ret.append( '<' );

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
      ret.append( '>' );
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
  //if ( m_altGrPressed ) {
  if ( mods & Qt::GroupSwitchModifier ) {
    return ( !text.isEmpty() ) ? text.at(0) : QChar();
  }

  if ( text.isEmpty() || ( text.length() == 1 && text.at(0) < 0x20 ) || keyCode == Qt::Key_Delete
      || ( mods != Qt::NoModifier && mods != Qt::ShiftModifier && mods != Qt::KeypadModifier) ) {
    QString keyPress;

    keyPress.append( '<' );
    keyPress.append( ( mods & Qt::ShiftModifier ) ? "s-" : "" );
    keyPress.append( ( mods & Qt::ControlModifier ) ? "c-" : "" );
    keyPress.append( ( mods & Qt::AltModifier ) ? "a-" : "" );
    keyPress.append( ( mods & Qt::MetaModifier ) ? "m-" : "" );
    keyPress.append( keyCode <= 0xFF ? QChar( keyCode ) : qt2vi( keyCode ) );
    keyPress.append( '>' );

    return encodeKeySequence( keyPress ).at( 0 );
  } else {
    return text.at(0);
  }
}
