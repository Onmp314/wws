/*
 * lsys.c, a part of the W Window System
 *
 * Copyright (C) 1996 by Eero Tamminen
 *
 * This package is free software; you can redistribute it and/or modify it
 * under the terms specified in the docs/COPYRIGHTS file coming with this
 * package.
 *
 * -- a primitive L-system parser and some basic turtle graphics for W
 */

#include <Wlib.h>
#include <math.h>		/* sin(), cos(), M_PI */
#include <stdio.h>
#include <stdlib.h>		/* atoi() */
#include <string.h>		/* mem*() */

/* window defines */
#define WIN_NAME	" W-Lsys "
#define WIN_PROPERTIES	(W_MOVE | W_TITLE | W_CLOSE)
#define DEF_SIZE	256
#define MIN_SIZE	64

/* window ID */
WWIN *Win;

/* parser limit defines */
#define MAX_DEPTH	16	/* max. recursion depth for parsing */
#define MAX_ANGLES	36
#define MAX_RULES	16

typedef struct
{
  double x;
  double y;
} MinMax;

typedef struct
{
  int dir;			/* turtle heading (angle array index) */
  double penx, peny;		/* turtle position */
} TURTLE;


/* global graphics variables */
double Distance;
double Qsin[MAX_ANGLES];
double Qcos[MAX_ANGLES];
int Angles, Scaling;
MinMax Min, Max;		/* for scaling */

int StkIdx;			/* Turtle state (Store) stack index */
TURTLE Store[MAX_DEPTH];	/* grammar -> graphics mapping stack */
TURTLE Turtle;			/* for drawing */


/* prototypes */
void	help(const char *name);
const char *args(int argc, char *argv[], int *depth, int *angle,
		 int *width, int *height);
char	*get_rules(int *angle);
void	parse(const char *axiom, const char *rules, int depth);
void	draw(char token);
const char *win_init(int width, int height);
void	start_drawing(void);
void	check_events(long timeout);


int main(int argc, char *argv[])
{
  int
    idx,
    depth = 3,			/* recursion depth */
    width = DEF_SIZE,		/* window width */
    height= DEF_SIZE;		/* window height */
  const char
    *rules,
    *error;

  Angles = 6;
  if(!(rules = get_rules(&Angles)))
  {
    fprintf(stderr, "%s: invalid rules\n", *argv);
    return -1;
  }
  if((error = args(argc, argv, &depth, &Angles, &width, &height)))
  {
    fprintf(stderr, "%s: %s\n", *argv, error);
    help(*argv);
    return -1;
  }

  if((error = win_init(width, height)))
  {
    fprintf(stderr, "%s: %s\n", *argv, error);
    return -1;
  }

  /* precalculate angles (so that 0 degrees is up) */
  for(idx = 0; idx < Angles; idx++)
  {
    Qsin[idx] = qsin(360.0 / Angles * idx + 90);
    Qcos[idx] = qcos(360.0 / Angles * idx + 90);
  }

  /* setup variables for scaling */
  Max.x = Max.y = Min.x = Min.y = Turtle.penx = Turtle.peny = 0.0;
  Distance = 1.0; Scaling = 1;
  StkIdx = Turtle.dir = 0;

  parse(&rules[1], &rules[rules[0]+1], depth);
  check_events(0L);

  /* Scales Distance / Turtle position values
   * so that the image fits into the window
   */
  if((Max.x - Min.x) > (Max.y - Min.y))
    Distance = (double)(width - width/8) / (Max.x - Min.x);
  else
    Distance = (double)(height - height/8) / (Max.y - Min.y);
  Turtle.penx = (double)(width/2)  - (Max.x + Min.x) / 2.0 * Distance;
  Turtle.peny = (double)(height/2) - (Max.y + Min.y) / 2.0 * Distance;
  StkIdx = Turtle.dir = 0;
  Scaling = 0;

  /* do some changes */
  start_drawing();
  parse(&rules[1], &rules[rules[0]+1], depth);
  check_events(-1L);

  return 0;
}

void help(const char *name)
{
  fprintf(stderr, "\nUsage: %s [<options>] < <rules>\n\n", name);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -w <window width>\n");
  fprintf(stderr, "  -h <window height>\n\n"); 
  fprintf(stderr, "  -d <recursion depth> (0-%d)\n", MAX_DEPTH/2);
  fprintf(stderr, "  -a <angles> (360 / turn angle, 3-%d)\n", MAX_ANGLES);
  fprintf(stderr, "For rules (file) format see the manual page.\n\n");
  fprintf(stderr, "w/ Feb 1996 by Eero Tamminen\n");
}

/* Parse command line arguments. 1 for success, 0 for failure */
const char *args(int argc, char *argv[],
	int *depth, int *angle, int *width, int *height)
{
  int idx = 0;
  while(++idx < argc)
  {
    /* an one letter option and argument for it? */
    if(argv[idx][0] == '-' && argv[idx][2] == '\0' && idx+1 < argc)
      switch(argv[idx++][1])
      {
	case 'w':				/* window width */
	  if((*width = atoi(argv[idx])) < MIN_SIZE)
	    *width = MIN_SIZE;
	  break;
	case 'h':				/* window height */
	  if((*height = atoi(argv[idx])) < MIN_SIZE)
	    *height = MIN_SIZE;
	  break;
	case 'a':				/* 360 / turn angle */
          if((*angle = atoi(argv[idx])) < 3 || *angle > MAX_ANGLES)
	    return "invalid angles value";
	  break;
	case 'd':				/* recursion depth */
          if((*depth = atoi(argv[idx])) < 0)
	    return "bogus recursion depth";
	  break;
	default:
	  return "unrecognized option";
      }
    else
      return "not an option & it's argument";
  }
  return 0;	/* success */
}

/* parse rules (rule => 'char rule_lenght+2' 'char rule[]' with the
 * '=' character and white space stripped off from the input rule)
 * from the stdin to buffer
 */
char *get_rules(int *angle)
{
#define BUFFER_SIZE	512		/* the rules buffer size */
  static char buffer[BUFFER_SIZE];
  int idx = 1;

  scanf("%d", angle);

  if(*angle < 3 || *angle > MAX_ANGLES || scanf("%s", &buffer[idx]) != 1)
  {
    *angle = 6;
    /* no axiom -> default axiom & rule */
    memcpy(buffer, "\011" "f++f++f\0" "\013" "f" "f-f++f-f\0\0\0", 22);
    return buffer;
  }
  idx += buffer[idx-1] = strlen(&buffer[idx]) + 2;

  /* get the rules */
  while(idx+72 < BUFFER_SIZE)
  {
    if(scanf("%1s = %70s", &buffer[idx], &buffer[idx+1]) != 2)
      break;
    idx += buffer[idx-1] = strlen(&buffer[idx]) + 2;
  }
  buffer[idx] = 0;
  return buffer;
#undef BUFFER_SIZE
}


/* ---------------------- L-system parser ---------------------- */

/* axiom (intial rule), recursive rule, recursion depth */
void parse(const char *axiom, const char *rules, int depth)
{
  const char *rule;

  if(depth < 1)
  {
    while(*axiom)
      draw(*(axiom++));
    return;
  }

  while(*axiom)
  {
    rule = rules;
    while(*rule)
    {
      if(*axiom == *rule)
      {
        parse(&rule[1], rules, depth - 1);
	goto next;
      }
      rule += *(rule-1);
    }
    draw(*axiom);
    next:
    axiom++;
  }
}


/* --------------- Grammar -> graphics mapping -------------- */

void draw(char token)
{
  double oldx, oldy;

  switch(token)
  {
    case 'f':
    case 'F':
      oldx = Turtle.penx;
      oldy = Turtle.peny;
      Turtle.penx += Qcos[Turtle.dir] * Distance;
      Turtle.peny -= Qsin[Turtle.dir] * Distance;
      if(Scaling)
      {
       	if(Turtle.penx > Max.x) Max.x = Turtle.penx;
	if(Turtle.peny > Max.y) Max.y = Turtle.peny;
	if(Turtle.penx < Min.x) Min.x = Turtle.penx;
	if(Turtle.peny < Min.y) Min.y = Turtle.peny;
      }
      else if(token == 'f')
        w_line(Win, (short)oldx, (short)oldy,
	  (short)Turtle.penx, (short)Turtle.peny);
      break;

    case '+':			/* turn clockwise 1 'angle' */
      Turtle.dir++;
      if(Turtle.dir >= Angles)
        Turtle.dir = 0;
      break;

    case '-':			/* turn anti-clockwise 1 'angle' */
      Turtle.dir--;
      if(Turtle.dir < 0)
        Turtle.dir = Angles-1;
      break;

    case '[':			/* store turtle state to stack */
      if(StkIdx < MAX_DEPTH)
      {
	Store[StkIdx].penx = Turtle.penx;
	Store[StkIdx].peny = Turtle.peny;
	Store[StkIdx].dir  = Turtle.dir;
	StkIdx++;
      }
      break;

    case ']':			/* restore turtle state from stack */
      if(StkIdx > 0)
      {
	StkIdx--;
	Turtle.penx = Store[StkIdx].penx;
	Turtle.peny = Store[StkIdx].peny;
	Turtle.dir  = Store[StkIdx].dir;
      }
      break;
  }
}


/* ----------------------- W stuff --------------------------- */

/* initialize W connection and a output window */
const char *win_init(int width, int height)
{
  if(!w_init())
    return "can't connect to wserver";

  if(!(Win = w_create(width, height, WIN_PROPERTIES)))
    return "can't create window";

  /* draw white onto black window (looks IMHO better :-)) */
  w_pbox(Win, 0, 0, width, height);
  w_settitle(Win, " Scaling... ");

  if(w_open(Win, UNDEF, UNDEF) < 0)
    return "can't open window";

  return 0;
}

/* show that drawing has started... */
void start_drawing(void)
{
  /* change window title */
  w_settitle(Win, WIN_NAME);
  /* use white on black */
  w_setmode(Win, M_CLEAR);
}

/* wait until an event... */
void check_events(long timeout)
{
  WEVENT *ev;
  if(!(ev = w_queryevent(NULL, NULL, NULL, timeout)))
    return;

  if(ev->type == EVENT_GADGET &&
     (ev->key == GADGET_EXIT || ev->key == GADGET_CLOSE))
  {
    w_delete(Win);
    exit(0);
  }
}
