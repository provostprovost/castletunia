#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Dual-Path 16-bit SNES Asset Generator for Fantasia.

Path A: Reads authored 120x80 PNGs from assets/zones/zone_0..8.png and
        16x16 PNGs from assets/sprites/{fighter,wmage,sorceress,cursor}.png.
Path B: Deterministically generates 3-layer dithered procedural silhouettes
        and 16x16 SNES-style party sprites when authored PNGs are absent.
"""
import math
from pathlib import Path
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
OUT_H = ROOT / "firmware" / "src" / "fantasia_assets.h"
PREVIEWS = ROOT / "build" / "previews"


def rgb_to_565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def make_zone_palette(zone_id):
    # 16-color RGB888 palettes per zone (sky/bg, mid, fg, accents, particle)
    palettes = [
        # 0: THE CRYPT - deep purple-black, stone slate, torch amber
        [(10, 8, 18), (18, 14, 30), (28, 22, 46), (42, 34, 62),
         (56, 48, 76), (74, 66, 94), (96, 88, 114), (122, 114, 138),
         (30, 26, 36), (48, 42, 54), (68, 60, 74), (90, 82, 96),
         (180, 90, 30), (235, 145, 45), (255, 210, 90), (220, 220, 240)],
        # 1: THE TAVERN - warm hearth, oak beams, candlelight
        [(24, 14, 10), (38, 22, 14), (56, 32, 18), (78, 44, 24),
         (98, 58, 30), (122, 74, 38), (146, 92, 48), (172, 114, 62),
         (64, 48, 38), (90, 70, 56), (118, 94, 76), (150, 124, 100),
         (210, 110, 30), (245, 165, 50), (255, 225, 120), (250, 240, 220)],
        # 2: THE GLADE - twilight emerald, moss, fireflies
        [(8, 20, 18), (14, 34, 28), (22, 52, 40), (32, 72, 52),
         (44, 94, 66), (58, 118, 80), (78, 142, 96), (104, 168, 118),
         (26, 42, 30), (42, 62, 44), (60, 86, 60), (84, 112, 80),
         (160, 210, 80), (210, 245, 120), (245, 255, 180), (220, 245, 235)],
        # 3: THE CASTLE - crimson moon, obsidian towers
        [(16, 6, 14), (32, 10, 24), (54, 16, 36), (78, 22, 46),
         (108, 30, 58), (140, 42, 70), (176, 60, 86), (214, 92, 110),
         (24, 22, 34), (40, 38, 54), (62, 60, 80), (88, 86, 110),
         (180, 40, 60), (230, 80, 90), (255, 190, 190), (245, 235, 245)],
        # 4: THE SANDS - indigo night, golden dunes, obelisk
        [(16, 14, 34), (26, 22, 54), (40, 34, 78), (58, 48, 104),
         (92, 64, 48), (124, 88, 60), (158, 114, 74), (192, 142, 90),
         (216, 168, 108), (236, 194, 132), (248, 218, 164), (110, 96, 120),
         (200, 140, 40), (240, 185, 60), (255, 235, 140), (240, 240, 255)],
        # 5: THE SPIRE - arcane violet, cyan crystal
        [(12, 8, 30), (22, 16, 52), (34, 26, 78), (50, 38, 108),
         (68, 54, 140), (90, 74, 174), (116, 102, 204), (148, 136, 232),
         (28, 44, 76), (44, 74, 114), (64, 112, 158), (92, 158, 206),
         (120, 208, 240), (168, 234, 255), (220, 248, 255), (255, 255, 255)],
        # 6: THE TUNDRA - arctic navy, glacial ice, aurora
        [(10, 18, 34), (18, 30, 54), (28, 46, 78), (42, 66, 104),
         (58, 90, 134), (78, 118, 164), (104, 148, 194), (138, 180, 222),
         (60, 130, 120), (96, 178, 156), (156, 204, 232), (190, 224, 245),
         (218, 240, 255), (238, 250, 255), (255, 255, 255), (180, 255, 220)],
        # 7: CORONATION - royal azure, marble & gold banners
        [(18, 26, 64), (28, 42, 94), (42, 62, 128), (60, 86, 164),
         (84, 114, 196), (114, 144, 222), (148, 176, 240), (188, 208, 252),
         (104, 98, 114), (138, 132, 148), (176, 170, 184), (216, 212, 224),
         (196, 142, 32), (235, 184, 58), (255, 226, 110), (255, 250, 240)],
        # 8: BOSS - volcanic crimson, obsidian, magma
        [(20, 4, 6), (38, 8, 10), (62, 12, 14), (92, 18, 18),
         (126, 24, 22), (162, 34, 26), (200, 52, 30), (238, 84, 38),
         (22, 18, 22), (38, 32, 38), (58, 50, 58), (82, 72, 82),
         (255, 128, 24), (255, 188, 48), (255, 240, 140), (255, 255, 255)],
    ]
    return palettes[zone_id % 9]


def gen_procedural_zone(zone_id):
    pal = make_zone_palette(zone_id)
    w, h = 120, 80
    idx = [[0] * w for _ in range(h)]

    # Layer 1: Ordered 2x2 Bayer dithered sky gradient (indices 0..7)
    bayer = ((0, 2), (3, 1))
    for y in range(h):
        t = (y / float(h - 1)) * 7.0
        base = int(t)
        frac = t - base
        for x in range(w):
            c = base + (1 if frac * 4.0 > bayer[y & 1][x & 1] else 0)
            if c > 7:
                c = 7
            idx[y][x] = c

    # Celestial moon / orb / rose window
    cx, cy, rad = 88 if zone_id % 2 == 0 else 32, 22, 11
    for y in range(max(0, cy - rad), min(h, cy + rad + 1)):
        for x in range(max(0, cx - rad), min(w, cx + rad + 1)):
            d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy)
            if d2 <= rad * rad:
                idx[y][x] = 14 if d2 < (rad - 3) * (rad - 3) else 13

    # Layer 2: Mid-ground silhouettes (indices 8..11)
    for x in range(w):
        if zone_id in (0, 3, 5, 7):  # Arches / Spires / Castle battlements
            tower = 38 + int(14 * math.sin(x * 0.11 + zone_id))
            if (x // 8) % 2 == 0 and zone_id in (3, 7):
                tower -= 6
            if zone_id == 5:
                tower = 28 + abs((x % 24) - 12) * 2
        elif zone_id in (2, 6):  # Pine trees / Peaks
            tower = 44 + abs((x % 16) - 8) * 2 + int(6 * math.sin(x * 0.08))
        elif zone_id == 4:  # Dunes
            tower = 48 + int(8 * math.sin(x * 0.07))
        elif zone_id == 1:  # Tavern roof & beams
            tower = 36 if (x % 20 < 4) else 46
        else:  # Boss jagged crags
            tower = 36 + ((x * 7 + 3) % 19)

        for y in range(max(0, tower), h):
            shade = 8 + min(3, (y - tower) // 8)
            idx[y][x] = shade

    # Layer 3: Foreground framing & floor (indices 8..13)
    for y in range(64, h):
        for x in range(w):
            if (x + y * 2) % 9 == 0:
                idx[y][x] = 10
            else:
                idx[y][x] = 8 if y > 72 else 9

    # Torch / Lantern / Crystal accents
    for px in (18, 102):
        for dy in range(-5, 1):
            idx[58 + dy][px] = 11
        idx[52][px] = 14
        idx[51][px] = 13
        idx[52][px - 1] = 12
        idx[52][px + 1] = 12

    return idx, pal


def load_or_gen_zone(zone_id):
    p = ASSETS / "zones" / f"zone_{zone_id}.png"
    if p.exists():
        im = Image.open(p).convert("RGB").resize((120, 80), Image.NEAREST)
        q = im.quantize(colors=16, method=Image.MEDIANCUT)
        raw_pal = q.getpalette()[:48]
        pal = [(raw_pal[i * 3], raw_pal[i * 3 + 1], raw_pal[i * 3 + 2]) for i in range(16)]
        px = list(q.getdata())
        idx = [px[r * 120:(r + 1) * 120] for r in range(80)]
        return idx, pal
    return gen_procedural_zone(zone_id)


def make_sprite_palette():
    # 0 = transparent, 1 = black outline, 2..15 = SNES RPG colors
    return [
        (0, 0, 0),       # 0: transparent
        (16, 16, 24),    # 1: outline
        (252, 216, 176), # 2: skin
        (220, 172, 132), # 3: skin shade
        (180, 196, 224), # 4: steel light
        (108, 124, 156), # 5: steel dark
        (48, 84, 184),   # 6: royal blue
        (28, 52, 128),   # 7: dark blue
        (244, 244, 248), # 8: white robe
        (188, 196, 214), # 9: robe shade
        (216, 48, 56),   # 10: crimson
        (148, 24, 34),   # 11: dark crimson
        (244, 200, 64),  # 12: gold
        (164, 108, 44),  # 13: leather/wood
        (96, 220, 248),  # 14: arcane cyan
        (255, 255, 255), # 15: pure white
    ]


def gen_party_sprites():
    # 16x16 pixel art patterns using hex digits 0..F
    fighter = [
        "0000011111100000",
        "0000144444410000",
        "000144C44C441000",
        "0001551111551000",
        "0001512222151000",
        "0000112332110000",
        "0011466666641100",
        "0144166CC6614410",
        "0145176666715410",
        "01511D6666D11510",
        "0010144CC4410100",
        "0000155555510000",
        "0000166116610000",
        "0000177117710000",
        "0001DD1001DD1000",
        "0001110000111000",
    ]
    wmage = [
        "0000011881100000",
        "0000188888810000",
        "000188AAAA881000",
        "0001812222181000",
        "0001812112181000",
        "0000112332110000",
        "0001888888881000",
        "00188A8888A88100",
        "0018998AA8998100",
        "0D18988888898100",
        "0D10188AA8810100",
        "0D00198888910000",
        "0D00189889810000",
        "0D00199999910000",
        "0D001DD11DD10000",
        "0000111001110000",
    ]
    sorceress = [
        "0000001AA1000000",
        "000001AAAA100000",
        "00001AABBAA10000",
        "0011AAAAAAAA1100",
        "0001112222111000",
        "0000112332110000",
        "0001AAACCAAA1000",
        "001AABAAAABAA100",
        "01EA1BAAAAB11000",
        "01E11AAACAA10000",
        "00D01BAAAAB10000",
        "00D01ABBBBA10000",
        "00D01BBBBBB10000",
        "00D0111111110000",
        "00D001D11D100000",
        "0000011001100000",
    ]
    cursor = [
        "0000000000000000",
        "0000001111000000",
        "000001FFFF100000",
        "00001FFFFFF11000",
        "0001FFFFFFFFF100",
        "001FFFFFFFFFFF10",
        "001FFFFFFFF99910",
        "001FFFFFFFFFFF10",
        "001FFFF999111100",
        "0001FFFFF1000000",
        "00001FFFF1000000",
        "0000011110000000",
        "0000000000000000",
        "0000000000000000",
        "0000000000000000",
        "0000000000000000",
    ]
    sprites = {}
    for name, rows in [("fighter", fighter), ("wmage", wmage), ("sorceress", sorceress), ("cursor", cursor)]:
        sprites[name] = [[int(ch, 16) for ch in row] for row in rows]
    return sprites


def pack_nibbles(grid):
    out = []
    for row in grid:
        for i in range(0, len(row), 2):
            hi = row[i] & 0x0F
            lo = row[i + 1] & 0x0F
            out.append((hi << 4) | lo)
    return out


def main():
    PREVIEWS.mkdir(parents=True, exist_ok=True)
    OUT_H.parent.mkdir(parents=True, exist_ok=True)

    zones_idx = []
    zones_pal = []
    for z in range(9):
        idx, pal = load_or_gen_zone(z)
        zones_idx.append(pack_nibbles(idx))
        zones_pal.append([rgb_to_565(*c) for c in pal])

        # Save preview PNG
        im = Image.new("RGB", (240, 160))
        px = im.load()
        for y in range(80):
            for x in range(120):
                c = pal[idx[y][x]]
                px[x * 2, y * 2] = c
                px[x * 2 + 1, y * 2] = c
                px[x * 2, y * 2 + 1] = c
                px[x * 2 + 1, y * 2 + 1] = c
        im.save(PREVIEWS / f"zone_{z}.png")

    sp_pal = make_sprite_palette()
    sp_pal_565 = [rgb_to_565(*c) for c in sp_pal]
    freeze_pal_565 = [
        rgb_to_565(0, 0, 0),
        rgb_to_565(12, 28, 60),
        rgb_to_565(180, 232, 255),
        rgb_to_565(128, 198, 240),
        rgb_to_565(220, 248, 255),
        rgb_to_565(92, 160, 218),
        rgb_to_565(64, 136, 212),
        rgb_to_565(40, 96, 176),
        rgb_to_565(228, 248, 255),
        rgb_to_565(168, 216, 248),
        rgb_to_565(112, 196, 244),
        rgb_to_565(72, 148, 212),
        rgb_to_565(200, 244, 255),
        rgb_to_565(88, 152, 204),
        rgb_to_565(160, 240, 255),
        rgb_to_565(255, 255, 255),
    ]
    sprites = gen_party_sprites()

    lines = [
        "/* SPDX-License-Identifier: GPL-3.0-only */",
        "/* Auto-generated by tools/gen_assets.py — DO NOT EDIT */",
        "#ifndef FANTASIA_ASSETS_H",
        "#define FANTASIA_ASSETS_H",
        "",
        "#include <stdint.h>",
        "",
        "typedef struct {",
        "    uint16_t zone_pals[9][16];",
        "    uint8_t  zone_maps[9][4800];",
        "    uint16_t sprite_pal[16];",
        "    uint16_t freeze_pal[16];",
        "    uint8_t  sprites[4][128];",
        "} FantasiaAssets;",
        "",
        "static const FantasiaAssets fantasia_assets = {",
        "    .zone_pals = {",
    ]
    for z in range(9):
        vals = ", ".join(f"0x{c:04X}u" for c in zones_pal[z])
        lines.append(f"        {{ {vals} }},")
    lines.append("    },")
    lines.append("    .zone_maps = {")
    for z in range(9):
        lines.append("        {")
        data = zones_idx[z]
        for off in range(0, len(data), 24):
            chunk = ", ".join(f"0x{b:02X}u" for b in data[off:off + 24])
            lines.append(f"            {chunk},")
        lines.append("        },")
    lines.append("    },")
    lines.append("    .sprite_pal = { " + ", ".join(f"0x{c:04X}u" for c in sp_pal_565) + " },")
    lines.append("    .freeze_pal = { " + ", ".join(f"0x{c:04X}u" for c in freeze_pal_565) + " },")
    lines.append("    .sprites = {")
    for name in ("fighter", "wmage", "sorceress", "cursor"):
        sdata = pack_nibbles(sprites[name])
        chunk = ", ".join(f"0x{b:02X}u" for b in sdata)
        lines.append(f"        {{ {chunk} }},")
    lines.append("    }")
    lines.append("};")
    lines.append("")
    lines.append("#define G_FREEZE_PALETTE (fantasia_assets.freeze_pal)")
    lines.append("")
    lines.append("#endif /* FANTASIA_ASSETS_H */")

    OUT_H.write_text("\n".join(lines) + "\n")
    print(f"Generated {OUT_H} ({OUT_H.stat().st_size} bytes text) and 9 previews in {PREVIEWS}")


if __name__ == "__main__":
    main()
