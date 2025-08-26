#!/bin/bash
# usage:
# run the following to do initial setup:
#     ./build_flatpak.sh setup
# then run the following after making changes to build and run kate:
#     ./build_flatpak.sh
# if you need to reconfigure cmake, then run:
#     ./build_flatpak.sh cmake
# to run tests do:
#     ./build_flatpak.sh test
set -e

SCRIPT_PATH="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
FLATPAK_BUILD=$SCRIPT_PATH/build_fp
KATE_BUILD=$FLATPAK_BUILD/build

mkdir -p $KATE_BUILD # create a build dir inside flatpak build dir

DO_SETUP=
DO_CMAKE=
DO_TEST=
if [[ "$1" == "setup" ]]; then DO_SETUP="true"; DO_CMAKE="true"; fi
if [[ "$1" == "cmake" ]]; then DO_CMAKE="true"; fi
if [[ "$1" == "test" ]]; then DO_TEST="true"; fi

# Initial setup
if [[ -n "$DO_SETUP" ]]; then
    flatpak-builder build_fp --user --install-deps-from=flathub --force-clean --install ./.flatpak-manifest.json --stop-at=kate
fi

# cmake step
if [[ -n "$DO_CMAKE" ]]; then
    flatpak build $FLATPAK_BUILD cmake -S $SCRIPT_PATH -B $KATE_BUILD -GNinja
fi

# build step
flatpak build $FLATPAK_BUILD ninja -C $KATE_BUILD

# run tests or kate
if [[ -n "$DO_TEST" ]]; then
    flatpak build $FLATPAK_BUILD ctest --test-dir $KATE_BUILD
else
    # run kate
    flatpak build --with-appdir --allow=devel --bind-mount=/run/host/fonts=/usr/share/fonts --device=dri --filesystem=host --share=ipc --socket=cups --socket=fallback-x11 --socket=wayland --system-talk-name=org.freedesktop.UDisks2 --talk-name=org.freedesktop.Flatpak --talk-name=org.freedesktop.portal.* --talk-name=org.a11y.Bus --env=COLORTERM=$COLORTERM --env=LANG=$LANG --env=XDG_CURRENT_DESKTOP=$XDG_CURRENT_DESKTOP --env=XDG_SEAT=$XDG_SEAT --env=XDG_SESSION_DESKTOP=$XDG_SESSION_DESKTOP --env=XDG_SESSION_ID=$XDG_SESSION_ID --env=XDG_SESSION_TYPE=$XDG_SESSION_TYPE --env=XDG_VTNR=$XDG_VTNR $FLATPAK_BUILD $KATE_BUILD/bin/kate -b
fi

