#! /bin/sh

FILE="$1"

if test ! -f "$FILE" ; then
	echo "Please specify file argument"
	exit 1
fi

grep -P -v '^\s*dnl' "$FILE" | egrep \
	'("x?(no|yes)|test\s+"?\$|test.+-(o|a)|!\s+test|test\s+x|"$x)'
