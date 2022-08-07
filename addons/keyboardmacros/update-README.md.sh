#!/bin/bash
# This script uses pandoc to update the local README.md file with the content from Kate's docbook documentation.

cd $(dirname "$(readlink -f "$BASH_SOURCE")")

sed -n '/^<sect1 id="kate-application-plugin-keyboardmacros">$/,/^<\/sect1>$/p' ../../doc/kate/plugins.docbook | \
    pandoc -f docbook -t markdown -s - -o - | \
    sed 's/{.\(menuchoice\|keycombo\)}//;s/\s{#.\+}$//' | \
    sed 's/CTRL/Ctrl+/;s/SHIFT/Shift+/;s/ALT/Alt+/;s/++/+/g;s/(\[/[`/;s/\])/`]/' | \
    cat > README.md

cd - >/dev/null
