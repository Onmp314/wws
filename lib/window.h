/* attribute setting is an include so that w2xlib can use the same code... */


static void set_defgc(WWIN *win, ushort handle, short width, short height)
{
  win->magic = MAGIC_W;
  win->handle = handle;
  win->width = width;
  win->height = height;

  win->linewidth = 1;
  win->pattern = GRAY_PATTERN;
  win->drawmode = DEFAULT_GMODE;
  win->textstyle = F_NORMAL;

  win->fg = FGCOL_INDEX;
  win->bg = BGCOL_INDEX;
  win->colors = _wserver.sharedcolors;

  /* others should already be zeroed */
}
