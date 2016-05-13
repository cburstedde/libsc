#! /bin/sh

# Maybe it's best not to use this file, or to write it better.
# It will certainly do bad things, such as changing the left hand side of an
# equal sign and not the right side.

echo 1>&2 "You should probably not use this script as is; see the source."

FILE="$1"

if test ! -f "$FILE" ; then
	echo "Please specify file argument"
	exit 1
fi

perl -p -i \
	-e 'next if /^\s*dnl/;' \
	-e 's,test\s+"\$,test "x\$,;' \
	-e 's,=\s+"?x?(yes|no|unknown|disable)"?,= x\1,;' \
	"$FILE"

