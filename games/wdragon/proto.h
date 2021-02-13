/* Functions in BOARD.C */

void Write_Game(FILE *file);
void Read_Game(FILE *file);
void Set_Tile_Controls(void);
void Setup_New_Game(void);
void Restart_Game(void);
void Tile_Remove(void);
void Tile_Press(WEVENT *event);
void Tile_Release(void);
void Next_Tile(int Click, int *row, int *col);
void Hints(void);

/* Functions in BUTTON.C */

void Complain(const char *about);
void Do_Button_Configuration(void);
void Draw_Text(const char *str, int x, int y);
void Draw_Score(int score, int x, int y);
void Button_Expose(void);
void Button_Press(WEVENT *event);
void Button_Release(void);

/* Functions in DRAW.C */

void Hilite_Tile(int row, int col);
void Draw_All_Tiles(void);
void Show_Samples(void);
void Board_Expose(void);
void Board_Configure(short width, short height);
void Board_Setup(void);

/* Functions in ICON.C */

void Icon_Setup(void);
void Iconify(void);

/* Functions in TILE.C */

void Configure_Tiles(int size);
void Draw_Spring(int x, int y);
void Draw_Summer(int x, int y);
void Draw_Fall(int x, int y);
void Draw_Winter(int x, int y);
void Draw_Bamboo(int x, int y);
void Draw_Mum(int x, int y);
void Draw_Orchid(int x, int y);
void Draw_Plum(int x, int y);
void Draw_GDragon(int x, int y);
void Draw_RDragon(int x, int y);
void Draw_WDragon(int x, int y);
void Draw_East(int x, int y);
void Draw_West(int x, int y);
void Draw_North(int x, int y);
void Draw_South(int x, int y);
void Draw_Bam1(int x, int y);
void Draw_Bam2(int x, int y);
void Draw_Bam3(int x, int y);
void Draw_Bam4(int x, int y);
void Draw_Bam5(int x, int y);
void Draw_Bam6(int x, int y);
void Draw_Bam7(int x, int y);
void Draw_Bam8(int x, int y);
void Draw_Bam9(int x, int y);
void Draw_Dot1(int x, int y);
void Draw_Dot2(int x, int y);
void Draw_Dot3(int x, int y);
void Draw_Dot4(int x, int y);
void Draw_Dot5(int x, int y);
void Draw_Dot6(int x, int y);
void Draw_Dot7(int x, int y);
void Draw_Dot8(int x, int y);
void Draw_Dot9(int x, int y);
void Draw_Crak1(int x, int y);
void Draw_Crak2(int x, int y);
void Draw_Crak3(int x, int y);
void Draw_Crak4(int x, int y);
void Draw_Crak5(int x, int y);
void Draw_Crak6(int x, int y);
void Draw_Crak7(int x, int y);
void Draw_Crak8(int x, int y);
void Draw_Crak9(int x, int y);

/* Functions in WFUNCS.C */

void wsetmode(short mode);
void wsetpattern(ushort mode);
void wpolyline(Point *pnts, int npnts);
void wpolygon(Point *pnts, int npnts);
void wdpolyline(Point *pnts, int npnts);
void wdpolygon(Point *pnts, int npnts);
void wputblock(BITMAP *bm, short x, short y);
