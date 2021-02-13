#!/bin/sh
#
# generate Wlib wrapper C-code for W-Lua
#
# For this you need 'tolua' utility from:
#	http://www.tecgraf.puc-rio.br/~celes/tolua/

package='wlib'

echo "cleaning Wlib.h for conversion..."

echo '$#include "Wlib.h"' > $package.pkg
echo '$#include "wwslua.h"' >> $package.pkg
echo "module wlib {" >> $package.pkg

# copy Wlib.h so that:
# - everything is inside 'wlib' module
# - following stuff is removed:
#   - ifdef/ifndef/endif/undef
#   - includes, string defines and macros
#   - protos with function pointer args
#   - structures (as they have members with same names -> tolua bugs)
# - defined unsigned types are expanded
grep -v \
  -e ifdef -e ifndef -e endif -e undef \
  -e include -e WLIB_H -e W_SEL_ -e 'define[^/]*(' \
  -e convertFunction -e unsigned \
  ../lib/Wlib.h |
sed -e 's/ulong/unsigned long/g' -e 's/ushort/unsigned short/g' -e 's/uchar/unsigned char/g' |
awk '/typedef struct/{omit=1} /}/{omit=0;next} {if(!omit)print}' >> $package.pkg

echo "}" >> $package.pkg


echo "generating '$package.c' wrapper for LUA from '$package.pkg'..."

# generate Wlib LUA wrapper:
# - remove 'w_' prefix from function names
# - remove 'W_' prefix from define names
# - remove '_t' postfix from type names
tolua $package.pkg | sed -e 's/"w_/"/g' -e 's/"W_/"/g' -e 's/_t"/"/g' > $package.c
