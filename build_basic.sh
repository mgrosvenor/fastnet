#! /bin/bash

set -o nounset
set -o errexit

DEFINES=""
CFLAGS="-g -Ideps -Ideps -D_XOPEN_SOURCE=700 -D_BSD_SOURCE -std=c11 -Werror -Wall -Wextra -pedantic -Wno-missing-field-initializers"
#CFLAGS="-g -Ideps -Ideps -std=c11 -Werror -Wall -Wextra -pedantic -Wno-missing-field-initializers"
#LINKFLAGS="-g -Ldeps/chaste "
LINKFLAGS="-g -Ldeps/chaste -lrt"
CAKECONFIG=cake.conf

SRC=src/fast_net.c

build/cake/cake $SRC --append-CFLAGS="$CFLAGS" --append-CFLAGS="$DEFINES" --LINKFLAGS="$LINKFLAGS" --variant=release
