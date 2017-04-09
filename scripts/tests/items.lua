require "lunit"

module("tests.items", package.seeall, lunit.testcase )

function setup()
    eressea.free_game()
    eressea.settings.set("nmr.timeout", "0")
    eressea.settings.set("rules.food.flags", "4")
    eressea.settings.set("rules.ship.storms", "0")
    eressea.settings.set("rules.encounters", "0")
    eressea.settings.set("magic.regeneration.enable", "0")
end

function test_mistletoe()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item('mistletoe', 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Mistelzweig")
    process_orders()
    assert_equal(1, u:get_item('mistletoe'))
    assert_equal(1, f:count_msg_type('use_item'))
    u.number = 2
    process_orders()
    assert_equal(1, u:get_item('mistletoe'))
    assert_equal(1, f:count_msg_type('use_singleperson'))
end

function test_dreameye()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("dreameye", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Traumauge")
    assert_false(u:is_cursed('skillmod'))
    turn_begin()
    turn_process()
    assert_true(u:is_cursed('skillmod'))
    assert_equal(1, u:get_item("dreameye"))
    assert_equal(1, f:count_msg_type('use_tacticcrystal'))
    turn_end()
    assert_false(u:is_cursed('skillmod'))
end

function test_manacrystal()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("manacrystal", 2)
    u:clear_orders()
    u.magic = "gwyrrd"
    u:set_skill('magic', 1)
    u.aura = 0
    u:add_order("BENUTZEN 1 Astralkristall")
    turn_begin()
    turn_process()
    assert_equal(1, u:get_item("manacrystal"))
    assert_equal(25, u.aura)
    assert_equal(1, f:count_msg_type('manacrystal_use'))
    turn_end()
end

function test_skillpotion()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("skillpotion", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Talenttrunk")
    process_orders()
    assert_equal(1, u:get_item("skillpotion"))
    assert_equal(1, f:count_msg_type('skillpotion_use'))
end

function test_studypotion()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("studypotion", 2)
    u:clear_orders()
    u:add_order("LERNE Unterhaltung")
    u:add_order("BENUTZEN 1 Lerntrank")
    process_orders()
    assert_equal(1, u:get_item("studypotion"))
end

function test_antimagic()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("antimagic", 2)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Antimagiekristall")
    process_orders()
    assert_equal(1, r:count_msg_type('use_antimagiccrystal'))
    assert_equal(1, u:get_item("antimagic"))
end

function test_ointment()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    local hp = u.hp
    u.hp = 1
    u:add_item("ointment", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Wundsalbe")
    process_orders()
    assert_equal(0, u:get_item("ointment"))
    assert_equal(1, f:count_msg_type('usepotion'))
    assert_equal(hp, u.hp)
end

function test_bloodpotion_demon()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "demon", "de")
    local u = unit.create(f, r, 1)
    u:add_item("peasantblood", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Bauernblut")
    process_orders()
    assert_equal(0, u:get_item("peasantblood"))
    assert_equal(1, f:count_msg_type('usepotion'))
    assert_equal("demon", u.race)
end

function test_bloodpotion_other()
    local r = region.create(0, 0, "plain")
    local f = faction.create("noreply@eressea.de", "human", "de")
    local u = unit.create(f, r, 1)
    u:add_item("peasantblood", 1)
    u:clear_orders()
    u:add_order("BENUTZEN 1 Bauernblut")
    process_orders()
    assert_equal(0, u:get_item("peasantblood"))
    assert_equal(1, f:count_msg_type('usepotion'))
    assert_equal("smurf", u.race)
end