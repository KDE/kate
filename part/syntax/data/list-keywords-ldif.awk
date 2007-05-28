#!/bin/awk -f

function printName (name)
{
	if ((name != "") && (name != "(") && (name != ")")) print type, name;
}

function printNames (str)
{
	s = substr(str,index(str,"NAME")+4);
	split (s, a, " ");
#	for (i=0; i<length(a); i++) {
#		print type,a[i]
#	}
	# ATTENTION: The loop above does not work for some strange reason.
	# The following statements imitate it for a limited amount of elements.
	# If you happen to have more then the ones given here, you have to extend it!
	printName(a[0]);
	printName(a[1]);
	printName(a[2]);
	printName(a[3]);
}

BEGIN {
	type="";
}

/attributetype/ {
	type="attributetype";
}

/objectclass/ {
	type="objectclass";
}

/ NAME / {
	printNames($0);
}
