#!/bin/env bash
# kate: tab-width 4; indent-mode normal;

export LC_ALL=C

# need cmake
CMAKE="$(type -P cmake)"
[ -x "$CMAKE" ] || exit 1
echo "found cmake... $CMAKE"

t=.tmp_cmake$$

# Get cmake version
CMAKE_VERSION="$("$CMAKE" --help | sed -n '1p')"

count() {
	wc -l $t.$1 | awk '{print $1}'
}

# Extract before/after command list
sed -n -e '/<list.*"commands"/{p;q}' -e 'p' cmake.xml > $t.1
sed -e '/<\/list\s*>/ba' -e 'd' -e ':a' -e 'n;ba' cmake.xml > $t._2
sed -n -e '/<list.*"special_args"/{p;q}' -e 'p' $t._2 > $t.2
sed "1,$(wc -l < $t.2)d" $t._2 | sed -e '/<\/list\s*>/ba' -e 'd' -e ':a' -e 'n;ba' > $t._3
sed -n -e '/<list.*"properties"/{p;q}' -e 'p' $t._3 > $t.3
sed "1,$(wc -l < $t.3)d" $t._3 | sed -e '/<\/list\s*>/ba' -e 'd' -e ':a' -e 'n;ba' > $t.4

cmake --help-command-list | sed '1d' | sort > $t.commands
echo "$(count commands) commands"

extract_args() {
	sed -e '/'"$1"'(/ba' \
	    -e 'd' \
	    -e ':a' \
	    -e '/)/{s/^.*(\(.*\)).*$/\1/p;d}' \
	    -e 'N;s/\n/ /;ba' | \
	sed -e 's/[][]//g' -e 's/|\| \+/\n/g' | \
	sed -n '/^[[:upper:][:digit:]_]\+$/p' >> $t.args
}

while read COMMAND ; do
	"$CMAKE" --help-command $COMMAND | extract_args $COMMAND
done < $t.commands
sort -u $t.args > $t.argsu
echo "$(count args) arguments, $(count argsu) unique"
cmake --help-property-list | sed -e '1d' -e '/[<>]/d' | sort -u > $t.props
echo "$(count props) properties"

# Generate new .xml
{
	cat $t.1
	echo "      <!-- generated list -->"
	sed 's!.*!      <item> & </item>!' $t.commands
	cat $t.2
	echo "      <!-- generated list -->"
	sed 's!.*!      <item> & </item>!' $t.argsu
	cat $t.3
	echo "      <!-- generated list -->"
	sed 's!.*!      <item> & </item>!' $t.props
	cat $t.4
} > cmake.xml

rm -f $t.*
