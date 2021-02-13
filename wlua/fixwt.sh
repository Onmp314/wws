#!/bin/sh
#
# generate Wt wrapper C-code for W-Lua
#
# For this you need 'tolua' utility from:
#	http://www.tecgraf.puc-rio.br/~celes/tolua/


echo "generating 'wt.c' wrapper for Lua from 'wt.pkg'..."

# generate Wt LUA wrapper:
# - remove 'wt_' prefix from functions names
# - remove 'WT_' prefix from define names
# - remove '_t' postfix from type names
# - rename wt.break to wt.quit
tolua wt.pkg | sed -e 's/"wt_/"/g' -e 's/"WT_/"/g' -e 's/_t"/"/g' -e 's/"break"/"quit"/g' > wt.c
