# Build it on Mac howto in script form
# uses a patched macdeployqt, see https://bugreports.qt.io/browse/QTBUG-48836
# copy the patched one over the one in your Qt

# errors fatal
set -e

# set build type, default is release for bundle creation
export BUILD_TYPE=Release
#export BUILD_TYPE=Debug

# steps to build kate on make

# install Qt 5.6.1
# install CMake 3.3.2

# set path to Qt 5.6.1 stuff
export QTDIR=~/Qt5.6.1/5.6/clang_64
export PATH=$QTDIR/bin:/Applications/CMake.app/Contents/bin:$PATH

# remember some dirs
export PREFIX=`pwd`/usr
export BUILD=`pwd`/build
export SRC=`pwd`/src

mkdir -p src
cd src

# install gettext, needed for ki18n
curl http://ftp.gnu.org/pub/gnu/gettext/gettext-0.19.6.tar.gz > gettext-0.19.6.tar.gz
tar -xvzf gettext-0.19.6.tar.gz
pushd gettext-0.19.6
./configure --prefix=$PREFIX
make install
popd

cd ..

# build helper
function build_framework
{ (
    # errors fatal
    set -e

    # framework
    FRAMEWORK=$1

    # clone if not there
    mkdir -p $SRC
    cd $SRC
    if ( test -d $FRAMEWORK )
    then
        echo "$FRAMEWORK already cloned"
    else
        git clone kde:$FRAMEWORK
    fi

    # create build dir
    mkdir -p $BUILD/$FRAMEWORK

    # go there
    cd $BUILD/$FRAMEWORK

    # cmake it
    cmake $SRC/$FRAMEWORK -DCMAKE_INSTALL_PREFIX:PATH=$PREFIX -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_TESTING:BOOL=OFF $2

    # make
    make -j2

    # install
    make install
) }

# get & build stuff

build_framework extra-cmake-modules
build_framework kconfig
build_framework kguiaddons
build_framework ki18n
build_framework kitemviews
build_framework sonnet
build_framework kwindowsystem
build_framework kwidgetsaddons
build_framework kcompletion
build_framework kdbusaddons
build_framework karchive
build_framework kcoreaddons
build_framework kjobwidgets
build_framework kcrash
build_framework kservice
build_framework kcodecs
build_framework kauth
build_framework kconfigwidgets
build_framework kiconthemes
build_framework ktextwidgets
build_framework kglobalaccel
build_framework kxmlgui
build_framework kbookmarks
build_framework solid
build_framework kio
build_framework kparts
build_framework kitemmodels
build_framework threadweaver
build_framework attica
build_framework knewstuff
build_framework ktexteditor
build_framework breeze-icons -DBINARY_ICONS_RESOURCE=1

# clear old bundles + dmg files
rm -rf /Applications/KDE/kate.*
rm -rf /Applications/KDE/kwrite.*

build_framework kate

# deploy qt plugins as extra plugins, too, as e.g. iconengine will be missing otherwise ;)
for i in kwrite kate; do
    echo $i;
    cp -f $PREFIX/share/icons/breeze/breeze-icons.rcc /Applications/KDE/$i.app/Contents/Resources
    cp -f $PREFIX/lib/libexec/kf5/kioslave /Applications/KDE/$i.app/Contents/MacOS
    cp -f $PREFIX/lib/libexec/kf5/kio_http_cache_cleaner /Applications/KDE/$i.app/Contents/MacOS
    
    # deploy
    macdeployqt /Applications/KDE/$i.app -executable=/Applications/KDE/$i.app/Contents/MacOS/kioslave -executable=/Applications/KDE/$i.app/Contents/MacOS/kio_http_cache_cleaner -extra-plugins=$PREFIX/lib/plugins -extra-plugins=$QTDIR/plugins
    
    # remove stuff we don't need aka like
    rm -rf /Applications/KDE/$i.app/Contents/Frameworks/Qt3DCore.framework
    rm -rf /Applications/KDE/$i.app/Contents/Frameworks/Qt3DRender.framework
    rm -rf /Applications/KDE/$i.app/Contents/Frameworks/QtLocation.framework
    rm -rf /Applications/KDE/$i.app/Contents/Frameworks/QtMultimedia.framework
    rm -rf /Applications/KDE/$i.app/Contents/Frameworks/QtMultimediaWidgets.framework
    rm -rf /Applications/KDE/$i.app/Contents/Frameworks/QtPositioning.framework
    rm -rf /Applications/KDE/$i.app/Contents/Frameworks/QtSerialBus.framework
    rm -rf /Applications/KDE/$i.app/Contents/Frameworks/QtSerialPort.framework
    rm -rf /Applications/KDE/$i.app/Contents/Frameworks/QtWebChannel.framework
    rm -rf /Applications/KDE/$i.app/Contents/Frameworks/QtWebEngineCore.framework
    rm -rf /Applications/KDE/$i.app/Contents/Frameworks/QtWebEngineWidgets.framework
    rm -rf /Applications/KDE/$i.app/Contents/PlugIns/qmltooling
    rm -rf /Applications/KDE/$i.app/Contents/PlugIns/position
    rm -rf /Applications/KDE/$i.app/Contents/PlugIns/playlistformats
    rm -rf /Applications/KDE/$i.app/Contents/PlugIns/mediaservice
    rm -rf /Applications/KDE/$i.app/Contents/PlugIns/geoservices
    rm -rf /Applications/KDE/$i.app/Contents/PlugIns/canbus
    rm -rf /Applications/KDE/$i.app/Contents/PlugIns/sceneparsers
    rm -rf /Applications/KDE/$i.app/Contents/PlugIns/sensorgestures
    rm -rf /Applications/KDE/$i.app/Contents/PlugIns/sensors
    
    # create the final disk image
    macdeployqt /Applications/KDE/$i.app -dmg
done
