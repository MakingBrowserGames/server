/* vi: set ts=2:
 *
 *	Eressea PB(E)M host Copyright (C) 1998-2003
 *      Christian Schlittchen (corwin@amber.kn-bremen.de)
 *      Katja Zedel (katze@felidae.kn-bremen.de)
 *      Henning Peters (faroul@beyond.kn-bremen.de)
 *      Enno Rehling (enno@eressea-pbem.de)
 *      Ingo Wilken (Ingo.Wilken@informatik.uni-oldenburg.de)
 *
 * This program may not be used, modified or distributed without
 * prior permission by the authors of Eressea.
 */

#ifndef MAPPER_H
#define MAPPER_H

#define INPUT_BUFSIZE 80

#include <terrain.h>

#ifndef ISLANDSIZE
# define ISLANDSIZE      ((rand()%3)?(25+rand()%10):(11+rand()%14))
#endif

#ifndef NCURSES_CONST
#ifdef __PDCURSES__
# define NCURSES_CONST
#else
# define NCURSES_CONST const
#endif
#endif

struct race;

typedef struct dbllist dbllist;
struct dbllist {
	dbllist *next, *prev;
	char s[1];
};

typedef struct tagregion {
	struct tagregion *next;
	struct region *r;
} tagregion;

extern tagregion *Tagged;

void saddstr(char *s);
struct region *SeedPartei(void);
void Exit(int level);
int showunits(struct region * r);
void showregion(struct region * r, char full);
int modify_region(struct region * r);
void NeueBurg(struct region * r);
void NeuesSchiff(struct region * r);
void create_island(struct region *r, int n, terrain_t t);
void make_ocean_block(int x, int y);
void make_new_block(int x, int y);
void moveln(const int x);
char *my_input(WINDOW * win, int x, int y, const char *text, const char *def);
void make_new_region(int x, int y);
int map_input(WINDOW * win, int x, int y, const char *text, int mn, int mx, int pre);
boolean yes_no(WINDOW * win, const char *text, const char def);
void warnung(WINDOW * win, const char *text);
FILE *mapperFopen(const char *defName, const char *mode);
void adddbllist(dbllist ** S, const char *s);
void ScrollRegList(int dir);
void DisplayRegList(int neu);
char GetTerrain(struct region * r);
void NeuePartei(struct region * r);
void RemovePartei(void);
int ParteiListe(void);
int koor_distance(int a, int b, int x, int y);
int create_backup(char *file);
void SpecialFunction(struct region *r);

extern WINDOW *mywin;
extern dbllist *reglist;
extern int MINX, MINY, MAXX, MAXY, pline;
extern dbllist *pointer;
extern char modified;
extern char *datadir;
extern struct unit *clipunit;
extern struct region *clipregion;
extern struct ship *clipship;
#define SX (stdscr->_maxx-1)
#define SY (stdscr->_maxy-1)

#define NL(S) adddbllist(&S," ")

#define wAddstr(x) waddnstr(win, (NCURSES_CONST char*)x,-1)
#define Addstr(x) waddnstr(mywin, (NCURSES_CONST char*)x,-1)
#define Movexy(x,y) wmove(mywin,y,x)
#define movexy(x,y) move(y,x)
/* move(zeile, spalte) ist "verkehrt"... */

extern WINDOW * openwin(int b, int h, const char* t);

#define S_SIGWINCH 1

char *Unitid(struct unit * u);
char *Buildingid(struct building * b);
char *Shipid(struct ship * sh);
char *BuildingName(struct building * b);

/* map_tools */
typedef struct selection {
	struct selection * next;
	struct selection * prev;
	int index;
	char * str;
	void * data;
} selection;

struct selection * do_selection(struct selection * sel, const char * title, void (*perform)(struct selection *, void *), void * data);
struct selection ** push_selection(struct selection ** p_sel, char * str, void * payload);
void insert_selection(struct selection ** p_sel, struct selection * prev, char * str, void * payload);
void block_create(int x1, int y1, int size, char chaotisch, int special, char terrain);

extern void read_orders(const char * filename);
extern void read_dropouts(const char *filename);
extern void seed_dropouts(void);
extern int numnewbies;

#define sncat(b, s, size) strncat ((b), s, size - strlen (b))

typedef struct dropout {
	struct dropout * next;
	const struct race * race;
	int x, y, fno;
} dropout;

extern dropout * dropouts;
extern struct newfaction * newfactions;

#endif				/* MAPPER_H */
