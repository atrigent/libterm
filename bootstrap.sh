#!/bin/sh

AUXDIR=`autom4te -l Autoconf -t AC_CONFIG_AUX_DIR:\\$1 configure.ac`
if [ -n "$AUXDIR" ]; then
	echo "Creating auxiliary build tools directory ($AUXDIR)..."
	mkdir -p $AUXDIR
fi

echo Running aclocal...
aclocal || exit

echo Running libtoolize --copy --force...
libtoolize --copy --force || exit

echo Running autoconf...
autoconf || exit

echo Running autoheader...
autoheader || exit

echo Running automake --gnu --add-missing --copy...
automake --gnu --add-missing --copy
