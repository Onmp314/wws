/* 
 * A test for range (and dial widget if DIAL defined)
 *
 * (w) 1997 by Eero Tamminen
 */

#include <stdio.h>
#include <sys/types.h>
#include <Wlib.h>
#include <Wt.h>

static widget_t *Red, *Green, *Blue, *Gamma;
#ifdef DIAL
static widget_t *Dial;
#endif

static void show_values(void)
{
  char *red, *green, *blue, *gamma, *angle = "0.0";

  wt_getopt(Red,   WT_VALUE, &red,   WT_EOL);
  wt_getopt(Green, WT_VALUE, &green, WT_EOL);
  wt_getopt(Blue,  WT_VALUE, &blue,  WT_EOL);
  wt_getopt(Gamma, WT_VALUE, &gamma, WT_EOL);
#ifdef DIAL
  wt_getopt(Dial,  WT_VALUE, &angle, WT_EOL);
#endif
  fprintf(stderr, "Color rgb <%s, %s, %s> with gamma: %s at angle: %s\n",
	red, green, blue, gamma, angle);
}

int main()
{
#ifdef DIAL
	widget_t *dial;
#endif
	widget_t *top, *shell, *vpane, *hpane, *gamma,
		*vpane1, *red, *vpane2, *green, *vpane3, *blue;
	long a, b, d;

	top    = wt_init();
	shell  = wt_create(wt_shell_class, top);
	vpane  = wt_create(wt_pane_class, shell);
#ifdef DIAL
	dial   = wt_create(wt_label_class, vpane);
	Dial   = wt_create(wt_dial_class, vpane);
#endif
	hpane  = wt_create(wt_pane_class, vpane);
	vpane1 = wt_create(wt_pane_class, hpane);
	red    = wt_create(wt_label_class, vpane1);
	Red    = wt_create(wt_range_class, vpane1);
	vpane2 = wt_create(wt_pane_class, hpane);
        green  = wt_create(wt_label_class, vpane2);
	Green  = wt_create(wt_range_class, vpane2);
	vpane3 = wt_create(wt_pane_class, hpane);
	blue   = wt_create(wt_label_class, vpane3);
	Blue   = wt_create(wt_range_class, vpane3);
	gamma  = wt_create(wt_label_class, vpane);
	Gamma  = wt_create(wt_range_class, vpane);

	wt_setopt(shell, WT_LABEL, " Range demo ", WT_EOL);
	wt_setopt(red,   WT_LABEL, "red", WT_EOL);
	wt_setopt(green, WT_LABEL, "green", WT_EOL);
	wt_setopt(blue,  WT_LABEL, "blue", WT_EOL);
	wt_setopt(gamma, WT_LABEL, "Gamma correction", WT_EOL);

#ifdef DIAL
	wt_setopt(dial,  WT_LABEL, "The color angle", WT_EOL);
	a = AlignRight;
/*	b = DialFullCircle; */
	wt_setopt(Dial,
/*		WT_MODE, &b, */
		WT_VALUE_MIN, "0",
		WT_VALUE_MAX, "180",
		WT_ALIGNMENT, &a,
		WT_EOL);
#endif

	b = OrientHorz;
	wt_setopt(hpane, WT_ORIENTATION, &b, WT_EOL);

	d = 1;
	a = 100;
	wt_setopt(Gamma,
		WT_ORIENTATION, &b,
		WT_VALUE_MIN, "0.0",
		WT_VALUE_MAX, "4.0",
		WT_VALUE, "1.0",
		WT_VALUE_DECIMALS, &d,
		WT_VALUE_STEPS, &a,
		WT_EOL);

	d = 3;
	a = 256;
	wt_setopt(Red,
		WT_VALUE_MIN, "1.000",
		WT_VALUE_MAX, "0.000",
		WT_VALUE_DECIMALS, &d,
		WT_VALUE_STEPS, &a,
		WT_EOL);

	wt_setopt(Green,
		WT_VALUE_MIN, "1.000",
		WT_VALUE_MAX, "0.000",
		WT_VALUE_DECIMALS, &d,
		WT_VALUE_STEPS, &a,
		WT_EOL);

	wt_setopt(Blue,
		WT_VALUE_MIN, "1.000",
		WT_VALUE_MAX, "0.000",
		WT_VALUE_DECIMALS, &d,
		WT_VALUE_STEPS, &a,
		WT_EOL);

	wt_realize(top);
	wt_run();

	show_values();
	return 0;
}
