#!/bin/sh
# Extract messages for ACP Client plugin

exrc="acpclientplugin.cpp acpclientpluginview.cpp acpclientconfigpage.cpp acpserverdialog.cpp acpclientserver.cpp acpclientservermanager.cpp acpclientprotocol.cpp"

extractrc "ui.rc" >> rc.cpp
extractrc "acpconfigwidget.ui" >> rc.cpp
extractrc "acpserverdialog.ui" >> rc.cpp

xgettext --from-code=UTF-8 -kde -ki18n -ktr2i18n -kI18N_NOOP -kI18N_NOOP2 -kalreadyTranslatedText \
    -kalreadyTranslatedTextPlural -kki18n -kki18n: -kki18nc -kki18nc: \
    -kki18np -kki18np: -kki18nq -kki18nq: -ktranslate -o $podir/acpclient.pot \
    rc.cpp $exrc