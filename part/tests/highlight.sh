#! /bin/sh
# This is a test script for the Katepart Bash Syntax Highlighting by
#	Wilbert Berendsen.  This is not runnable !!!


# The highlighting recognizes basic types of input, and has a few special cases that
# are all in FindCommands.  The main objective is to create really proper nesting of
# (multiline) strings, variables, expressions, etc.

# ============== Tests: ===============

# basic types:
'single quoted string'
"double quoted string"
$'string with esc\apes\x0din it'
$"string meant to be translated"


# brace expansion
mv my_file.{JPG,jpg}


# variable substitution:
$filename.ext
${filename}_ext
text${array[$subscript]}.text
text${array["string"]}.text
${!prefix*}
${!redir}
short are $_, $$, $?, ${@}, etc.
${variable//a/d}
${1:-default}		# TODO


# expression subst:
$(( cd << ed + 1 ))


# command subst:
$(ls -l)
echo `cat myfile`


# file subst:
$(<$filename)
$(</path/to/myfile)


# process subst:
sort <(show_labels) | sed 's/a/bg' > my_file.txt


# All substitutions also work in strings:
"subst ${in}side string"  'not $inside this ofcourse'
"The result is $(( $a + $b )). Thanks!"


# keywords are black, builtins dark purple and common commands lighter purple
set
exit
login


# Other colorings:
error() {
	cat /usr/bin/lesspipe.sh
	runscript >& redir.bak
	exec 3>&4
}


# do - done make code blocks
while [ $p -lt $q ] 
do
	chown 0644 $file.$p
done


# braces as well
run_prog | sort -u |
{
	echo Header
	while read a b d
	do
		echo $a/$b/$c
	done
	echo Footer
}


# Any constructions can be nested:
echo "A long string with $(
	if [ $count -gt 100 ] ; then
		echo "much"
	else
		echo "not much"
	fi ) substitutions." ;


# Even the case construct is correctly folded:
test -f blaat &&
(	do_something
	case $p in
		*bak)
			do_bak $p
			;;
		*)
			dont_bak $p
			;;
	esac
) # despite the extra parentheses in the case construction.


# variable assignments:
DIR=/dev
p=`ls`
LC_ALL="nl" dcop 'kate*'
_VAR=val
usage="$0 -- version $VERSION
Multiple lines of output
can be possible."


# Some commands expect variable names, these are colored correctly:
export PATH=/my/bin:$PATH BLAAT
export A B D
local p=3 x=$1 y='\'
read x y z 	# TODO
unset B


# The let builtin has special characteristics:
let  i=i+1	# TODO don't eat whole line.
make i=i+1


# options are recoqnized:
zip -f=file.zip
./configure  --destdir=/usr
make  destdir=/usr/
