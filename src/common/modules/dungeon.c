/* vi: set ts=2:
 +-------------------+  Christian Schlittchen <corwin@amber.kn-bremen.de>
 |                   |  Enno Rehling <enno@eressea-pbem.de>
 | Eressea PBEM host |  Katja Zedel <katze@felidae.kn-bremen.de>
 | (c) 1998 - 2001   |  Henning Peters <faroul@beyond.kn-bremen.de>
 |                   |  Ingo Wilken <Ingo.Wilken@informatik.uni-oldenburg.de>
 +-------------------+  Stefan Reich <reich@halbling.de>

 This program may not be used, modified or distributed 
 without prior permission by the authors of Eressea.
*/

#include <config.h>
#include <eressea.h>
#include "dungeon.h"
#include "gmcmd.h"

#include <triggers/gate.h>
#include <triggers/unguard.h>

/* kernel includes */
#include <building.h>
#include <item.h>
#include <plane.h>
#include <race.h>
#include <region.h>
#include <unit.h>

/* util includes */
#include <event.h>
#include <xml.h>

/* libc includes */
#include <string.h>

typedef struct treasure {
	const struct item_type * itype;
	int amount;
	struct treasure * next;
} treasure;

typedef struct monster {
	const struct race * race;
	double chance;
	int maxunits;
	int avgsize;
	struct treasure * treasures;
	struct monster * next;
	struct itemtype_list * weapons;
} monster;

typedef struct dungeon {
	int level;
	int radius;
	int size;
	double connect;
	struct monster * boss;
	struct monster * monsters;
	struct dungeon * next;
} dungeon;

dungeon * dungeonstyles;

region *
make_dungeon(const dungeon * data)
{
	int nb[2][3][2] = { 
		{ { -1, 0 }, { 0, 1 }, { 1, -1 } }, 
		{ { 1, 0 }, { -1, 1 }, { 0, -1 } }
	};
	const struct race * bossrace = data->boss->race;
	char name[128];
	int size = data->size;
	unsigned int flags = PFL_NORECRUITS;
	int n = 0;
	struct faction * fmonsters = findfaction(MONSTER_FACTION);
	plane * p;
	region *r, *center;
	region * rnext;
	regionlist * iregion, * rlist = NULL;

	sprintf(name, "Die H�hlen von %s", bossrace->generate_name(NULL));
	p = gm_addplane(data->radius, flags, name);

	center = findregion(p->minx+(p->maxx-p->minx)/2, p->miny+(p->maxy-p->miny)/2);
	assert(center);
	terraform(center, T_HELL);
	add_regionlist(&rlist, center);
	rnext = r = center;
	while (size>0) {
		int d, o = rand() % 3;
		for (d=0;d!=3;++d) {
			int index = (d+o) % 3;
			region * rn = findregion(r->x+nb[n][index][0], r->y+nb[n][index][1]);
			assert(r->terrain==T_HELL);
			if (rn) {
				switch (rn->terrain) {
				case T_OCEAN:
					if (rand() % 100 < data->connect*100) {
						terraform(rn, T_HELL);
						--size;
						rnext = rn;
						add_regionlist(&rlist, rn);
					}
					else terraform(rn, T_FIREWALL);
					break;
				case T_HELL:
					rnext = rn;
					break;
				}
				if (size == 0) break;
			}
			rn = findregion(r->x+nb[(n+1)%2][index][0], r->y+nb[(n+1)%2][index][1]);
			if (rn && rn->terrain==T_OCEAN) terraform(rn, T_FIREWALL);
		}
		if (size==0) break;
		assert(r!=rnext);
		r = rnext;
		n = (n+1) % 2;
	}

	for (iregion=rlist;iregion;iregion=iregion->next) {
		monster * m = data->monsters;
		region * r = iregion->region;
		while (m) {
			if ((rand() % 100) < (m->chance * 100)) {
				/* TODO: check maxunits. */
				treasure * loot = m->treasures;
				struct itemtype_list * weapon = m->weapons;
				int size = 1 + (rand() % m->avgsize) + (rand() % m->avgsize);
				unit * u = createunit(r, fmonsters, size, m->race);
				while (weapon) {
					i_change(&u->items, weapon->type, size);
					weapon = weapon->next;
				}
				while (loot) {
					i_change(&u->items, loot->itype, loot->amount*size);
					loot = loot->next;
				}
			}
			m = m->next;
		}
	}
	return center;
}

void
make_dungeongate(region * source, region * target, const struct dungeon * d)
{
	building *bsource, *btarget;
	
	bsource = new_building(bt_find("castle"), source, default_locale);
	set_string(&bsource->name, "Pforte zur H�lle");
	bsource->size = 50;
	add_trigger(&bsource->attribs, "timer", trigger_gate(bsource, target));
	add_trigger(&bsource->attribs, "create", trigger_unguard(bsource));
	fset(bsource, BLD_UNGUARDED);

	btarget = new_building(bt_find("castle"), target, default_locale);
	set_string(&btarget->name, "Pforte zur Au�enwelt");
	btarget->size = 50;
	add_trigger(&btarget->attribs, "timer", trigger_gate(btarget, source));
	add_trigger(&btarget->attribs, "create", trigger_unguard(btarget));
	fset(btarget, BLD_UNGUARDED);
}

static int
tagbegin(xml_stack *stack)
{
	xml_tag * tag = stack->tag;
	if (strcmp(tag->name , "dungeon")) {
		stack->state = calloc(sizeof(dungeon), 1);
	} else {
		dungeon * d = (dungeon*)stack->state;
		if (strcmp(tag->name, "monster")) {
			monster * m = calloc(sizeof(monster), 1);
			m->race = rc_find(xml_value(tag, "race"));
			m->chance = xml_fvalue(tag, "chance");
			m->avgsize = xml_ivalue(tag, "avgsize");
			m->maxunits = xml_ivalue(tag, "maxunits");

			if (m->race) {
				if (xml_bvalue(tag, "boss")) {
					d->boss = m;
				} else {
					m->next = d->monsters;
					d->monsters = m;
				}
			}
		} else if (strcmp(tag->name, "weapon")) {
			monster * m = d->monsters;
			itemtype_list * w = calloc(sizeof(itemtype_list), 1);
			w->type = it_find(xml_value(tag, "type"));
			if (w->type) {
				w->next = m->weapons;
				m->weapons = w;
			}
		}
	}
	return XML_OK;
}

static int
tagend(xml_stack * stack)
{
	xml_tag * tag = stack->tag;
	if (strcmp(tag->name, "dungeon")) {
		dungeon * d = (dungeon*)stack->state;
		stack->state = NULL;
		d->next = dungeonstyles;
		dungeonstyles = d;
	}
	return XML_OK;
}

xml_callbacks xml_dungeon = {
	tagbegin, tagend, NULL
};

void
register_dungeon(void)
{
	xml_register(&xml_dungeon, "eressea dungeon", 0);
}