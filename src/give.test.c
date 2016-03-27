#include <platform.h>

#include "give.h"
#include "economy.h"

#include <kernel/ally.h>
#include <kernel/config.h>
#include <kernel/faction.h>
#include <kernel/item.h>
#include <kernel/order.h>
#include <kernel/race.h>
#include <kernel/region.h>
#include <kernel/terrain.h>
#include <kernel/unit.h>

#include <util/base36.h>
#include <util/language.h>
#include <util/message.h>

#include <CuTest.h>
#include <tests.h>
#include <stdio.h>
#include <assert.h>

struct give {
    struct unit *src, *dst;
    struct region *r;
    struct faction *f1, *f2;
    struct item_type * itype;
};

static void setup_give(struct give *env) {
    struct terrain_type *ter = test_create_terrain("plain", LAND_REGION);
    race *rc;

    assert(env->f1);
    rc = test_create_race(env->f1->race ? env->f1->race->_name : "humon");
    rc->ec_flags |= GIVEPERSON;

    env->r = test_create_region(0, 0, ter);
    env->src = test_create_unit(env->f1, env->r);
    env->dst = env->f2 ? test_create_unit(env->f2, env->r) : 0;
    env->itype = it_get_or_create(rt_get_or_create("money"));
    env->itype->flags |= ITF_HERB;
    if (env->f1 && env->f2) {
        ally * al = ally_add(&env->f2->allies, env->f1);
        al->status = HELP_GIVE;
    }
}

static void test_give_unit_to_peasants(CuTest * tc) {
    struct give env;
    test_cleanup();
    env.f1 = test_create_faction(0);
    env.f2 = 0;
    setup_give(&env);
    rsetpeasants(env.r, 0);
    give_unit(env.src, NULL, NULL);
    CuAssertIntEquals(tc, 0, env.src->number);
    CuAssertIntEquals(tc, 1, rpeasants(env.r));
    test_cleanup();
}

static void test_give_unit(CuTest * tc) {
    struct give env;
    test_cleanup();
    env.f1 = test_create_faction(0);
    env.f2 = test_create_faction(0);
    setup_give(&env);
    env.r->terrain = test_create_terrain("ocean", SEA_REGION);
    config_set("rules.give.max_men", "0");
    give_unit(env.src, env.dst, NULL);
    CuAssertPtrEquals(tc, env.f1, env.src->faction);
    CuAssertIntEquals(tc, 0, env.f2->newbies);
    config_set("rules.give.max_men", "-1");
    give_unit(env.src, env.dst, NULL);
    CuAssertPtrEquals(tc, env.f2, env.src->faction);
    CuAssertIntEquals(tc, 1, env.f2->newbies);
    CuAssertPtrEquals(tc, 0, env.f1->units);
    test_cleanup();
}

static void test_give_unit_in_ocean(CuTest * tc) {
    struct give env;
    test_cleanup();
    env.f1 = test_create_faction(0);
    env.f2 = 0;
    setup_give(&env);
    env.r->terrain = test_create_terrain("ocean", SEA_REGION);
    give_unit(env.src, NULL, NULL);
    CuAssertIntEquals(tc, 0, env.src->number);
    test_cleanup();
}

static void test_give_men(CuTest * tc) {
    struct give env;
    test_cleanup();
    env.f2 = env.f1 = test_create_faction(0);
    setup_give(&env);
    CuAssertPtrEquals(tc, 0, give_men(1, env.src, env.dst, NULL));
    CuAssertIntEquals(tc, 2, env.dst->number);
    CuAssertIntEquals(tc, 0, env.src->number);
    test_cleanup();
}

static void test_give_men_limit(CuTest * tc) {
    struct give env;
    message *msg;
    test_cleanup();
    env.f2 = test_create_faction(0);
    env.f1 = test_create_faction(0);
    setup_give(&env);
    config_set("rules.give.max_men", "1");

    /* below the limit, give men, increase newbies counter */
    usetcontact(env.dst, env.src);
    msg = give_men(1, env.src, env.dst, NULL);
    CuAssertStrEquals(tc, "give_person", (const char *)msg->parameters[0].v);
    CuAssertIntEquals(tc, 2, env.dst->number);
    CuAssertIntEquals(tc, 0, env.src->number);
    CuAssertIntEquals(tc, 1, env.f2->newbies);
    msg_release(msg);

    /* beyond the limit, do nothing */
    usetcontact(env.src, env.dst);
    msg = give_men(2, env.dst, env.src, NULL);
    CuAssertStrEquals(tc, "error129", (const char *)msg->parameters[3].v);
    CuAssertIntEquals(tc, 2, env.dst->number);
    CuAssertIntEquals(tc, 0, env.src->number);
    CuAssertIntEquals(tc, 0, env.f1->newbies);
    msg_release(msg);

    test_cleanup();
}

static void test_give_men_in_ocean(CuTest * tc) {
    struct give env;
    message * msg;

    test_cleanup();
    env.f1 = test_create_faction(0);
    env.f2 = 0;
    setup_give(&env);
    env.r->terrain = test_create_terrain("ocean", SEA_REGION);
    msg = disband_men(1, env.src, NULL);
    CuAssertStrEquals(tc, "give_person_ocean", (const char *)msg->parameters[0].v);
    CuAssertIntEquals(tc, 0, env.src->number);
    msg_release(msg);
    test_cleanup();
}

static void test_give_men_too_many(CuTest * tc) {
    struct give env;
    test_cleanup();
    env.f2 = env.f1 = test_create_faction(0);
    setup_give(&env);
    CuAssertPtrEquals(tc, 0, give_men(2, env.src, env.dst, NULL));
    CuAssertIntEquals(tc, 2, env.dst->number);
    CuAssertIntEquals(tc, 0, env.src->number);
    test_cleanup();
}

static void test_give_men_none(CuTest * tc) {
    struct give env;
    message * msg;

    test_cleanup();
    env.f2 = env.f1 = test_create_faction(0);
    setup_give(&env);
    msg = give_men(0, env.src, env.dst, NULL);
    CuAssertStrEquals(tc, "error96", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 1, env.dst->number);
    CuAssertIntEquals(tc, 1, env.src->number);
    msg_release(msg);
    test_cleanup();
}

static void test_give_men_other_faction(CuTest * tc) {
    struct give env;
    message * msg;

    test_cleanup();
    env.f1 = test_create_faction(0);
    env.f2 = test_create_faction(0);
    setup_give(&env);
    usetcontact(env.dst, env.src);
    msg = give_men(1, env.src, env.dst, NULL);
    CuAssertStrEquals(tc, "give_person", (const char *)msg->parameters[0].v);
    CuAssertIntEquals(tc, 2, env.dst->number);
    CuAssertIntEquals(tc, 0, env.src->number);
    msg_release(msg);
    test_cleanup();
}

static void test_give_men_requires_contact(CuTest * tc) {
    struct give env;
    message * msg;
    order *ord;
    char cmd[32];

    test_cleanup();
    env.f1 = test_create_faction(0);
    env.f2 = test_create_faction(0);
    setup_give(&env);
    msg = give_men(1, env.src, env.dst, NULL);
    CuAssertStrEquals(tc, "feedback_no_contact", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 1, env.dst->number);
    CuAssertIntEquals(tc, 1, env.src->number);

    _snprintf(cmd, sizeof(cmd), "%s ALLES PERSONEN", itoa36(env.dst->no));
    ord = create_order(K_GIVE, env.f1->locale, cmd);
    test_clear_messages(env.f1);
    give_cmd(env.src, ord);
    CuAssertPtrEquals(tc, 0, test_find_messagetype(env.f1->msgs, "give_person"));
    CuAssertPtrNotNull(tc, test_find_messagetype(env.f1->msgs, "feedback_no_contact"));

    msg_release(msg);
    free_order(ord);
    test_cleanup();
}

static void test_give_men_not_to_self(CuTest * tc) {
    struct give env;
    message * msg;
    test_cleanup();
    env.f2 = env.f1 = test_create_faction(0);
    setup_give(&env);
    msg = give_men(1, env.src, env.src, NULL);
    CuAssertStrEquals(tc, "error10", test_get_messagetype(msg));
    CuAssertIntEquals(tc, 1, env.src->number);
    msg_release(msg);
    test_cleanup();
}

static void test_give_peasants(CuTest * tc) {
    struct give env;
    message * msg;

    test_cleanup();
    env.f1 = test_create_faction(0);
    env.f2 = 0;
    setup_give(&env);
    rsetpeasants(env.r, 0);
    msg = disband_men(1, env.src, NULL);
    CuAssertStrEquals(tc, "give_person_peasants", (const char*)msg->parameters[0].v);
    CuAssertIntEquals(tc, 0, env.src->number);
    CuAssertIntEquals(tc, 1, rpeasants(env.r));
    msg_release(msg);
    test_cleanup();
}

static void test_give(CuTest * tc) {
    struct give env;

    test_cleanup();
    env.f2 = env.f1 = test_create_faction(0);
    setup_give(&env);

    i_change(&env.src->items, env.itype, 10);
    CuAssertIntEquals(tc, 0, give_item(10, env.itype, env.src, env.dst, 0));
    CuAssertIntEquals(tc, 0, i_get(env.src->items, env.itype));
    CuAssertIntEquals(tc, 10, i_get(env.dst->items, env.itype));

    CuAssertIntEquals(tc, -1, give_item(10, env.itype, env.src, env.dst, 0));
    CuAssertIntEquals(tc, 0, i_get(env.src->items, env.itype));
    CuAssertIntEquals(tc, 10, i_get(env.dst->items, env.itype));
    test_cleanup();
}

static void test_give_herbs(CuTest * tc) {
    struct give env;
    struct order *ord;
    char cmd[32];

    test_cleanup();
    test_create_world();
    env.f2 = env.f1 = test_create_faction(0);
    setup_give(&env);
    i_change(&env.src->items, env.itype, 10);

    _snprintf(cmd, sizeof(cmd), "%s %s", itoa36(env.dst->no), LOC(env.f1->locale, parameters[P_HERBS]));
    ord = create_order(K_GIVE, env.f1->locale, cmd);
    assert(ord);

    give_cmd(env.src, ord);
    CuAssertIntEquals(tc, 0, i_get(env.src->items, env.itype));
    CuAssertIntEquals(tc, 10, i_get(env.dst->items, env.itype));
    free_order(ord);
    test_cleanup();
}

static void test_give_okay(CuTest * tc) {
    struct give env;

    test_cleanup();
    env.f2 = env.f1 = test_create_faction(0);
    setup_give(&env);

    config_set("rules.give.flags", "0");
    CuAssertPtrEquals(tc, 0, check_give(env.src, env.dst, 0));
    test_cleanup();
}

static void test_give_denied_by_rules(CuTest * tc) {
    struct give env;
    struct message *msg;

    test_cleanup();
    env.f1 = test_create_faction(0);
    env.f2 = test_create_faction(0);
    setup_give(&env);

    config_set("rules.give.flags", "0");
    CuAssertPtrNotNull(tc, msg = check_give(env.src, env.dst, 0));
    msg_release(msg);
    test_cleanup();
}

static void test_give_invalid_target(CuTest *tc) {
    // bug https://bugs.eressea.de/view.php?id=1685
    struct give env;
    order *ord;

    test_cleanup();
    env.f1 = test_create_faction(0);
    env.f2 = 0;
    setup_give(&env);

    i_change(&env.src->items, env.itype, 10);
    ord = create_order(K_GIVE, env.f1->locale, "## KRAUT");
    assert(ord);

    give_cmd(env.src, ord);
    CuAssertIntEquals(tc, 10, i_get(env.src->items, env.itype));
    CuAssertPtrNotNull(tc, test_find_messagetype(env.f1->msgs, "feedback_unit_not_found"));
    free_order(ord);
    test_cleanup();
}

CuSuite *get_give_suite(void)
{
    CuSuite *suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, test_give);
    SUITE_ADD_TEST(suite, test_give_men);
    SUITE_ADD_TEST(suite, test_give_men_limit);
    SUITE_ADD_TEST(suite, test_give_men_in_ocean);
    SUITE_ADD_TEST(suite, test_give_men_none);
    SUITE_ADD_TEST(suite, test_give_men_too_many);
    SUITE_ADD_TEST(suite, test_give_men_other_faction);
    SUITE_ADD_TEST(suite, test_give_men_requires_contact);
    SUITE_ADD_TEST(suite, test_give_men_not_to_self);
    SUITE_ADD_TEST(suite, test_give_unit);
    SUITE_ADD_TEST(suite, test_give_unit_in_ocean);
    SUITE_ADD_TEST(suite, test_give_unit_to_peasants);
    SUITE_ADD_TEST(suite, test_give_peasants);
    SUITE_ADD_TEST(suite, test_give_herbs);
    SUITE_ADD_TEST(suite, test_give_okay);
    SUITE_ADD_TEST(suite, test_give_denied_by_rules);
    SUITE_ADD_TEST(suite, test_give_invalid_target);
    return suite;
}
