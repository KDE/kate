# This file is part of PyKDE4.
#
# Copyright (C) 2012, 2013 Shaheed Haque <srhaque@theiet.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#
# The .sip files being built here are generated using MetaSIP from
# http://www.riverbankcomputing.com/hg/metasip. This is a relatively new tool
# and is typically not packaged by distros. A summary of the steps needed to
# get it working are at http://python.6.n6.nabble.com/ANN-MetaSIP-v1-0-Released-tp4715818p4991287.html.
# The result is a MetaSIP .msp project file, from which MetaSIP's CLI
# can be used to generate the .sip files. The MetaSIP GUI is used to create
# the .msp file:
#
# msip <whatever>.msp
#
# To recreate the .sip files, cd to this directory and then:
#
# msip -g . <whatever>.msp
#
# For now, the .msp file and the .sip files are checked into this directory,
# and the <whatever> subdirectory respectively.
#

include(SIPMacros)
include(KDE4Defaults)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
    # Okteta
    /usr/include/okteta1)

set(SIP_INCLUDES ${PYQT4_SIP_DIR} ${PYKDE4_SIP_DIR} ./okteta)
set(SIP_CONCAT_PARTS 1)
set(SIP_TAGS ALL WS_X11 ${PYQT4_VERSION_TAG})

if(PYQT4_VERSION LESS "040905")
    set(SIP_EXTRA_OPTIONS -g)
else (PYQT4_VERSION LESS "040905")
    # Disable QVector<int> for newer PyQt
    set(SIP_EXTRA_OPTIONS -g -x PyKDE_QVector)
endif(PYQT4_VERSION LESS "040905")

add_sip_python_module(PyKate4.okteta ./okteta/oktetamod.sip ${KDE4_KDEUI_LIBS} okteta1core okteta1gui)

install(FILES
    ./__init__.py
    DESTINATION ${PYTHON_SITE_PACKAGES_INSTALL_DIR}/PyKate4/)
