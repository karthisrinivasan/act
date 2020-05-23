/*************************************************************************
 *
 *  Copyright (c) 2019 Rajit Manohar
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 *
 **************************************************************************
 */
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <map>

#include <act/act.h>
#include <act/tech.h>
#include <pp.h>

#define pp_nl pp_forced (pp, 0)
#define pp_nltab pp_forced (pp, 3)
#define pp_TAB pp_nltab; pp_setb (pp)
#define pp_UNTAB pp_endb (pp); pp_nl
#define pp_SPACE pp_nl; pp_nl
 
static void emit_header (pp_t *pp)
{
  pp_printf (pp, "tech"); pp_TAB;
  pp_printf (pp, "format 33"); pp_nl;
  for (int i=0; Technology::T->name[i]; i++) {
    if (Technology::T->name[i] == ' ' || Technology::T->name[i] == '\t') break;
    pp_printf (pp, "%c", Technology::T->name[i]);
  }
  pp_UNTAB;
  pp_printf (pp, "end"); pp_SPACE;

  pp_printf (pp, "version"); pp_TAB;
  pp_printf (pp, "version 1"); pp_nl;
  pp_printf (pp, "description \"%s\"", Technology::T->date); pp_UNTAB;
  pp_printf (pp, "end"); pp_SPACE;
}

static void emit_planes (pp_t *pp)
{
  pp_printf (pp, "#--- Planes ---"); pp_nl;
  pp_printf (pp, "planes"); pp_TAB;
  pp_printf (pp, "well,w"); pp_nl;
  pp_printf (pp, "select,s"); pp_nl;
  pp_printf (pp, "active,a"); pp_nl;
  for (int i=0; i < Technology::T->nmetals; i++) {
    pp_printf (pp, "metal%d,m%d", i+1, i+1); pp_nl;
    if (i != Technology::T->nmetals-1) {
      pp_printf (pp, "via%d,v%d", i+1, i+1); pp_nl;
    }
  }
  pp_printf (pp, "comment");
  pp_UNTAB;
  pp_printf (pp, "end"); pp_SPACE;
}

static void emit_tiletypes (pp_t *pp)
{
  int empty = 1;
  
  pp_printf (pp, "#--- Tile types ---"); pp_nl;
  pp_printf (pp, "types"); pp_TAB;
  for (int i=0; i < Technology::T->num_devs; i++) {
    for (int j=0; j < 2; j++) {
      if (Technology::T->well[j][i]) {
	pp_printf (pp, "well   %s", Technology::T->well[j][i]->getName());
	pp_nl;
	empty = 0;
      }
    }
    for (int j=0; j < 2; j++) {
      if (Technology::T->welldiff[j][i]) {
	pp_printf (pp, "active   %s", Technology::T->welldiff[j][i]->getName());
	pp_nl;
	empty = 0;
      }
    }
    for (int j=0; j < 2; j++) {
      if (Technology::T->sel[j][i]) {
	pp_printf (pp, "select   %s", Technology::T->sel[j][i]->getName());
	pp_nl;
	empty = 0;
      }
    }
  }
  if (!empty) {
    pp_nl;
  }
  empty = 1;
  for (int i=0; i < Technology::T->num_devs; i++) {
    for (int j=0; j < 2; j++) {
      if (Technology::T->diff[j][i]) {
	pp_printf (pp, "active   %s", Technology::T->diff[j][i]->getName());
	pp_nl;
	pp_printf (pp, "active   %sc", Technology::T->diff[j][i]->getName());
	pp_nl;
	empty = 0;
      }
      if (Technology::T->welldiff[j][i]) {
	pp_printf (pp, "active   %sc", Technology::T->welldiff[j][i]->getName());
	pp_nl;
	empty = 0;
      }
      if (Technology::T->fet[j][i]) {
	pp_printf (pp, "active   %s", Technology::T->fet[j][i]->getName());
	pp_nl;
	empty = 0;
      }
    }
  }
  pp_printf (pp, "active   %s", Technology::T->poly->getName()); pp_nl;
  pp_printf (pp, "active   %sc", Technology::T->poly->getName()); pp_nl;
  pp_nl;

  for (int i=0; i < Technology::T->nmetals; i++) {
    pp_printf (pp, "metal%d m%d", i+1, i+1); pp_nl;
    pp_printf (pp, "metal%d m%dpin", i+1, i+1); pp_nl;
    if (i != Technology::T->nmetals-1) {
      pp_printf (pp, "metal%d m%dc,v%d", i+1, i+2, i+1); pp_nl;
    }
    else {
      pp_printf (pp, "metal%d", i+1); pp_nl;
    }
  }
  pp_printf (pp, "comment comment"); 
  pp_UNTAB;
  pp_printf (pp, "end"); pp_SPACE;
}


static void emit_contacts (pp_t *pp)
{
  pp_printf (pp, "#--- contacts ---"); pp_nl;
  pp_printf (pp, "contact"); pp_TAB;

  pp_printf (pp, "%sc %s metal1", Technology::T->poly->getName(),
	     Technology::T->poly->getName());
  pp_nl;
  for (int i=0; i < Technology::T->num_devs; i++) {
    for (int j=0; j < 2; j++) {
      if (Technology::T->diff[j][i]) {
	pp_printf (pp, "%sc %s metal1", Technology::T->diff[j][i]->getName(),
		   Technology::T->diff[j][i]->getName());
	pp_nl;
      }
#if 0      
      if (Technology::T->well[j][i]) {
	pp_printf (pp, "%sc %s metal1", Technology::T->well[j][i]->getName(),
		   Technology::T->well[j][i]->getName());
	pp_nl;
      }
#endif      
      if (Technology::T->welldiff[j][i]) {
	pp_printf (pp, "%sc %s metal1", Technology::T->welldiff[j][i]->getName(),
		   Technology::T->welldiff[j][i]->getName());
	pp_nl;
      }
    }
  }

  for (int i=1; i < Technology::T->nmetals; i++) {
    pp_printf (pp, "m%dc metal%d metal%d", i+1, i, i+1);
    pp_nl;
  }
  pp_printf (pp, "stackable"); pp_UNTAB;
  pp_printf (pp, "end"); pp_SPACE;
}

#define PTYPE 1
#define NTYPE 0

static void emit_styles (pp_t *pp)
{
  pp_printf (pp, "styles"); pp_TAB;
  pp_printf (pp, "styletype mos"); pp_TAB;

  for (int i=0; i < Technology::T->num_devs; i++) {
    for (int j=0; j < 2; j++) {
      char c = (j == PTYPE) ? 'p' : 'n';
      char d = (j == PTYPE) ? 'n' : 'p';
      if (Technology::T->well[j][i]) {
	pp_printf (pp, "%s %cwell", Technology::T->well[j][i]->getName(), d);
	pp_nl;
      }
      if (Technology::T->diff[j][i]) {
	pp_printf (pp, "%s %cdiffusion",
		   Technology::T->diff[j][i]->getName(), c); pp_nl;
	pp_printf (pp, "%sc %cdiffusion metal1 contact_X'es",
		   Technology::T->diff[j][i]->getName(), c); pp_nl;
      }
      if (Technology::T->fet[j][i]) {
	pp_printf (pp, "%s %ctransistor %ctransistor_stripes",
		   Technology::T->fet[j][i]->getName(), c, c); pp_nl;
      }
      if (Technology::T->welldiff[j][i]) {
	pp_printf (pp, "%s %cdiff_in_%cwell", Technology::T->welldiff[j][i]->getName(), d, d);
	pp_nl;
	pp_printf (pp, "%sc %cdiff_in_%cwell metal1_alt contact_X'es",
		   Technology::T->welldiff[j][i]->getName(), d, d);
	pp_nl;
      }
      if (Technology::T->sel[j][i]) {
	/* something here */
      }
    }
  }
  pp_printf (pp, "%s polysilicon", Technology::T->poly->getName()); pp_nl;
  pp_printf (pp, "%sc poly_contact contact_X'es",
	     Technology::T->poly->getName()); pp_nl;


  for (int i=0; i < Technology::T->nmetals; i++) {
    pp_printf (pp, "m%d metal%d", i+1, i+1); pp_nl;
    if (i > 0) {
      pp_printf (pp, "m%dc metal%d metal%d via%darrow", i+1, i, i+1, i);
      pp_nl;
    }
  }

  pp_printf (pp, "boundary subcircuit"); pp_nl;
  pp_printf (pp, "error_p error_waffle"); pp_nl;
  pp_printf (pp, "error_s error_waffle"); pp_nl;
  pp_printf (pp, "error_ps error_waffle"); pp_nl;
  //pp_printf (pp, "pad overglass metal%d", Technology::T->nmetals+1); pp_nl;

  pp_endb (pp); pp_UNTAB;
  pp_printf (pp, "end"); pp_SPACE;
}


static void emit_compose (pp_t *pp)
{
  pp_printf (pp, "compose"); pp_TAB;

  for (int i=0; i < Technology::T->num_devs; i++) {
    for (int j=0; j < 2; j++) {
      pp_printf (pp, "compose %s %s %s",
		 Technology::T->fet[j][i]->getName(),
		 Technology::T->poly->getName(),
		 Technology::T->diff[j][i]->getName());
      pp_nl;

      if (Technology::T->well[j][i]) {
	pp_printf (pp, "paint %s %s %s",
		   Technology::T->diff[1-j][i]->getName(),
		   Technology::T->well[j][i]->getName(),
		   Technology::T->diff[j][i]->getName()); pp_nl;
	pp_printf (pp, "paint %sc %s %sc",
		   Technology::T->diff[1-j][i]->getName(),
		   Technology::T->well[j][i]->getName(),
		   Technology::T->diff[j][i]->getName()); pp_nl;
	pp_printf (pp, "paint %s %s %s",
		   Technology::T->fet[1-j][i]->getName(),
		   Technology::T->well[j][i]->getName(),
		   Technology::T->fet[j][i]->getName()); pp_nl;
	if (Technology::T->well[1-j][i]) {
	  pp_printf (pp, "paint %sc %s %sc",
		     Technology::T->well[1-j][i]->getName(),
		     Technology::T->well[j][i]->getName(),
		     Technology::T->well[j][i]->getName()); pp_nl;
	}
      }
    }
  }
  
  pp_UNTAB;
  pp_printf (pp, "end"); pp_SPACE;
}


static void emit_connect (pp_t *pp)
{
  Material *m1, *m2;
  pp_printf (pp, "connect"); pp_TAB;
  pp_printf (pp, "%s", Technology::T->poly->getName());
  pp_printf (pp, ",%sc/a", Technology::T->poly->getName());
  for (int i=0; i < Technology::T->num_devs; i++) {
    for (int j=0; j < 2; j++) {
      pp_printf (pp, ",%s", Technology::T->fet[j][i]->getName());
    }
  }
  pp_printf (pp, " ");
  pp_printf (pp, "%s", Technology::T->poly->getName());
  pp_printf (pp, ",%sc/a", Technology::T->poly->getName());
  for (int i=0; i < Technology::T->num_devs; i++) {
    for (int j=0; j < 2; j++) {
      pp_printf (pp, ",%s", Technology::T->fet[j][i]->getName());
    }
  }
  pp_nl;
  
  for (int i=0; i < Technology::T->num_devs; i++) {
    for (int j=0; j < 2; j++) {
      m1 = Technology::T->welldiff[j][i];
      m2 = Technology::T->well[j][i];
      if (m1 && m2) {
	pp_printf (pp, "%s,%s,%sc/a ", m1->getName(), m2->getName(),
		   m1->getName());
	pp_printf (pp, "%s,%s,%sc/a", m1->getName(), m2->getName(),
		   m1->getName());
	pp_nl;
      }

      m1 = Technology::T->diff[j][i];
      if (m1) {
	pp_printf (pp, "%s,%sc/a %s,%sc/a", m1->getName(),
		   m1->getName(), m1->getName(), m1->getName());
	pp_nl;
      }
    }
  }

  pp_printf (pp, "m1,m2c/m1,%sc/m1", Technology::T->poly->getName());
  for (int i=0; i < Technology::T->num_devs; i++) {
    for (int j=0; j < 2; j++) {
      m1 = Technology::T->welldiff[j][i];
      if (m1) {
	pp_printf (pp, ",%sc/a", m1->getName());
      }
      m1 = Technology::T->diff[j][i];
      if (m1) {
	pp_printf (pp, ",%sc/a", m1->getName());
      }
    }
  }
  pp_printf (pp, " ");
  pp_printf (pp, "m1,m2c/m1,%sc/m1", Technology::T->poly->getName());
  for (int i=0; i < Technology::T->num_devs; i++) {
    for (int j=0; j < 2; j++) {
      m1 = Technology::T->welldiff[j][i];
      if (m1) {
	pp_printf (pp, ",%sc/a", m1->getName());
      }
      m1 = Technology::T->diff[j][i];
      if (m1) {
	pp_printf (pp, ",%sc/a", m1->getName());
      }
    }
  }
  pp_nl;
  
  for (int i=2; i < Technology::T->nmetals+1; i++) {
    if (i != Technology::T->nmetals) {
      pp_printf (pp, "m%d,m%dc/m%d,m%dc/m%d m%d,m%dc/m%d,m%dc/m%d",
		 i, i, i, (i+1), i, 
		 i, i, i, (i+1), i);
      pp_nl;
    }
    else {
      pp_printf (pp, "m%d,m%dc/m%d m%d,m%dc/m%d",
		 i, i, i, i, i, i);
    }
  }
  

  pp_UNTAB;
  pp_printf (pp, "end"); pp_SPACE;
}


void emit_mzrouter (pp_t *pp)
{
  pp_printf (pp, "mzrouter"); pp_TAB;
  pp_printf (pp, "# this is a dummy section"); pp_nl;
  pp_printf (pp, "style irouter"); pp_nl;
  pp_printf (pp, "layer m1  2 1 1 1"); pp_nl;
  pp_printf (pp, "layer %s 2 2 1 1", Technology::T->poly->getName()); pp_nl;
  pp_printf (pp, "contact %sc m1 %s 10", Technology::T->poly->getName(),
	     Technology::T->poly->getName());
  pp_UNTAB;
  pp_printf (pp, "end");
  pp_SPACE;
}

void emit_cif (pp_t *pp)
{
  
}

void emit_drc (pp_t *pp)
{
}

void emit_extract (pp_t *pp)
{
  pp_printf (pp, "extract"); pp_TAB;

  pp_printf (pp, "style generic"); pp_nl;
  pp_printf (pp, "scale 1"); pp_nl;
  pp_printf (pp, "lambda 5"); pp_nl;
  pp_printf (pp, "step 100"); pp_nl;
  pp_printf (pp, "sidehalo 8"); pp_nl;
  pp_printf (pp, "rscale 1"); pp_nl;
  int order = 0;
  pp_printf (pp, "planeorder well %d", order++); pp_nl;
  pp_printf (pp, "planeorder select %d", order++); pp_nl;
  pp_printf (pp, "planeorder active %d", order++); pp_nl;
  for (int i=0; i < Technology::T->nmetals; i++) {
    pp_printf (pp, "planeorder metal%d %d", i+1, order++); pp_nl;
  }

  for (int i=0; i < Technology::T->nmetals-1; i++) {
    pp_printf (pp, "planeorder via%d %d", i+1, order++); pp_nl;
  }
  /*
   fet pfet pdiff,pdc 2 pfet Vdd! nwell 50 46
   fet pfet pdiff,pdc 1 pfet Vdd! nwell 50 46
   fet nfet ndiff,ndc 2 nfet GND! pwell 56 48
   fet nfet ndiff,ndc 1 nfet GND! pwell 56 48
  */
  

  pp_UNTAB;
  pp_printf (pp, "end");
  pp_SPACE;
}

void emit_wiring (pp_t *pp)
{
  pp_printf (pp, "wiring"); pp_TAB;

  pp_printf (pp, "# dummy"); pp_nl;
  for (int i=0; i < Technology::T->nmetals-1; i++) {
    pp_printf (pp, "contact m%dc 28 metal%d 0 metal%d 0", i+2,
	       i+1, i+2); pp_nl;
  }
  pp_printf (pp, "contact %sc 28 metal1 0 %s 0",
	     Technology::T->poly->getName(),
	     Technology::T->poly->getName());
  
  pp_UNTAB;
  pp_printf (pp, "end");
  pp_SPACE;
}

void emit_router (pp_t *pp)
{
  pp_printf (pp, "router"); pp_TAB;

  pp_printf (pp, "# dummy"); pp_nl;
  pp_printf (pp, "layer1  metal1 18 m1,m2c/m1 18"); pp_nl;
  pp_printf (pp, "layer2  metal2 20 m2c/m2,m3c,m2 20"); pp_nl;
  pp_printf (pp, "contacts m2c 28"); pp_nl;
  pp_printf (pp, "gridspacing 8");

  pp_UNTAB;
  pp_printf (pp, "end");
  pp_SPACE;
}

void emit_plowing (pp_t *pp)
{
  pp_printf (pp, "plowing"); pp_TAB;

  pp_printf (pp, "# dummy"); pp_nl;
  pp_printf (pp, "fixed %s,%s",
	     Technology::T->fet[0][0]->getName(),
	     Technology::T->fet[1][0]->getName());
  pp_nl;
  pp_printf (pp, "covered %s,%s",
	     Technology::T->fet[0][0]->getName(),
	     Technology::T->fet[1][0]->getName());
  pp_nl;
  pp_printf (pp, "drag %s,%s",
	     Technology::T->fet[0][0]->getName(),
	     Technology::T->fet[1][0]->getName());

  pp_UNTAB;
  pp_printf (pp, "end");
  pp_SPACE;
}

void emit_plot (pp_t *pp)
{
  pp_printf (pp, "plot"); pp_TAB;

  pp_printf (pp, "style pnm"); pp_TAB;

  for (int i=0; i < Technology::T->nmetals; i++) {
    pp_printf (pp, "draw metal%d", i+1); pp_nl;
  }
  pp_printf (pp, "draw polysilicon"); pp_nl;

  for (int i=0; i < Technology::T->num_devs; i++)
    for (int j=0; j < 2; j++) {
      pp_printf (pp, "draw %s", Technology::T->fet[j][i]->getName());
      pp_nl;

      pp_printf (pp, "draw %s", Technology::T->diff[j][i]->getName());
      pp_nl;
    }
  pp_UNTAB;
  pp_UNTAB;
  pp_printf (pp, "end:");
  pp_SPACE;
}


int main (int argc, char **argv)
{
  pp_t *pp;
  
  Act::Init (&argc, &argv);
  Technology::Init("layout.conf");
  
  if (Technology::T->nmetals < 2) {
    fatal_error ("Can't handle a process with fewer than two metal layers!");
  }

  pp = pp_init (stdout, 1000);

  emit_header (pp);
  emit_planes (pp);
  emit_tiletypes (pp);
  emit_contacts (pp);
  emit_styles (pp);
  emit_compose (pp);
  emit_connect (pp);
  emit_cif (pp);
  emit_mzrouter (pp);
  emit_drc (pp);
  emit_extract (pp);
  emit_wiring (pp);
  emit_router (pp);
  emit_plowing (pp);
  emit_plot (pp);
  
  
  pp_close (pp);
  
  return 0;
}
