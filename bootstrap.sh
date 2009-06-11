#!/bin/sh

run() {
	echo "Running ${@}...";
	$@ || exit;
}

AUXDIR=`autom4te -l Autoconf -t AC_CONFIG_AUX_DIR:\\$1 configure.ac`
if [ -n "$AUXDIR" ]; then
	echo "Creating auxiliary build tools directory ($AUXDIR)..."
	mkdir -p $AUXDIR
fi

run aclocal
run libtoolize --copy --force --ltdl
run autoconf
run autoheader
run automake --gnu --add-missing --copy
