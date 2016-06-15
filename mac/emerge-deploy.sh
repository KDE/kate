# Build it on Mac howto in script form
# but be aware, some frameworks need patching to have this working

# errors fatal
set -e

# call patched macdeployqt for both bundles
# that needs patch from https://bugreports.qt.io/browse/QTBUG-48836
# deploy qt plugins as extra plugins, too, as e.g. iconengine will be missing otherwise ;)
for i in kwrite kate; do
    echo $i;
    cp -f bin/data/icontheme.rcc Applications/KDE/$i.app/Contents/Resources/icontheme.rcc
    cp -f lib/libexec/kf5/kioslave Applications/KDE/$i.app/Contents/MacOS
    cp -f lib/libexec/kf5/kio_http_cache_cleaner Applications/KDE/$i.app/Contents/MacOS

    # deploy
    macdeployqt $PWD/Applications/KDE/$i.app -executable=$PWD/Applications/KDE/$i.app/Contents/MacOS/kioslave -executable=$PWD/Applications/KDE/$i.app/Contents/MacOS/kio_http_cache_cleaner -extra-plugins=$PWD/lib/plugins -extra-plugins=`qtpaths --install-prefix`/plugins

    # remove stuff we don't need aka like
    rm -rf Applications/KDE/$i.app/Contents/Frameworks/Qt3DCore.framework
    rm -rf Applications/KDE/$i.app/Contents/Frameworks/Qt3DRender.framework
    rm -rf Applications/KDE/$i.app/Contents/Frameworks/QtLocation.framework
    rm -rf Applications/KDE/$i.app/Contents/Frameworks/QtMultimedia.framework
    rm -rf Applications/KDE/$i.app/Contents/Frameworks/QtMultimediaWidgets.framework
    rm -rf Applications/KDE/$i.app/Contents/Frameworks/QtPositioning.framework
    rm -rf Applications/KDE/$i.app/Contents/Frameworks/QtSerialBus.framework
    rm -rf Applications/KDE/$i.app/Contents/Frameworks/QtSerialPort.framework
    rm -rf Applications/KDE/$i.app/Contents/Frameworks/QtWebChannel.framework
    rm -rf Applications/KDE/$i.app/Contents/Frameworks/QtWebEngineCore.framework
    rm -rf Applications/KDE/$i.app/Contents/Frameworks/QtWebEngineWidgets.framework
    rm -rf Applications/KDE/$i.app/Contents/PlugIns/qmltooling
    rm -rf Applications/KDE/$i.app/Contents/PlugIns/position
    rm -rf Applications/KDE/$i.app/Contents/PlugIns/playlistformats
    rm -rf Applications/KDE/$i.app/Contents/PlugIns/mediaservice
    rm -rf Applications/KDE/$i.app/Contents/PlugIns/geoservices
    rm -rf Applications/KDE/$i.app/Contents/PlugIns/canbus
    rm -rf Applications/KDE/$i.app/Contents/PlugIns/sceneparsers
    rm -rf Applications/KDE/$i.app/Contents/PlugIns/sensorgestures
    rm -rf Applications/KDE/$i.app/Contents/PlugIns/sensors

    # create the final disk image
    macdeployqt Applications/KDE/$i.app -dmg
done
