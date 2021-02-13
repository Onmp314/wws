/******************************************************************************
* Dragon - a version of Mah-Jongg for X Windows
*
* Author: Gary E. Barnes	March 1989
* W Port: Jens Kilian		February 1996
*
* tile_bits.c - Mah-Jongg tile bitmap data.
******************************************************************************/

#include <Wlib.h>
#include "main.h"

/******************************************************************************
* Tile Sizes
*
* For each size of playing surface (there are 5 sizes) we have a different
* size for tiles and for what goes on them.  The tile sizes are in the Tile
* column.
*
* Many tiles (eg. Summer) are a single image.  However, some times (eg. Crak's)
* are made up of composite images.  These image pieces have sizes as shown
* in the Objs column.
*
* Many tiles (eg. Summer) have writing upon them.  The sizes of lettering
* on these tiles is given in the Char column.  The lettering on these tiles
* is positioned upwards from the bottom of the tile by the number of pixels
* indicated in the Up column.  The letters are spaced apart by the number of
* pixels indicated in the Apart column.
*
*	Tile	Objs	Char	Up	Apart
*
*	28x32	7x8	4x6	1	1
*	40x48	10x12	5x7	1	1
*	56x64	14x16	7x11	7	1
*	68x80	17x20	9x13	11	2
*	80x96	20x24	9x13	16	3
* 
******************************************************************************/

/*--SEASONS */

#include "bitmaps/spring_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/spring_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/spring_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/spring_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/spring_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	spring;

#include "bitmaps/summer_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/summer_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/summer_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/summer_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/summer_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	summer;

#include "bitmaps/fall_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/fall_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/fall_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/fall_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/fall_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	fall;

#include "bitmaps/winter_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/winter_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/winter_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/winter_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/winter_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	winter;

/*--FLOWERS */

#include "bitmaps/bamboo_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/bamboo_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/bamboo_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/bamboo_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/bamboo_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	bamboo;

#include "bitmaps/mum_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/mum_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/mum_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/mum_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/mum_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	mum;

#include "bitmaps/orchid_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/orchid_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/orchid_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/orchid_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/orchid_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	orchid;

#include "bitmaps/plum_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/plum_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/plum_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/plum_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/plum_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	plum;

/*--DRAGONS */

#include "bitmaps/gdragon_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/gdragon_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/gdragon_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/gdragon_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/gdragon_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	gdragon;

#include "bitmaps/rdragon_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/rdragon_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/rdragon_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/rdragon_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/rdragon_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	rdragon;

#include "bitmaps/wdragon_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/wdragon_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/wdragon_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/wdragon_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/wdragon_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	wdragon;

/*--WINDS */

#include "bitmaps/east_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/east_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/east_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/east_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/east_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	east;

#include "bitmaps/west_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/west_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/west_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/west_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/west_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	west;

#include "bitmaps/north_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/north_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/north_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/north_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/north_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	north;

#include "bitmaps/south_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/south_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/south_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/south_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/south_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	south;

/*--NUMBERS */

#include "bitmaps/one_7x8.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/one_10x12.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/one_14x16.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/one_17x20.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/one_20x24.c"
#      endif
#    endif
#  endif
#endif

BITMAP	one;

#include "bitmaps/two_7x8.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/two_10x12.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/two_14x16.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/two_17x20.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/two_20x24.c"
#      endif
#    endif
#  endif
#endif

BITMAP	two;

#include "bitmaps/three_7x8.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/three_10x12.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/three_14x16.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/three_17x20.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/three_20x24.c"
#      endif
#    endif
#  endif
#endif

BITMAP	three;

#include "bitmaps/four_7x8.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/four_10x12.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/four_14x16.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/four_17x20.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/four_20x24.c"
#      endif
#    endif
#  endif
#endif

BITMAP	four;

#include "bitmaps/five_7x8.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/five_10x12.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/five_14x16.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/five_17x20.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/five_20x24.c"
#      endif
#    endif
#  endif
#endif

BITMAP	five;

#include "bitmaps/six_7x8.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/six_10x12.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/six_14x16.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/six_17x20.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/six_20x24.c"
#      endif
#    endif
#  endif
#endif

BITMAP	six;

#include "bitmaps/seven_7x8.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/seven_10x12.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/seven_14x16.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/seven_17x20.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/seven_20x24.c"
#      endif
#    endif
#  endif
#endif

BITMAP	seven;

#include "bitmaps/eight_7x8.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/eight_10x12.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/eight_14x16.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/eight_17x20.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/eight_20x24.c"
#      endif
#    endif
#  endif
#endif

BITMAP	eight;

#include "bitmaps/nine_7x8.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/nine_10x12.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/nine_14x16.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/nine_17x20.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/nine_20x24.c"
#      endif
#    endif
#  endif
#endif

BITMAP	nine;

/*--OTHER */

#include "bitmaps/bam_7x8.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/bam_10x12.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/bam_14x16.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/bam_17x20.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/bam_20x24.c"
#      endif
#    endif
#  endif
#endif

BITMAP	bam;

#include "bitmaps/crak_28x32.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/crak_40x48.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/crak_56x64.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/crak_68x80.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/crak_80x96.c"
#      endif
#    endif
#  endif
#endif

BITMAP	crak;

#include "bitmaps/dot_7x8.c"
#if N_BITMAP_SIZES > 1
#  include "bitmaps/dot_10x12.c"
#  if N_BITMAP_SIZES > 2
#    include "bitmaps/dot_14x16.c"
#    if N_BITMAP_SIZES > 3
#      include "bitmaps/dot_17x20.c"
#      if N_BITMAP_SIZES > 4
#        include "bitmaps/dot_20x24.c"
#      endif
#    endif
#  endif
#endif

BITMAP	dot;


/******************************************************************************
* BITMAP initialization structures
******************************************************************************/

BITMAP_Init Sizes[5][28] = {
 {
  { &spring,  spring_28x32_bits,  spring_28x32_width,  spring_28x32_height  },
  { &summer,  summer_28x32_bits,  summer_28x32_width,  summer_28x32_height  },
  { &fall,    fall_28x32_bits,    fall_28x32_width,    fall_28x32_height    },
  { &winter,  winter_28x32_bits,  winter_28x32_width,  winter_28x32_height  },
  { &bamboo,  bamboo_28x32_bits,  bamboo_28x32_width,  bamboo_28x32_height  },
  { &mum,     mum_28x32_bits,     mum_28x32_width,     mum_28x32_height     },
  { &orchid,  orchid_28x32_bits,  orchid_28x32_width,  orchid_28x32_height  },
  { &plum,    plum_28x32_bits,    plum_28x32_width,    plum_28x32_height    },
  { &gdragon, gdragon_28x32_bits, gdragon_28x32_width, gdragon_28x32_height },
  { &rdragon, rdragon_28x32_bits, rdragon_28x32_width, rdragon_28x32_height },
  { &wdragon, wdragon_28x32_bits, wdragon_28x32_width, wdragon_28x32_height },
  { &east,    east_28x32_bits,    east_28x32_width,    east_28x32_height    },
  { &west,    west_28x32_bits,    west_28x32_width,    west_28x32_height    },
  { &north,   north_28x32_bits,   north_28x32_width,   north_28x32_height   },
  { &south,   south_28x32_bits,   south_28x32_width,   south_28x32_height   },
  { &one,     one_7x8_bits,       one_7x8_width,       one_7x8_height       },
  { &two,     two_7x8_bits,       two_7x8_width,       two_7x8_height       },
  { &three,   three_7x8_bits,     three_7x8_width,     three_7x8_height     },
  { &four,    four_7x8_bits,      four_7x8_width,      four_7x8_height      },
  { &five,    five_7x8_bits,      five_7x8_width,      five_7x8_height      },
  { &six,     six_7x8_bits,       six_7x8_width,       six_7x8_height       },
  { &seven,   seven_7x8_bits,     seven_7x8_width,     seven_7x8_height     },
  { &eight,   eight_7x8_bits,     eight_7x8_width,     eight_7x8_height     },
  { &nine,    nine_7x8_bits,      nine_7x8_width,      nine_7x8_height      },
  { &bam,     bam_7x8_bits,       bam_7x8_width,       bam_7x8_height       },
  { &crak,    crak_28x32_bits,    crak_28x32_width,    crak_28x32_height    },
  { &dot,     dot_7x8_bits,       dot_7x8_width,       dot_7x8_height       },
  { (BITMAP*)NULL, NULL, 0, 0 }
 },
#if N_BITMAP_SIZES > 1
 {
  { &spring,  spring_40x48_bits,  spring_40x48_width,  spring_40x48_height  },
  { &summer,  summer_40x48_bits,  summer_40x48_width,  summer_40x48_height  },
  { &fall,    fall_40x48_bits,    fall_40x48_width,    fall_40x48_height    },
  { &winter,  winter_40x48_bits,  winter_40x48_width,  winter_40x48_height  },
  { &bamboo,  bamboo_40x48_bits,  bamboo_40x48_width,  bamboo_40x48_height  },
  { &mum,     mum_40x48_bits,     mum_40x48_width,     mum_40x48_height     },
  { &orchid,  orchid_40x48_bits,  orchid_40x48_width,  orchid_40x48_height  },
  { &plum,    plum_40x48_bits,    plum_40x48_width,    plum_40x48_height    },
  { &gdragon, gdragon_40x48_bits, gdragon_40x48_width, gdragon_40x48_height },
  { &rdragon, rdragon_40x48_bits, rdragon_40x48_width, rdragon_40x48_height },
  { &wdragon, wdragon_40x48_bits, wdragon_40x48_width, wdragon_40x48_height },
  { &east,    east_40x48_bits,    east_40x48_width,    east_40x48_height    },
  { &west,    west_40x48_bits,    west_40x48_width,    west_40x48_height    },
  { &north,   north_40x48_bits,   north_40x48_width,   north_40x48_height   },
  { &south,   south_40x48_bits,   south_40x48_width,   south_40x48_height   },
  { &one,     one_10x12_bits,     one_10x12_width,     one_10x12_height     },
  { &two,     two_10x12_bits,     two_10x12_width,     two_10x12_height     },
  { &three,   three_10x12_bits,   three_10x12_width,   three_10x12_height   },
  { &four,    four_10x12_bits,    four_10x12_width,    four_10x12_height    },
  { &five,    five_10x12_bits,    five_10x12_width,    five_10x12_height    },
  { &six,     six_10x12_bits,     six_10x12_width,     six_10x12_height     },
  { &seven,   seven_10x12_bits,   seven_10x12_width,   seven_10x12_height   },
  { &eight,   eight_10x12_bits,   eight_10x12_width,   eight_10x12_height   },
  { &nine,    nine_10x12_bits,    nine_10x12_width,    nine_10x12_height    },
  { &bam,     bam_10x12_bits,     bam_10x12_width,     bam_10x12_height     },
  { &crak,    crak_40x48_bits,    crak_40x48_width,    crak_40x48_height    },
  { &dot,     dot_10x12_bits,     dot_10x12_width,     dot_10x12_height     },
  { (BITMAP*)NULL, NULL, 0, 0 }
 },
#  if N_BITMAP_SIZES > 2
 {
  { &spring,  spring_56x64_bits,  spring_56x64_width,  spring_56x64_height  },
  { &summer,  summer_56x64_bits,  summer_56x64_width,  summer_56x64_height  },
  { &fall,    fall_56x64_bits,    fall_56x64_width,    fall_56x64_height    },
  { &winter,  winter_56x64_bits,  winter_56x64_width,  winter_56x64_height  },
  { &bamboo,  bamboo_56x64_bits,  bamboo_56x64_width,  bamboo_56x64_height  },
  { &mum,     mum_56x64_bits,     mum_56x64_width,     mum_56x64_height     },
  { &orchid,  orchid_56x64_bits,  orchid_56x64_width,  orchid_56x64_height  },
  { &plum,    plum_56x64_bits,    plum_56x64_width,    plum_56x64_height    },
  { &gdragon, gdragon_56x64_bits, gdragon_56x64_width, gdragon_56x64_height },
  { &rdragon, rdragon_56x64_bits, rdragon_56x64_width, rdragon_56x64_height },
  { &wdragon, wdragon_56x64_bits, wdragon_56x64_width, wdragon_56x64_height },
  { &east,    east_56x64_bits,    east_56x64_width,    east_56x64_height    },
  { &west,    west_56x64_bits,    west_56x64_width,    west_56x64_height    },
  { &north,   north_56x64_bits,   north_56x64_width,   north_56x64_height   },
  { &south,   south_56x64_bits,   south_56x64_width,   south_56x64_height   },
  { &one,     one_14x16_bits,     one_14x16_width,     one_14x16_height     },
  { &two,     two_14x16_bits,     two_14x16_width,     two_14x16_height     },
  { &three,   three_14x16_bits,   three_14x16_width,   three_14x16_height   },
  { &four,    four_14x16_bits,    four_14x16_width,    four_14x16_height    },
  { &five,    five_14x16_bits,    five_14x16_width,    five_14x16_height    },
  { &six,     six_14x16_bits,     six_14x16_width,     six_14x16_height     },
  { &seven,   seven_14x16_bits,   seven_14x16_width,   seven_14x16_height   },
  { &eight,   eight_14x16_bits,   eight_14x16_width,   eight_14x16_height   },
  { &nine,    nine_14x16_bits,    nine_14x16_width,    nine_14x16_height    },
  { &bam,     bam_14x16_bits,     bam_14x16_width,     bam_14x16_height     },
  { &crak,    crak_56x64_bits,    crak_56x64_width,    crak_56x64_height    },
  { &dot,     dot_14x16_bits,     dot_14x16_width,     dot_14x16_height     },
  { (BITMAP*)NULL, NULL, 0, 0 }
 },
#    if N_BITMAP_SIZES > 3
 {
  { &spring,  spring_68x80_bits,  spring_68x80_width,  spring_68x80_height  },
  { &summer,  summer_68x80_bits,  summer_68x80_width,  summer_68x80_height  },
  { &fall,    fall_68x80_bits,    fall_68x80_width,    fall_68x80_height    },
  { &winter,  winter_68x80_bits,  winter_68x80_width,  winter_68x80_height  },
  { &bamboo,  bamboo_68x80_bits,  bamboo_68x80_width,  bamboo_68x80_height  },
  { &mum,     mum_68x80_bits,     mum_68x80_width,     mum_68x80_height     },
  { &orchid,  orchid_68x80_bits,  orchid_68x80_width,  orchid_68x80_height  },
  { &plum,    plum_68x80_bits,    plum_68x80_width,    plum_68x80_height    },
  { &gdragon, gdragon_68x80_bits, gdragon_68x80_width, gdragon_68x80_height },
  { &rdragon, rdragon_68x80_bits, rdragon_68x80_width, rdragon_68x80_height },
  { &wdragon, wdragon_68x80_bits, wdragon_68x80_width, wdragon_68x80_height },
  { &east,    east_68x80_bits,    east_68x80_width,    east_68x80_height    },
  { &west,    west_68x80_bits,    west_68x80_width,    west_68x80_height    },
  { &north,   north_68x80_bits,   north_68x80_width,   north_68x80_height   },
  { &south,   south_68x80_bits,   south_68x80_width,   south_68x80_height   },
  { &one,     one_17x20_bits,     one_17x20_width,     one_17x20_height     },
  { &two,     two_17x20_bits,     two_17x20_width,     two_17x20_height     },
  { &three,   three_17x20_bits,   three_17x20_width,   three_17x20_height   },
  { &four,    four_17x20_bits,    four_17x20_width,    four_17x20_height    },
  { &five,    five_17x20_bits,    five_17x20_width,    five_17x20_height    },
  { &six,     six_17x20_bits,     six_17x20_width,     six_17x20_height     },
  { &seven,   seven_17x20_bits,   seven_17x20_width,   seven_17x20_height   },
  { &eight,   eight_17x20_bits,   eight_17x20_width,   eight_17x20_height   },
  { &nine,    nine_17x20_bits,    nine_17x20_width,    nine_17x20_height    },
  { &bam,     bam_17x20_bits,     bam_17x20_width,     bam_17x20_height     },
  { &crak,    crak_68x80_bits,    crak_68x80_width,    crak_68x80_height    },
  { &dot,     dot_17x20_bits,     dot_17x20_width,     dot_17x20_height     },
  { (BITMAP*)NULL, NULL, 0, 0 }
 },
#      if N_BITMAP_SIZES > 4
 {
  { &spring,  spring_80x96_bits,  spring_80x96_width,  spring_80x96_height  },
  { &summer,  summer_80x96_bits,  summer_80x96_width,  summer_80x96_height  },
  { &fall,    fall_80x96_bits,    fall_80x96_width,    fall_80x96_height    },
  { &winter,  winter_80x96_bits,  winter_80x96_width,  winter_80x96_height  },
  { &bamboo,  bamboo_80x96_bits,  bamboo_80x96_width,  bamboo_80x96_height  },
  { &mum,     mum_80x96_bits,     mum_80x96_width,     mum_80x96_height     },
  { &orchid,  orchid_80x96_bits,  orchid_80x96_width,  orchid_80x96_height  },
  { &plum,    plum_80x96_bits,    plum_80x96_width,    plum_80x96_height    },
  { &gdragon, gdragon_80x96_bits, gdragon_80x96_width, gdragon_80x96_height },
  { &rdragon, rdragon_80x96_bits, rdragon_80x96_width, rdragon_80x96_height },
  { &wdragon, wdragon_80x96_bits, wdragon_80x96_width, wdragon_80x96_height },
  { &east,    east_80x96_bits,    east_80x96_width,    east_80x96_height    },
  { &west,    west_80x96_bits,    west_80x96_width,    west_80x96_height    },
  { &north,   north_80x96_bits,   north_80x96_width,   north_80x96_height   },
  { &south,   south_80x96_bits,   south_80x96_width,   south_80x96_height   },
  { &one,     one_20x24_bits,     one_20x24_width,     one_20x24_height     },
  { &two,     two_20x24_bits,     two_20x24_width,     two_20x24_height     },
  { &three,   three_20x24_bits,   three_20x24_width,   three_20x24_height   },
  { &four,    four_20x24_bits,    four_20x24_width,    four_20x24_height    },
  { &five,    five_20x24_bits,    five_20x24_width,    five_20x24_height    },
  { &six,     six_20x24_bits,     six_20x24_width,     six_20x24_height     },
  { &seven,   seven_20x24_bits,   seven_20x24_width,   seven_20x24_height   },
  { &eight,   eight_20x24_bits,   eight_20x24_width,   eight_20x24_height   },
  { &nine,    nine_20x24_bits,    nine_20x24_width,    nine_20x24_height    },
  { &bam,     bam_20x24_bits,     bam_20x24_width,     bam_20x24_height     },
  { &crak,    crak_80x96_bits,    crak_80x96_width,    crak_80x96_height    },
  { &dot,     dot_20x24_bits,     dot_20x24_width,     dot_20x24_height     },
  { (BITMAP*)NULL, NULL, 0, 0 }
 }
#      endif
#    endif
#  endif
#endif
};
