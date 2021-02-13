/*
 * this file is part of WetScape, a Web browser for the W window system.
 * Copyrigt (C) 1996 Kay Roemer.
 *
 * default contents for ~/.wetscape/ files.
 *
 * $Id: defaults.h,v 1.2 2003/05/18 16:09:30 eero Exp $
 */

#define DEF_CONFIG_FILE "\
#\n\
# WetScape configuration file.\n\
#\n\
\n\
# set home page\n\
#home-page \"http://www.uni-frankfurt.de/~roemer/\"\n\
\n\
# set bookmark file (file name, not an url!).\n\
# Defaults to $HOME/.wetscape/bookmarks.html\n\
#bookmark-file /home/staff/roemer/.wetscape/bookmarks.html\n\
\n\
# set http proxy\n\
#http-proxy rs501.th.physik.uni-frankfurt.de:8080\n"


#define DEF_BOOKMARK_FILE "\
<!-- WetScape bookmarks -->\n\
<h1>Bookmarks</h1>\n\
<ul>\n\
<li><a href=\"http://www.vsb.informatik.uni-frankfurt.de/~roemer/wt.html\">\
The W Toolkit</a>\n\
</ul>\n"
