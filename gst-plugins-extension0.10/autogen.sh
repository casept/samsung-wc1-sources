#!/bin/sh
# you can either set the environment variables AUTOCONF and AUTOMAKE
# to the right versions, or leave them unset and get the RedHat 7.3 defaults

aclocal -I m4 -I common/m4
libtoolize --copy --force
autoheader
autoconf
automake --add-missing --copy --foreign

echo "Now type 'make' to compile $package."

