"""3D models of the inventory items, built from scripts/inventory_mesh.py parts.

Axes: X runs left to right, Y up, Z towards the viewer. Units are arbitrary;
each model is centred and scaled when compiled. Models are stylised low-poly
interpretations of the item shapes, not extracted game meshes.
"""
from inventory_mesh import (along_x, along_z, box, cylinder, extrude, frustum, lathe, move, ring,
                            rotate, rounded_rectangle, scale, sphere)


def barrel(x0, x1, y, radius, z=0, segments=10):
    return move(along_x(cylinder(radius, 0, x1 - x0, segments)), x0, y, z)


# Weapons ------------------------------------------------------------------

def pistol_10mm():
    return [
        box(-50, 14, -6, 48, 30, 6),                                                  # slide
        extrude([(-42, 15), (-18, 15), (-24, -28), (-44, -31), (-46, -26)], -7, 7),   # grip
        extrude([(-19, 14), (-16, 14), (-16, 5), (-3, 5), (-3, 14), (0, 14), (0, 2), (-19, 2)], -2.5, 2.5),  # trigger guard
        barrel(48, 60, 22, 4),
        box(-46, -37, -6, -24, -31, 6),                                               # magazine base
        box(40, 30, -1.5, 46, 34, 1.5), box(-48, 30, -3, -42, 34, 3),                 # sights
    ]


def laser_rifle():
    return [
        box(-40, 0, -6, 40, 14, 6),
        barrel(40, 95, 7, 4),
        move(along_x(cylinder(7, 0, 4)), 55, 7, 0), move(along_x(cylinder(7, 0, 4)), 70, 7, 0),
        move(along_x(cylinder(7, 0, 4)), 85, 7, 0),
        extrude([(-40, 12), (-95, 6), (-100, -14), (-86, -16), (-40, 0)], -5, 5),     # stock
        extrude([(-20, 0), (-8, 0), (-12, -26), (-24, -26)], -5, 5),                  # grip
        barrel(-12, 25, 21, 5),                                                       # scope
        box(5, -10, -5, 22, 0, 5),                                                    # fusion cell
    ]


def combat_shotgun():
    return [
        box(-30, -4, -6, 30, 12, 6),
        barrel(30, 95, 8, 4),
        box(35, -6, -6, 62, 4, 6),                                                    # pump grip
        box(0, -24, -5, 14, -4, 5),                                                   # box magazine
        extrude([(-30, 10), (-95, 4), (-98, -16), (-88, -18), (-30, -4)], -5, 5),     # stock
        extrude([(-22, -4), (-10, -4), (-14, -26), (-26, -26)], -5, 5),               # grip
        box(88, 12, -1.5, 93, 16, 1.5),                                               # front sight
    ]


def hunting_rifle():
    return [
        extrude([(-100, -14), (-100, 4), (-70, 6), (60, 6), (60, -2), (20, -4), (-20, -4), (-30, -10),
                 (-45, -12), (-60, -6), (-92, -16)], -5, 5),                         # wooden stock
        barrel(30, 112, 4, 3),
        move(along_x(lathe([(6, -30), (6, -22), (4, -16), (4, 18), (7, 24), (7, 32)], 10)), 0, 18, 0),  # scope
        box(-22, 6, -2, -16, 14, 2), box(14, 6, -2, 20, 14, 2),                       # scope mounts
        move(along_z(cylinder(1.5, 0, 12, 8)), -30, 6, 4), move(sphere(3, 4, 8), -30, 6, 17),  # bolt handle
    ]


def fat_man():
    return [
        barrel(-55, 60, 0, 10, segments=12),
        move(along_x(frustum(10, 14, 0, 12, 12)), 60, 0, 0),                         # muzzle
        move(along_x(cylinder(13, 0, 25, 12)), -65, 0, 0),                            # rear housing
        extrude([(-10, -10), (0, -10), (-4, -32), (-14, -32)], -4, 4),                # grips
        extrude([(25, -10), (35, -10), (31, -30), (21, -30)], -4, 4),
        extrude([(-25, 10), (-20, 10), (-20, 18), (20, 18), (20, 10), (25, 10), (25, 22), (-25, 22)], -3, 3),  # carry handle
        box(-40, 10, -4, -30, 16, 4),                                                 # sight
    ]


def super_sledge():
    return [
        barrel(-85, 40, 0, 3),
        move(along_x(cylinder(4.5, 0, 25, 10)), -85, 0, 0),                           # grip wrap
        box(40, -18, -14, 72, 18, 14),                                                # head
        move(along_x(frustum(10, 7, 0, 16, 12)), 72, 0, 0),                           # rocket
        move(along_x(cylinder(8, 0, 6, 12)), 34, 0, 0),                               # collar
        box(46, 18, -8, 66, 22, 8),
    ]


# Apparel ------------------------------------------------------------------

def jumpsuit_outline():
    return [(-15, 60), (15, 60), (35, 52), (48, 10), (38, 7), (28, 38), (26, 0), (30, -70), (8, -70), (0, -12),
            (-8, -70), (-30, -70), (-26, 0), (-28, 38), (-38, 7), (-48, 10), (-35, 52)]


def vault_111_jumpsuit():
    return [
        extrude(jumpsuit_outline(), -8, 8),
        box(-28, -4, -9.5, 28, 4, 9.5),                                               # belt
        extrude([(-15, 60), (0, 46), (15, 60), (10, 66), (0, 56), (-10, 66)], -9, 9),  # collar
        box(-3, -1, 9.5, 3, 3, 11),                                                   # buckle
    ]


def leather_chest():
    torso = scale(lathe([(24, -32), (28, -10), (30, 14), (22, 30)], 12), z=0.6)
    straps = [scale(cylinder(29.5, y, y + 4, 12), z=0.62) for y in (-24, -12, 0)]
    pads = [move(rotate(box(-12, -4, -12, 12, 4, 12), 'z', angle), x, 28, 0) for x, angle in ((-24, 25), (24, -25))]
    return [torso] + straps + pads


def combat_chest():
    plate = extrude([(-28, -32), (28, -32), (32, 18), (20, 32), (-20, 32), (-32, 18)], -12, 12)
    return [
        plate,
        extrude([(-22, -4), (22, -4), (24, 20), (0, 26), (-24, 20)], 12, 16),         # chest plate
        box(-20, -28, 12, 20, -8, 15),                                                # abdominal plate
        move(rotate(box(-14, -5, -14, 14, 5, 14), 'z', 28), -30, 30, 0),             # pauldrons
        move(rotate(box(-14, -5, -14, 14, 5, 14), 'z', -28), 30, 30, 0),
    ]


def road_leathers():
    jacket = [(-16, 60), (16, 60), (36, 52), (50, 4), (40, 2), (30, 34), (30, -10), (-30, -10), (-30, 34), (-40, 2),
              (-50, 4), (-36, 52)]
    return [
        extrude(jacket, -9, 9),
        extrude([(-28, -10), (28, -10), (30, -70), (8, -70), (0, -22), (-8, -70), (-30, -70)], -8, 8),  # trousers
        extrude([(-16, 60), (-6, 30), (-22, 54)], 9, 12), extrude([(16, 60), (22, 54), (6, 30)], 9, 12),  # lapels
        box(-30, -14, -10, 30, -8, 10),                                               # belt
    ]


def army_helmet():
    return [
        lathe([(0, 30), (14, 28.5), (23, 22), (28, 12), (30, 2), (36, -4), (36, -7), (0, -7)], 14),
        box(-2, -7, 26, 2, 4, 31),                                                    # chin strap clip
    ]


def t60_helmet():
    shell = rotate(extrude([(-20, -20), (24, -20), (28, 0), (22, 24), (0, 31), (-22, 22), (-26, 0)], -23, 23), 'y', -90)
    return [
        shell,
        box(-16, 2, 24, 16, 13, 30),                                                  # visor
        box(-10, -18, 22, 10, -4, 31),                                                # breather
        move(along_z(cylinder(5, 0, 6, 10)), -14, -12, 26), move(along_z(cylinder(5, 0, 6, 10)), 14, -12, 26),
        move(along_x(cylinder(8, 0, 6, 10)), 23, 2, -2), move(along_x(cylinder(8, 0, 6, 10)), -29, 2, -2),
        box(-4, 28, -18, 4, 34, 14),                                                  # crest
    ]


# Aid ----------------------------------------------------------------------

def stimpak():
    return [
        move(along_x(cylinder(6, 0, 45, 12)), -20, 0, 0),                             # barrel
        move(along_x(cylinder(2, 0, 22, 8)), -42, 0, 0),                              # plunger rod
        move(along_x(cylinder(8, 0, 3, 12)), -45, 0, 0),                              # thumb disc
        box(-22, -12, -2.5, -18, 12, 2.5),                                            # finger flange
        move(along_x(frustum(4, 1, 0, 8, 10)), 25, 0, 0),                             # hub
        move(along_x(frustum(0.9, 0.3, 0, 22, 6)), 33, 0, 0),                         # needle
        move(cylinder(4.5, 0, 6, 12), 4, 5, 0),                                       # gauge
    ]


def radaway():
    bag = extrude(rounded_rectangle(44, 60, 9), -6, 6)
    return [
        bag,
        move(extrude(rounded_rectangle(32, 22, 5), 6, 8), 0, -6, 0),                  # label
        box(-6, 30, -3, 6, 36, 3), ring(2, 5, 36, 39, 10),                            # hanger
        move(cylinder(2.5, 0, 14, 8), 0, -44, 0),                                     # outlet
        move(rotate(cylinder(1.6, 0, 26, 8), 'z', -60), 0, -44, 0),                    # tube
    ]


def rad_x():
    return [
        lathe([(0, -20), (12, -20), (12, 10), (10, 12), (10, 14), (0, 14)], 14),      # bottle
        lathe([(11, 13), (12, 13), (12, 24), (0, 24)], 14),                           # cap
        cylinder(12.4, -10, 4, 14),                                                   # label band
    ]


def nuka_cola():
    return [
        lathe([(0, -40), (10, -40), (11, -36), (11, -10), (9, -2), (10, 6), (8, 14), (4, 26), (4, 32), (0, 32)], 14),
        lathe([(5, 32), (5.5, 32), (5.5, 37), (0, 37)], 12),                          # cap
    ]


def mentats():
    tin = rotate(extrude(rounded_rectangle(60, 40, 8), -5, 5), 'x', -90)
    lid = rotate(extrude(rounded_rectangle(62, 42, 8), 3, 7), 'x', -90)
    return [tin, lid, box(-20, 7, -10, 20, 8.5, 10)]


def purified_water():
    return [
        lathe([(0, -40), (13, -40), (14, -36), (14, 6), (11, 16), (5, 24), (5, 28), (0, 28)], 14),
        lathe([(6, 27), (6.5, 27), (6.5, 34), (0, 34)], 12),
        cylinder(14.6, -24, -20, 14), cylinder(14.6, -12, -8, 14),                    # ribs
    ]


# Misc ---------------------------------------------------------------------

def bobby_pin():
    wave = [(-48, -3), (-30, -3), (-26, -6), (-22, -3), (-14, -3), (-10, -6), (-6, -3), (2, -3), (6, -6), (10, -3), (40, -3)]
    lower = extrude(wave + [(x, y - 2) for x, y in reversed(wave)], -1, 1)        # wavy prong
    return [
        box(-48, 3, -1, 44, 5, 1),                                                    # straight prong
        lower,
        move(along_z(ring(3, 5, -1, 1, 12)), -48, 0, 0),                              # bend
        move(sphere(1.6, 4, 8), 44, 4, 0), move(sphere(1.6, 4, 8), 40, -4, 0),        # rubber tips
    ]


def holotape():
    return [
        extrude(rounded_rectangle(80, 52, 6), -6, 6),
        box(-30, 8, 6, 30, 22, 7.5),                                                  # label
        move(along_z(cylinder(7, 0, 2, 12)), -18, -8, 6), move(along_z(cylinder(7, 0, 2, 12)), 18, -8, 6),
        box(-8, -14, 6, 8, -2, 7),                                                    # window
    ]


def overdue_book():
    return [
        box(-30, -40, -10, 30, 40, -8), box(-30, -40, 8, 30, 40, 10),                 # covers
        box(-33, -40, -10, -30, 40, 10),                                              # spine
        box(-30, -38, -8, 28, 38, 8),                                                 # pages
        box(-20, 18, 10, 20, 28, 11),                                                 # title plate
    ]


def book_token():
    coin = along_z(lathe([(0, -1.5), (20, -1.5), (20, 1.5), (17, 1.5), (17, 1), (0, 1)], 20))
    return [coin, box(-8, -6, 1, 8, 6, 2.5), box(-1, -6, 2.5, 1, 6, 3)]


def subway_token():
    return [along_z(ring(6, 18, -1.5, 1.5, 20)), along_z(ring(13, 18, 1.5, 2.2, 20))]


def bobblehead():
    return [
        lathe([(0, -10), (16, -10), (16, -4), (12, 0), (0, 0)], 14),                  # base
        cylinder(6, 0, 22, 10),                                                       # body
        box(-14, 14, -3, -6, 18, 3), box(6, 14, -3, 14, 18, 3),                       # arms
        cylinder(2, 22, 27, 8),                                                       # spring neck
        move(sphere(13, 6, 12), 0, 39, 0),                                            # head
        move(frustum(5, 0, 0, 8, 8), 4, 50, 2),                                       # hair curl
    ]


# Junk ---------------------------------------------------------------------

def duct_tape():
    return [along_z(ring(12, 22, -7, 7, 16)), box(14, -26, -6.5, 34, -22.5, 6.5)]


def desk_fan():
    blades = [rotate(scale(box(-3, 4, -1, 3, 20, 1), 1.6, 1, 1), 'z', a) for a in (0, 120, 240)]
    return [
        lathe([(0, -40), (16, -40), (16, -36), (0, -36)], 14),                        # base
        cylinder(2.5, -36, -10, 8),                                                   # stem
        along_z(lathe([(0, -14), (9, -14), (10, -4), (0, -4)], 12)),                  # motor
        along_z(ring(22, 24, -2, 2, 20)), along_z(ring(22, 24, 6, 10, 20)),           # cage rims
    ] + [move(b, 0, 0, 4) for b in blades]


def wonderglue():
    return [
        lathe([(0, -30), (8, -30), (8, 10), (4, 18), (3, 18), (3, 24), (0, 24)], 12),
        box(-9, -36, -2, 9, -30, 2),                                                  # crimped end
        lathe([(4, 22), (4.5, 22), (4.5, 32), (0, 32)], 10),                          # cap
    ]


def alarm_clock():
    return [
        along_z(lathe([(0, -6), (20, -6), (21, 0), (20, 6), (0, 6)], 16)),           # body
        along_z(ring(17, 21, 6, 8, 16)),                                              # bezel
        move(rotate(lathe([(0, 9), (5, 8), (8, 4), (9, 0), (0, 0)], 10), 'z', 35), -14, 18, 0),  # bells
        move(rotate(lathe([(0, 9), (5, 8), (8, 4), (9, 0), (0, 0)], 10), 'z', -35), 14, 18, 0),
        box(-1.5, 18, -1.5, 1.5, 28, 1.5),                                            # hammer
        move(cylinder(2.5, 0, 6, 8), -12, -26, 0), move(cylinder(2.5, 0, 6, 8), 12, -26, 0),  # feet
        box(-1, 0, 7, 1, 14, 8), box(0, -1, 7, 10, 1, 8),                            # hands
    ]


def typewriter():
    body = rotate(extrude([(-24, -10), (24, -10), (20, 6), (-6, 14), (-24, 18)], -40, 40), 'y', 90)
    keys = [box(-32, -6 + 3*r, 4 + 7*r, 32, -3 + 3*r, 9 + 7*r) for r in range(3)]
    return [
        body,
        move(along_x(cylinder(5, 0, 90, 12)), -45, 22, -14),                          # platen
        box(-30, 26, -20, 30, 52, -18),                                               # paper
        move(along_x(cylinder(2, 0, 8, 8)), -53, 22, -14),                            # carriage knob
    ] + keys


def telephone():
    body = rotate(extrude([(-22, -16), (22, -16), (16, 10), (-16, 10)], -26, 26), 'y', 90)
    handset = extrude([(-40, 14), (40, 14), (44, 22), (34, 30), (26, 22), (-26, 22), (-34, 30), (-44, 22)], -6, 6)
    dial = move(rotate(cylinder(12, 0, 3, 16), 'x', 70), 0, -2, 12)
    return [body, handset, dial, move(rotate(cylinder(3, 0, 3.5, 10), 'x', 70), 0, -1, 15)]


MODELS = {
    '10mm-pistol': pistol_10mm, 'laser-rifle': laser_rifle, 'combat-shotgun': combat_shotgun,
    'hunting-rifle': hunting_rifle, 'fat-man': fat_man, 'super-sledge': super_sledge,
    'vault-111-jumpsuit': vault_111_jumpsuit, 'leather-chest': leather_chest, 'combat-chest': combat_chest,
    'road-leathers': road_leathers, 'army-helmet': army_helmet, 't60-helmet': t60_helmet,
    'stimpak': stimpak, 'radaway': radaway, 'rad-x': rad_x, 'nuka-cola': nuka_cola, 'mentats': mentats,
    'purified-water': purified_water, 'bobby-pin': bobby_pin, 'holotape': holotape, 'overdue-book': overdue_book,
    'book-token': book_token, 'subway-token': subway_token, 'bobblehead': bobblehead, 'duct-tape': duct_tape,
    'desk-fan': desk_fan, 'wonderglue': wonderglue, 'alarm-clock': alarm_clock, 'typewriter': typewriter,
    'telephone': telephone,
}
