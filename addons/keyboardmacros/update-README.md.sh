#!/usr/bin/env bash
# This script uses pandoc to update the local README.md file with the content from Kate's docbook documentation.

cd $(dirname "$(readlink -f "$BASH_SOURCE")")

sed -n '/^<sect1 id="kate-application-plugin-keyboardmacros">$/,/^<\/sect1>$/p' ../../doc/kate/plugins.docbook | \
    pandoc -f docbook -t markdown -s - -o - | \
    sed 's/{.\(menuchoice\|keycombo\)}//;s/\s{#.\+}$//' | \
    sed 's/CTRL/Ctrl+/;s/SHIFT/Shift+/;s/ALT/Alt+/;s/++/+/g;s/(\[/[`/;s/\])/`]/' | \
    cat > README.md

cd - >/dev/null

##### BEGIN git pre-commit hook script
###
### #!/usr/bin/env bash
###
### UPDATE_SCRIPT="$(dirname $(git rev-parse --git-common-dir))"/addons/keyboardmacros/update-README.md.sh
###
### test -f "$UPDATE_SCRIPT" || exit 0
###
### TMP=$(mktemp)
### README="$(dirname $(git rev-parse --git-common-dir))"/addons/keyboardmacros/README.md
### cp "$README" "$TMP"
### source "$UPDATE_SCRIPT"
###
### if ! diff -q "$README" "$TMP" &>/dev/null; then
###     echo "Warning: README.md has been updated!"
###     rm "$TMP"
###     exit 1
### fi
### rm "$TMP"
### exit 0
###
##### END
