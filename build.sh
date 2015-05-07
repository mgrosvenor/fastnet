#! /bin/bash

set -o nounset
set -o errexit

if [[ "$#" -lt 8 ]] 
then
   echo "Not enough arguments" 
   exit 1
fi 

PTYPE=$1
USE_ETHER=$2
USE_NBO=$3
DO_COPY=$4
READ_DATA=$5
DTYPE=$6
PSIZE=$7
PREFETCH=$8

DEFINES="-Dexterndefs -DPTYPE=\"$PTYPE\" -DUSE_ETHER=\"$USE_ETHER\" -DUSE_NBO=\"$USE_NBO\" -DREAD_DATA=\"$READ_DATA\" -DDO_COPY=\"$DO_COPY\" -DDTYPE=\"$DTYPE\" -DPSIZE=\"$PSIZE\" -DDO_PREFETCH=\"$PREFETCH\""

CFLAGS="-g -Ideps -Ideps -D_XOPEN_SOURCE=700 -D_BSD_SOURCE -std=c11 -Werror -Wall -Wextra -pedantic -Wno-missing-field-initializers"
LINKFLAGS="-g -Ldeps/chaste -lrt"
CAKECONFIG=cake.conf

SRC=src/fast_net.c

build/cake/cake $SRC --append-CFLAGS="$CFLAGS" --append-CFLAGS="$DEFINES" --LINKFLAGS="$LINKFLAGS" --variant=release
