#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Generate detailed 32x32 FF-style side-profile party sprites and 9 Zone Boss sprites."""
import math
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
SPRITES = ROOT / "assets" / "sprites"
BOSSES = ROOT / "assets" / "bosses"


def outline_sprite(im, outline_col=(16, 14, 24)):
    """Add a 1px dark RPG outline around all non-black pixels."""
    w, h = im.size
    px = im.load()
    out = im.copy()
    opx = out.load()
    for y in range(h):
        for x in range(w):
            if px[x, y] != (0, 0, 0):
                continue
            for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < w and 0 <= ny < h and px[nx, ny] != (0, 0, 0):
                    opx[x, y] = outline_col
                    break
    return out


def make_fighter_side():
    """FFVI-style side-profile Knight facing left (classic RPG battle stance)."""
    im = Image.new("RGB", (32, 32), (0, 0, 0))
    d = ImageDraw.Draw(im)
    STEEL_HI = (204, 218, 244)
    STEEL_MD = (132, 148, 180)
    STEEL_DK = (76, 90, 120)
    BLUE_HI = (68, 112, 220)
    BLUE_MD = (40, 72, 168)
    GOLD = (244, 200, 64)
    CAPE_R = (180, 36, 48)
    CAPE_D = (116, 20, 32)
    SKIN = (248, 208, 168)

    # Billowing red cape behind (right side)
    d.polygon([(17, 11), (26, 14), (28, 24), (22, 25), (17, 19)], fill=CAPE_R)
    d.line([(20, 14), (26, 23)], fill=CAPE_D)

    # Plumed knight helm (facing left)
    d.ellipse([11, 4, 20, 12], fill=STEEL_MD)
    d.arc([11, 4, 20, 12], 180, 340, fill=STEEL_HI)
    d.polygon([(15, 2), (23, 2), (26, 6), (19, 6)], fill=CAPE_R)  # plume
    d.rectangle([10, 8, 15, 11], fill=STEEL_DK)  # visor
    d.point((12, 9), fill=SKIN)

    # Armored torso & pauldron
    d.polygon([(12, 12), (20, 12), (21, 21), (13, 21)], fill=BLUE_MD)
    d.ellipse([14, 11, 21, 16], fill=STEEL_HI)  # shoulder pauldron
    d.line([(13, 18), (20, 18)], fill=GOLD)  # belt

    # Legs in stride
    d.polygon([(13, 21), (16, 21), (14, 29), (11, 29)], fill=STEEL_DK)
    d.polygon([(17, 21), (20, 21), (22, 29), (19, 29)], fill=STEEL_MD)
    d.rectangle([10, 28, 14, 30], fill=STEEL_HI)
    d.rectangle([18, 28, 23, 30], fill=STEEL_HI)

    # Broadsword held forward (left)
    d.line([(10, 15), (3, 6)], fill=STEEL_HI, width=2)
    d.line([(9, 15), (3, 7)], fill=(255, 255, 255))
    d.line([(8, 17), (12, 14)], fill=GOLD, width=2)  # crossguard

    # Kite shield on front arm
    d.polygon([(14, 14), (19, 14), (19, 21), (16, 24), (14, 21)], fill=BLUE_HI)
    d.line([(16, 15), (16, 22)], fill=GOLD)
    d.line([(15, 17), (18, 17)], fill=GOLD)

    return outline_sprite(im)


def make_wmage_side():
    """FFVI-style side-profile White Mage facing left."""
    im = Image.new("RGB", (32, 32), (0, 0, 0))
    d = ImageDraw.Draw(im)
    W_HI = (248, 248, 252)
    W_MD = (204, 210, 226)
    W_DK = (156, 164, 184)
    RED = (212, 44, 52)
    SKIN = (252, 216, 176)
    WOOD = (148, 92, 40)
    GOLD = (244, 200, 64)

    # Hooded head (facing left)
    d.ellipse([11, 4, 21, 13], fill=W_HI)
    d.polygon([(18, 5), (23, 9), (20, 13)], fill=W_MD)  # hood tail
    d.rectangle([11, 7, 15, 12], fill=SKIN)
    d.point((12, 9), fill=(24, 24, 40))  # eye
    d.line([(11, 6), (17, 6)], fill=RED)  # crimson trim

    # Flowing white robe
    d.polygon([(12, 13), (20, 13), (24, 28), (10, 28)], fill=W_HI)
    d.polygon([(17, 14), (20, 14), (24, 28), (19, 28)], fill=W_MD)
    d.line([(11, 27), (23, 27)], fill=RED, width=2)  # crimson hem
    d.polygon([(12, 14), (16, 14), (14, 19)], fill=RED)  # collar trim

    # Boots
    d.rectangle([12, 28, 15, 30], fill=WOOD)
    d.rectangle([17, 28, 20, 30], fill=WOOD)

    # Crooked healing staff held forward (left)
    d.line([(8, 8), (8, 29)], fill=WOOD, width=2)
    d.arc([4, 4, 10, 10], 180, 20, fill=WOOD)
    d.ellipse([5, 5, 8, 8], fill=GOLD)

    return outline_sprite(im)


def make_sorceress_side():
    """Alluring FF-style side-profile Red Sorceress facing left."""
    im = Image.new("RGB", (32, 32), (0, 0, 0))
    d = ImageDraw.Draw(im)
    R_HI = (236, 60, 76)
    R_MD = (188, 28, 48)
    R_DK = (118, 16, 32)
    GOLD = (248, 204, 72)
    SKIN = (252, 216, 180)
    SKIN_S = (224, 174, 138)
    HAIR = (176, 76, 36)
    HAIR_HI = (216, 112, 56)
    CYAN = (96, 232, 255)
    WOOD = (128, 72, 32)

    # Flowing auburn hair sweeping behind
    d.polygon([(16, 9), (24, 11), (27, 19), (22, 21), (17, 15)], fill=HAIR)
    d.line([(18, 11), (25, 17)], fill=HAIR_HI)

    # Jaunty tilted crimson witch/red-mage hat
    d.polygon([(16, 1), (23, 4), (19, 8), (11, 8)], fill=R_MD)
    d.line([(16, 2), (13, 7)], fill=R_HI)
    d.polygon([(7, 9), (12, 7), (22, 8), (24, 10)], fill=R_DK)  # wide brim
    d.line([(11, 8), (20, 8)], fill=GOLD)

    # Side-profile face & neck
    d.polygon([(12, 9), (16, 9), (16, 13), (12, 13)], fill=SKIN)
    d.point((13, 10), fill=(32, 24, 48))  # eye
    d.point((12, 12), fill=R_HI)  # lips
    d.line([(14, 13), (14, 15)], fill=SKIN)

    # Fitted crimson corset & flowing slit cloak/skirt
    d.polygon([(12, 15), (17, 15), (16, 20), (13, 20)], fill=R_MD)  # waist
    d.line([(13, 19), (16, 19)], fill=GOLD)  # gold belt
    d.polygon([(16, 15), (20, 16), (25, 28), (18, 28)], fill=R_DK)  # flowing cape
    d.polygon([(13, 20), (17, 20), (19, 26), (14, 26)], fill=R_HI)  # front skirt

    # Long legs & heeled crimson boots (slit dress silhouette)
    d.line([(14, 22), (12, 28)], fill=SKIN_S, width=2)
    d.line([(16, 23), (17, 28)], fill=SKIN, width=2)
    d.rectangle([10, 27, 13, 30], fill=R_DK)
    d.rectangle([16, 27, 19, 30], fill=R_MD)
    d.point((10, 29), fill=GOLD)
    d.point((16, 29), fill=GOLD)

    # Extended arm holding glowing cyan arcane staff
    d.line([(13, 16), (9, 17)], fill=SKIN)
    d.line([(8, 9), (8, 29)], fill=WOOD)
    d.ellipse([5, 4, 11, 10], fill=CYAN)
    d.point((7, 6), fill=(255, 255, 255))
    d.line([(6, 10), (10, 10)], fill=GOLD)

    return outline_sprite(im)


def make_boss(zone_id):
    """Generate 9 distinct 32x32 SNES-style Zone Boss monsters."""
    im = Image.new("RGB", (32, 32), (0, 0, 0))
    d = ImageDraw.Draw(im)

    if zone_id == 0:  # CRYPT: Lich Lord
        ROBE_D, ROBE_L = (52, 34, 84), (92, 62, 136)
        BONE, GLOW = (220, 214, 196), (96, 248, 128)
        d.polygon([(16, 6), (8, 27), (24, 27)], fill=ROBE_D)
        d.polygon([(16, 8), (11, 26), (20, 26)], fill=ROBE_L)
        d.ellipse([12, 5, 20, 13], fill=BONE)  # skull
        d.polygon([(12, 5), (10, 1), (14, 4), (16, 1), (18, 4), (22, 1), (20, 5)], fill=(212, 172, 56))
        d.point((14, 9), fill=GLOW); d.point((18, 9), fill=GLOW)
        d.ellipse([4, 14, 9, 19], fill=GLOW)  # floating orb
        d.line([(8, 18), (12, 16)], fill=BONE)
    elif zone_id == 1:  # TAVERN: Ale Barrel Mimic
        WOOD_L, WOOD_D, IRON = (168, 104, 48), (108, 62, 26), (132, 138, 152)
        d.ellipse([5, 7, 27, 27], fill=WOOD_L)
        d.chord([5, 7, 27, 27], 0, 180, fill=WOOD_D)
        d.line([(6, 12), (26, 12)], fill=IRON, width=2)
        d.line([(6, 22), (26, 22)], fill=IRON, width=2)
        d.rectangle([8, 15, 24, 20], fill=(28, 10, 16))  # maw
        for tx in range(9, 24, 3):
            d.point((tx, 15), fill=(248, 244, 228))
            d.point((tx + 1, 20), fill=(248, 244, 228))
        d.ellipse([10, 9, 13, 12], fill=(255, 200, 48))
        d.ellipse([19, 9, 22, 12], fill=(255, 200, 48))
    elif zone_id == 2:  # GLADE: Elder Treant
        BARK_D, BARK_L, LEAF = (68, 44, 28), (108, 72, 44), (56, 148, 72)
        d.ellipse([4, 3, 28, 16], fill=LEAF)
        d.ellipse([7, 5, 25, 14], fill=(88, 188, 96))
        d.polygon([(11, 12), (21, 12), (24, 29), (8, 29)], fill=BARK_D)
        d.rectangle([13, 13, 18, 28], fill=BARK_L)
        d.line([(10, 16), (4, 21)], fill=BARK_L, width=2)
        d.line([(22, 16), (28, 21)], fill=BARK_L, width=2)
        d.point((13, 16), fill=(192, 255, 96)); d.point((19, 16), fill=(192, 255, 96))
    elif zone_id == 3:  # CASTLE: Vampire Lord
        WING, CAPE, SKIN = (48, 24, 48), (176, 24, 44), (228, 220, 236)
        d.polygon([(16, 13), (2, 5), (4, 18), (12, 17)], fill=WING)
        d.polygon([(16, 13), (30, 5), (28, 18), (20, 17)], fill=WING)
        d.polygon([(16, 11), (9, 28), (23, 28)], fill=CAPE)
        d.ellipse([13, 6, 19, 12], fill=SKIN)
        d.polygon([(13, 7), (16, 4), (19, 7)], fill=(28, 24, 36))
        d.point((14, 9), fill=(255, 40, 48)); d.point((18, 9), fill=(255, 40, 48))
    elif zone_id == 4:  # SANDS: Golden Anubis Djinn
        GOLD, DARK, CYAN = (238, 184, 52), (44, 38, 64), (84, 224, 248)
        d.polygon([(12, 9), (11, 2), (14, 7)], fill=DARK)  # jackal ears
        d.polygon([(20, 9), (21, 2), (18, 7)], fill=DARK)
        d.ellipse([12, 7, 20, 14], fill=DARK)
        d.polygon([(11, 14), (21, 14), (24, 22), (8, 22)], fill=GOLD)
        d.polygon([(13, 22), (19, 22), (16, 30)], fill=(196, 142, 68))  # whirlwind tail
        d.point((14, 10), fill=CYAN); d.point((18, 10), fill=CYAN)
    elif zone_id == 5:  # SPIRE: Arcane Beholder
        BODY, EYE, GOLD = (84, 44, 148), (96, 236, 255), (244, 196, 72)
        for ang in (-140, -100, -60, -20, 20, 160):
            rad = math.radians(ang)
            ex = int(16 + 13 * math.cos(rad))
            ey = int(16 + 12 * math.sin(rad))
            d.line([(16, 16), (ex, ey)], fill=BODY, width=2)
            d.ellipse([ex - 2, ey - 2, ex + 2, ey + 2], fill=EYE)
        d.ellipse([8, 9, 24, 25], fill=BODY)
        d.ellipse([12, 13, 20, 20], fill=(248, 248, 255))
        d.ellipse([14, 15, 18, 19], fill=EYE)
        d.point((16, 17), fill=(16, 16, 32))
    elif zone_id == 6:  # TUNDRA: Frost Wyrm
        ICE_L, ICE_D, HORN = (140, 212, 248), (64, 128, 188), (224, 248, 255)
        d.polygon([(14, 12), (2, 6), (5, 19), (12, 18)], fill=ICE_D)
        d.polygon([(18, 12), (30, 6), (27, 19), (20, 18)], fill=ICE_D)
        d.ellipse([11, 12, 21, 27], fill=ICE_L)
        d.ellipse([12, 6, 20, 13], fill=ICE_L)
        d.line([(13, 6), (10, 2)], fill=HORN, width=2)
        d.line([(19, 6), (22, 2)], fill=HORN, width=2)
        d.point((14, 9), fill=(255, 255, 255)); d.point((18, 9), fill=(255, 255, 255))
    elif zone_id == 7:  # CORONATION: Golden Seraph
        GOLD, WING, HALO = (236, 182, 54), (244, 244, 252), (255, 232, 120)
        d.ellipse([11, 2, 21, 7], outline=HALO)
        d.polygon([(13, 12), (2, 6), (4, 18), (12, 17)], fill=WING)
        d.polygon([(19, 12), (30, 6), (28, 18), (20, 17)], fill=WING)
        d.polygon([(12, 12), (20, 12), (22, 27), (10, 27)], fill=GOLD)
        d.ellipse([13, 6, 19, 12], fill=(216, 216, 232))
        d.line([(23, 10), (26, 24)], fill=(255, 250, 210), width=2)
    else:  # 8: BOSS: Chaos Demon
        OBSID, LAVA, HORN = (48, 24, 34), (255, 128, 24), (220, 200, 160)
        d.polygon([(12, 13), (1, 4), (3, 20), (11, 18)], fill=(108, 24, 32))
        d.polygon([(20, 13), (31, 4), (29, 20), (21, 18)], fill=(108, 24, 32))
        d.polygon([(10, 11), (22, 11), (25, 29), (7, 29)], fill=OBSID)
        d.ellipse([12, 5, 20, 13], fill=OBSID)
        d.line([(12, 6), (7, 1)], fill=HORN, width=2)
        d.line([(20, 6), (25, 1)], fill=HORN, width=2)
        d.ellipse([13, 15, 19, 21], fill=LAVA)  # molten core
        d.point((14, 8), fill=(255, 224, 64)); d.point((18, 8), fill=(255, 224, 64))

    return outline_sprite(im)


def main():
    SPRITES.mkdir(parents=True, exist_ok=True)
    BOSSES.mkdir(parents=True, exist_ok=True)
    make_fighter_side().save(SPRITES / "fighter.png")
    make_wmage_side().save(SPRITES / "wmage.png")
    make_sorceress_side().save(SPRITES / "sorceress.png")
    for z in range(9):
        make_boss(z).save(BOSSES / f"boss_{z}.png")
    print("Generated side-profile party sprites and 9 Zone boss sprites!")


if __name__ == "__main__":
    main()
