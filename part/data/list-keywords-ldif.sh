#!/bin/sh

list_keywords=./list-keywords-ldif.awk
schemas="$@"

echo "		<list name=\"attributetypes\">"
cat $schemas | $list_keywords | grep "attributetype" | grep -v '^#' | sed -e "s/'//g" | sort -u | awk '{print "			<item>"$2"</item>"}'
echo "		</list>"
echo "		<list name=\"objectclasses\">"
cat $schemas | $list_keywords | grep "objectclass" | grep -v '^#' | sed -e "s/'//g" | sort -u | awk '{print "			<item>"$2"</item>"}'
echo "		</list>"

