#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Dual-Path 8-Bit/16-Bit Pixel Art Asset Generator for Fantasia.

Path A: Reads authored 120x80 PNGs from assets/zones/zone_0..8.png and
        16x16 PNGs from assets/sprites/{fighter,wmage,sorceress,cursor}.png.
Path B: Deterministically generates 9 distinct handcrafted 120x80 8-bit/16-bit
        diorama scenes and 16x16 SNES-style party sprites.
"""
import math
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "assets"
OUT_H = ROOT / "firmware" / "src" / "fantasia_assets.h"
PREVIEWS = ROOT / "build" / "previews"

W, H = 120, 80
BAYER = ((0, 2), (3, 1))


def rgb_to_565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def make_zone_palette(zone_id):
    palettes = [
        # 0: THE CRYPT - deep purple-black, stone slate, moss, torch amber
        [(10, 8, 18), (18, 14, 30), (28, 22, 46), (42, 34, 62),
         (34, 32, 42), (52, 48, 62), (74, 70, 86), (102, 98, 114),
         (24, 42, 34), (42, 68, 52), (128, 124, 138), (164, 160, 174),
         (185, 78, 24), (235, 140, 40), (255, 214, 96), (224, 224, 240)],
        # 1: THE TAVERN - warm oak, stone hearth, firelight
        [(22, 12, 8), (36, 20, 12), (54, 30, 18), (76, 42, 24),
         (96, 56, 30), (124, 74, 38), (150, 96, 50), (178, 120, 68),
         (52, 46, 44), (78, 70, 66), (108, 98, 92), (140, 128, 120),
         (205, 84, 20), (242, 154, 42), (255, 224, 118), (252, 242, 220)],
        # 2: THE GLADE - twilight emerald, bark, crystal water, fireflies
        [(6, 16, 18), (12, 28, 28), (18, 44, 38), (26, 62, 48),
         (20, 48, 32), (32, 74, 46), (48, 102, 62), (72, 134, 82),
         (38, 26, 20), (62, 42, 30), (88, 62, 42), (44, 108, 132),
         (84, 168, 188), (176, 232, 92), (238, 255, 156), (220, 248, 240)],
        # 3: THE CASTLE - crimson blood-moon, obsidian spires, stained glass
        [(14, 4, 14), (28, 8, 24), (48, 14, 34), (74, 20, 44),
         (20, 18, 28), (34, 32, 46), (52, 50, 70), (76, 74, 98),
         (104, 102, 128), (168, 28, 48), (218, 56, 72), (248, 120, 128),
         (255, 198, 198), (240, 180, 60), (96, 180, 230), (248, 240, 248)],
        # 4: THE SANDS - indigo starlit night, golden dunes, sandstone
        [(10, 12, 32), (18, 22, 52), (28, 34, 76), (44, 52, 104),
         (86, 58, 40), (118, 82, 54), (154, 110, 68), (192, 142, 86),
         (220, 172, 108), (242, 202, 138), (252, 226, 174), (62, 46, 36),
         (44, 96, 56), (214, 164, 52), (255, 238, 150), (244, 244, 255)],
        # 5: THE SPIRE - cosmic violet, arcane cyan crystal, rune gold
        [(8, 6, 24), (16, 12, 44), (26, 20, 68), (40, 30, 96),
         (22, 20, 38), (38, 36, 62), (58, 56, 90), (84, 82, 122),
         (24, 64, 108), (40, 104, 156), (68, 152, 208), (114, 204, 242),
         (176, 238, 255), (234, 252, 255), (240, 196, 88), (255, 255, 255)],
        # 6: THE TUNDRA - arctic navy, aurora green/cyan, glacial ice
        [(8, 14, 28), (14, 24, 46), (22, 36, 68), (34, 54, 94),
         (32, 104, 96), (54, 154, 128), (96, 204, 162), (164, 244, 196),
         (54, 82, 114), (82, 122, 158), (122, 166, 202), (168, 208, 236),
         (208, 234, 250), (236, 248, 255), (255, 255, 255), (42, 62, 84)],
        # 7: CORONATION - royal azure, marble columns, crimson & gold
        [(16, 24, 58), (26, 38, 88), (40, 58, 124), (58, 84, 162),
         (88, 86, 102), (124, 122, 138), (166, 164, 178), (210, 208, 222),
         (238, 236, 246), (144, 24, 38), (196, 40, 56), (190, 134, 28),
         (236, 182, 54), (255, 226, 112), (124, 168, 236), (255, 252, 242)],
        # 8: BOSS - volcanic crimson, obsidian basalt, molten magma
        [(16, 4, 6), (34, 6, 10), (58, 10, 14), (90, 16, 18),
         (128, 24, 20), (168, 38, 22), (18, 14, 20), (32, 26, 36),
         (50, 42, 54), (72, 62, 76), (98, 86, 100), (220, 72, 16),
         (255, 136, 24), (255, 198, 48), (255, 244, 156), (255, 255, 255)],
    ]
    return palettes[zone_id % 9]


def new_canvas(bg=0):
    return [[bg] * W for _ in range(H)]


def pset(g, x, y, c):
    if 0 <= x < W and 0 <= y < H:
        g[y][x] = c


def rect(g, x0, y0, w, h, c):
    for y in range(max(0, y0), min(H, y0 + h)):
        for x in range(max(0, x0), min(W, x0 + w)):
            g[y][x] = c


def dither_sky(g, y0, y1, c_start, c_end):
    span = max(1, y1 - y0)
    nc = c_end - c_start
    for y in range(y0, y1):
        t = ((y - y0) / float(span)) * nc
        b = int(t)
        f = t - b
        for x in range(W):
            c = c_start + b + (1 if f * 4.0 > BAYER[y & 1][x & 1] else 0)
            g[y][x] = min(c_end, max(c_start, c))


def draw_circle(g, cx, cy, r, c_fill, c_edge=None):
    for y in range(cy - r, cy + r + 1):
        for x in range(cx - r, cx + r + 1):
            d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy)
            if d2 <= r * r:
                if c_edge is not None and d2 > (r - 2) * (r - 2):
                    pset(g, x, y, c_edge)
                else:
                    pset(g, x, y, c_fill)


def draw_torch(g, tx, ty, col_post=6, col_flame=(12, 13, 14)):
    rect(g, tx - 1, ty, 3, 8, col_post)
    rect(g, tx - 2, ty - 1, 5, 2, col_post + 1)
    # Flame
    pset(g, tx, ty - 5, col_flame[2])
    rect(g, tx - 1, ty - 4, 3, 2, col_flame[1])
    pset(g, tx, ty - 3, col_flame[2])
    pset(g, tx - 2, ty - 3, col_flame[0])
    pset(g, tx + 2, ty - 3, col_flame[0])


def draw_stars(g, seed, count, c_dim=11, c_bright=15):
    s = seed
    for i in range(count):
        s = (s * 1103515245 + 12345) & 0x7FFFFFFF
        x = (s >> 8) % W
        s = (s * 1103515245 + 12345) & 0x7FFFFFFF
        y = (s >> 8) % 44
        pset(g, x, y, c_bright if (i % 4 == 0) else c_dim)


def scene_crypt():
    g = new_canvas(0)
    dither_sky(g, 0, 62, 0, 3)
    # Stone brick back wall with mortar
    for y in range(18, 64):
        for x in range(W):
            if y % 6 == 0 or (x + (y // 6) * 6) % 12 == 0:
                g[y][x] = 4
            else:
                g[y][x] = 5 if ((x * 3 + y) % 17 == 0) else 4 + ((x // 12 + y // 6) & 1)
    # 3 Gothic Vault Arches
    for ax in (20, 60, 100):
        for y in range(8, 62):
            for x in range(ax - 13, ax + 14):
                dx = abs(x - ax)
                arch_y = 14 + (dx * dx) // 11
                if y >= arch_y and dx < 10:
                    g[y][x] = 0 if y < 40 else 1
                elif y >= arch_y - 3 and dx <= 12:
                    g[y][x] = 6 if dx < 11 else 7
    # Mossy stone pillars
    for px in (4, 38, 78, 112):
        rect(g, px, 10, 5, 54, 6)
        rect(g, px + 1, 10, 2, 54, 7)
        rect(g, px - 1, 10, 7, 3, 10)
        rect(g, px - 1, 61, 7, 3, 10)
        # Moss patches
        rect(g, px, 48, 3, 6, 8)
        pset(g, px + 1, 50, 9)
    # Center Sarcophagus
    rect(g, 46, 53, 28, 10, 6)
    rect(g, 44, 51, 32, 3, 10)
    rect(g, 48, 55, 24, 6, 5)
    # Skull emblem on sarcophagus
    rect(g, 58, 56, 4, 4, 11)
    pset(g, 59, 57, 4)
    pset(g, 61, 57, 4)
    # Flagstone floor
    for y in range(64, H):
        for x in range(W):
            g[y][x] = 4 if (y % 5 == 0 or (x + y * 2) % 13 == 0) else 5
            if (x + y) % 19 == 0:
                g[y][x] = 8
    # Wall torches + warm glow halo
    for tx in (20, 100):
        draw_circle(g, tx, 38, 7, 2, 1)
        draw_torch(g, tx, 42)
    return g


def scene_tavern():
    g = new_canvas(1)
    # Warm timber-framed plaster & wood paneling
    rect(g, 0, 0, W, 62, 2)
    for y in range(0, 22):
        for x in range(W):
            g[y][x] = 1 if (y % 4 == 0) else 2
    # Vertical oak beams
    for bx in (6, 36, 80, 110):
        rect(g, bx, 0, 5, 64, 4)
        rect(g, bx + 1, 0, 2, 64, 5)
    # Horizontal roof beam
    rect(g, 0, 16, W, 4, 4)
    rect(g, 0, 17, W, 2, 6)
    # Leaded glass window (left)
    rect(g, 14, 24, 16, 18, 0)
    rect(g, 15, 25, 14, 16, 8)
    rect(g, 21, 25, 2, 16, 4)
    rect(g, 15, 32, 14, 2, 4)
    pset(g, 18, 28, 15)
    # Stone Fireplace Hearth (center)
    rect(g, 45, 24, 30, 40, 9)
    rect(g, 43, 22, 34, 3, 10)
    rect(g, 47, 26, 26, 38, 8)
    rect(g, 51, 42, 18, 22, 0)
    # Crackling hearth fire
    for y in range(50, 63):
        for x in range(53, 67):
            dx = abs(x - 60)
            if y > 52 + dx:
                g[y][x] = 14 if y > 57 and dx < 3 else (13 if y > 54 else 12)
    # Hanging iron chandelier
    rect(g, 59, 0, 2, 14, 8)
    rect(g, 48, 12, 24, 2, 8)
    for cx in (49, 60, 71):
        pset(g, cx, 10, 15)
        pset(g, cx, 9, 14)
        pset(g, cx, 8, 13)
    # Oak barrels (right)
    for bx in (88, 100):
        rect(g, bx, 46, 10, 18, 5)
        rect(g, bx + 2, 46, 6, 18, 6)
        rect(g, bx, 50, 10, 2, 9)
        rect(g, bx, 58, 10, 2, 9)
    # Wooden plank floor
    for y in range(64, H):
        for x in range(W):
            g[y][x] = 3 if (y % 4 == 0 or (x + (y // 4) * 11) % 24 == 0) else 5
    return g


def scene_glade():
    g = new_canvas(0)
    dither_sky(g, 0, 56, 0, 3)
    draw_stars(g, 42, 18, 11, 15)
    # Pale woodland moon
    draw_circle(g, 92, 16, 8, 15, 13)
    # Distant misty tree silhouettes
    for x in range(W):
        top = 28 + int(8 * math.sin(x * 0.14)) + ((x * 5) % 7)
        for y in range(top, 60):
            g[y][x] = 4 if y < top + 6 else 5
    # Ancient foreground oak trunks (left & right)
    for tx, tw in ((8, 10), (102, 10)):
        rect(g, tx, 10, tw, 56, 8)
        rect(g, tx + 2, 10, 4, 56, 9)
        rect(g, tx + 4, 10, 2, 56, 10)
    # Leafy canopy clusters
    for cx, cy, r in ((14, 12, 15), (34, 8, 12), (86, 10, 13), (108, 14, 16)):
        draw_circle(g, cx, cy, r, 6, 5)
        draw_circle(g, cx - 2, cy - 2, r - 4, 7, 6)
    # Mossy forest floor
    for y in range(58, H):
        for x in range(W):
            g[y][x] = 5 if (x + y * 3) % 7 == 0 else 6
    # Crystal pond in center
    for y in range(64, 77):
        hw = 28 - abs(y - 70) * 2
        for x in range(60 - hw, 60 + hw):
            g[y][x] = 12 if (x + y) % 5 == 0 else 11
    # Fireflies
    for fx, fy in ((28, 42), (46, 34), (58, 48), (74, 38), (88, 52), (38, 56)):
        pset(g, fx, fy, 14)
        pset(g, fx + 1, fy, 13)
    return g


def scene_castle():
    g = new_canvas(0)
    dither_sky(g, 0, 60, 0, 3)
    # Huge Crimson Blood Moon
    draw_circle(g, 88, 22, 15, 11, 10)
    draw_circle(g, 86, 20, 11, 12, 11)
    # Distant jagged mountain ridge
    for x in range(W):
        my = 42 + abs((x % 24) - 12)
        for y in range(my, 64):
            g[y][x] = 4
    # Gothic Castle Keep & Spires
    rect(g, 24, 34, 72, 32, 5)
    rect(g, 24, 34, 72, 3, 6)
    # Crenellations
    for cx in range(24, 96, 6):
        rect(g, cx, 30, 3, 4, 6)
    # Tall flanking towers + pointed spires
    for tx in (18, 54, 90):
        th = 20 if tx == 54 else 26
        rect(g, tx, th, 14, 66 - th, 6)
        rect(g, tx + 2, th, 4, 66 - th, 7)
        for sy in range(12):
            hw = sy // 2 + 1
            rect(g, tx + 7 - hw, th - 12 + sy, hw * 2, 1, 5 if sy & 1 else 7)
        # Glowing lancet windows
        rect(g, tx + 5, th + 6, 4, 9, 13)
        rect(g, tx + 6, th + 7, 2, 7, 15)
    # Rose stained-glass window in center keep
    draw_circle(g, 61, 48, 5, 14, 13)
    pset(g, 61, 48, 10)
    # Bats in front of moon
    for bx, by in ((78, 16), (94, 12), (42, 18)):
        pset(g, bx, by, 4)
        pset(g, bx - 2, by - 1, 4)
        pset(g, bx + 2, by - 1, 4)
    # Obsidian bridge & courtyard
    for y in range(64, H):
        for x in range(W):
            g[y][x] = 4 if y % 4 == 0 or (x + y) % 11 == 0 else 5
    return g


def scene_sands():
    g = new_canvas(0)
    dither_sky(g, 0, 52, 0, 3)
    draw_stars(g, 99, 24, 10, 15)
    # Crescent Moon
    draw_circle(g, 26, 16, 7, 14, 13)
    draw_circle(g, 29, 14, 6, 0, 0)
    # Stepped Pyramid (background left)
    for tier in range(5):
        w = 34 - tier * 6
        rect(g, 18 + tier * 3, 46 - tier * 4, w, 4, 5 + (tier & 1))
    # Towering Carved Obelisk (right)
    rect(g, 88, 22, 8, 42, 7)
    rect(g, 90, 22, 4, 42, 8)
    for py in range(6):
        rect(g, 88 + (5 - py) // 2, 16 + py, 2 + py, 1, 13 if py < 3 else 14)
    # Sweeping Layered Golden Dunes
    for x in range(W):
        d1 = 48 + int(6 * math.sin(x * 0.06))
        d2 = 58 + int(5 * math.cos(x * 0.08 + 1.0))
        d3 = 68 + int(4 * math.sin(x * 0.05 + 2.0))
        for y in range(d1, H):
            if y < d2:
                g[y][x] = 6 if y < d1 + 2 else 5
            elif y < d3:
                g[y][x] = 8 if y < d2 + 2 else 7
            else:
                g[y][x] = 10 if y < d3 + 2 else 9
    # Palm silhouette (far left)
    rect(g, 10, 40, 2, 22, 11)
    for dx in range(-6, 7):
        pset(g, 11 + dx, 40 + abs(dx) // 2, 12)
        pset(g, 11 + dx, 38 + abs(dx) // 3, 12)
    return g


def scene_spire():
    g = new_canvas(0)
    dither_sky(g, 0, 64, 0, 3)
    draw_stars(g, 77, 28, 12, 15)
    # Floating Arcane Rune Rings
    for angle in range(0, 360, 15):
        rad = math.radians(angle)
        rx = int(60 + 28 * math.cos(rad))
        ry = int(28 + 8 * math.sin(rad))
        pset(g, rx, ry, 14 if angle % 30 == 0 else 10)
    # Central Crystal Spire
    for y in range(10, 64):
        hw = 2 + (y - 10) // 4
        if y > 46:
            hw = 10
        for x in range(60 - hw, 60 + hw + 1):
            if x < 60 - hw // 2:
                g[y][x] = 9
            elif x < 60 + hw // 3:
                g[y][x] = 11
            else:
                g[y][x] = 12
        if y == 14 or y == 28:
            pset(g, 60, y, 13)
    # Flanking Obsidian Pedestals + Floating Crystals
    for px in (22, 98):
        rect(g, px - 5, 52, 10, 14, 5)
        rect(g, px - 3, 52, 6, 14, 6)
        # Floating diamond crystal
        for dy in range(-7, 8):
            hw = max(0, 4 - abs(dy) // 2)
            for dx in range(-hw, hw + 1):
                pset(g, px + dx, 36 + dy, 12 if dx >= 0 else 10)
    # Arcane tiled floor
    for y in range(64, H):
        for x in range(W):
            g[y][x] = 4 if ((x // 8 + y // 4) & 1) else 5
            if y == 64:
                g[y][x] = 10 if (x % 8 == 0) else 7
    return g


def scene_tundra():
    g = new_canvas(0)
    dither_sky(g, 0, 50, 0, 3)
    draw_stars(g, 19, 20, 12, 14)
    # Shimmering Aurora Borealis Ribbons
    for x in range(W):
        ay = 14 + int(6 * math.sin(x * 0.07))
        for dy in range(12):
            c = 4 + min(3, (11 - dy) // 3)
            if (x + dy) % 3 != 0:
                pset(g, x, ay + dy, c)
    # Jagged Snow-Capped Glacial Peaks
    for x in range(W):
        p1 = 30 + abs((x % 32) - 16)
        p2 = 38 + abs(((x + 12) % 24) - 12)
        top = min(p1, p2)
        for y in range(top, 64):
            if y < top + 5:
                g[y][x] = 13 if (x & 1) else 12
            elif y < top + 12:
                g[y][x] = 10
            else:
                g[y][x] = 8 if (x % 16 < 8) else 9
    # Frozen Pine Trees
    for tx in (12, 26, 94, 108):
        rect(g, tx, 54, 2, 10, 15)
        for row in range(10):
            hw = row // 2 + 1
            rect(g, tx - hw + 1, 44 + row, hw * 2, 1, 13 if row < 3 else 15)
    # Snow & Ice Floes
    for y in range(64, H):
        for x in range(W):
            g[y][x] = 13 if y < 68 else (12 if (x + y * 2) % 9 else 11)
    return g


def scene_coronation():
    g = new_canvas(0)
    dither_sky(g, 0, 62, 0, 3)
    # Tall arched cathedral windows with bright daylight
    for wx in (28, 60, 92):
        for y in range(8, 52):
            for x in range(wx - 7, wx + 8):
                dx = abs(x - wx)
                if y >= 10 + (dx * dx) // 5:
                    g[y][x] = 15 if (x + y) & 1 else 14
    # Diagonal light shafts
    for y in range(18, 66):
        for wx in (28, 60, 92):
            sx = wx + (y - 18) // 2
            for dx in range(-4, 5):
                if 0 <= sx + dx < W and g[y][sx + dx] < 4:
                    g[y][sx + dx] = 3
    # Royal Marble Columns
    for cx in (8, 42, 74, 106):
        rect(g, cx, 0, 8, 64, 5)
        rect(g, cx + 2, 0, 3, 64, 7)
        rect(g, cx + 3, 0, 1, 64, 8)
        rect(g, cx - 1, 60, 10, 4, 6)
    # Crimson & Gold Heraldic Banners
    for bx in (18, 98):
        rect(g, bx, 12, 6, 26, 9)
        rect(g, bx + 1, 12, 4, 24, 10)
        rect(g, bx + 2, 18, 2, 8, 12)
    # Golden Crown / Throne Dais (center)
    rect(g, 48, 54, 24, 10, 6)
    rect(g, 52, 48, 16, 6, 11)
    rect(g, 55, 44, 10, 4, 12)
    pset(g, 56, 43, 13)
    pset(g, 60, 42, 13)
    pset(g, 63, 43, 13)
    # Checkered Marble Floor
    for y in range(64, H):
        for x in range(W):
            g[y][x] = 8 if ((x // 8 + y // 4) & 1) else 5
            if 52 <= x <= 68:
                g[y][x] = 10 if y > 65 else 12  # Red carpet
    return g


def scene_boss():
    g = new_canvas(0)
    dither_sky(g, 0, 56, 0, 5)
    # Fiery eclipse sun
    draw_circle(g, 60, 22, 12, 13, 12)
    draw_circle(g, 60, 22, 8, 6, 14)
    # Jagged Obsidian Basalt Crags & Horned Dark Citadel
    for x in range(W):
        h = 34 + ((x * 11 + 5) % 17)
        if 44 <= x <= 76:
            h = 26 + abs(x - 60) // 2
        for y in range(h, 64):
            g[y][x] = 6 if y > h + 6 else (7 if y > h + 2 else 8)
    # Citadel horns
    for hy in range(14):
        rect(g, 44 + hy // 3, 14 + hy, 3, 1, 8)
        rect(g, 73 - hy // 3, 14 + hy, 3, 1, 8)
    # Glowing Lava Cracks & Molten River
    for y in range(64, H):
        for x in range(W):
            wave = 60 + int(16 * math.sin(y * 0.25))
            if abs(x - wave) < 10:
                g[y][x] = 14 if abs(x - wave) < 3 else (13 if abs(x - wave) < 7 else 12)
            else:
                g[y][x] = 6 if (x + y) % 5 else 7
    # Rising Volcanic Embers
    for ex, ey in ((20, 40), (35, 28), (48, 48), (72, 36), (86, 24), (104, 44)):
        pset(g, ex, ey, 14)
        pset(g, ex, ey + 1, 12)
    return g


def gen_procedural_zone(zone_id):
    builders = [
        scene_crypt, scene_tavern, scene_glade,
        scene_castle, scene_sands, scene_spire,
        scene_tundra, scene_coronation, scene_boss,
    ]
    return builders[zone_id % 9](), make_zone_palette(zone_id)


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


def quantize_rgba(path, size):
    im = Image.open(path).convert("RGBA").resize((size, size), Image.NEAREST)
    px = im.load()
    fg = Image.new("RGB", (size, size), (16, 14, 24))
    fg_px = fg.load()
    mask = [[False] * size for _ in range(size)]
    for y in range(size):
        for x in range(size):
            r, g, b, a = px[x, y]
            if a >= 96:
                mask[y][x] = True
                fg_px[x, y] = (r, g, b)
    q_im = fg.quantize(colors=15, method=Image.MEDIANCUT, dither=0)
    raw_pal = q_im.getpalette()[:45]
    pal = [(0, 0, 0)] + [(raw_pal[i * 3], raw_pal[i * 3 + 1], raw_pal[i * 3 + 2]) for i in range(15)]
    q_px = list(q_im.getdata())
    grid = [[0 if not mask[y][x] else (q_px[y * size + x] + 1) for x in range(size)] for y in range(size)]
    return grid, [rgb_to_565(*c) for c in pal]


def load_boss_sprite(zone_id):
    return quantize_rgba(ASSETS / "bosses" / f"boss_{zone_id}.png", 96)


def load_party_sprites():
    grids, pals = {}, {}
    atk_grids, atk_pals = {}, {}
    for name in ("fighter", "wmage", "sorceress"):
        g, p = quantize_rgba(ASSETS / "sprites" / f"{name}.png", 42)
        grids[name] = g
        pals[name] = p

        # Quantize 3 attack frames against a shared 15-color palette per character
        strip = Image.new("RGB", (42 * 3, 42), (16, 14, 24))
        masks = []
        for f in range(3):
            im = Image.open(ASSETS / "sprites" / f"{name}_atk_{f}.png").convert("RGBA").resize((42, 42), Image.NEAREST)
            px = im.load()
            m = [[False] * 42 for _ in range(42)]
            for y in range(42):
                for x in range(42):
                    r, g0, b, a = px[x, y]
                    if a >= 96:
                        m[y][x] = True
                        strip.putpixel((f * 42 + x, y), (r, g0, b))
            masks.append(m)
        q_strip = strip.quantize(colors=15, method=Image.MEDIANCUT, dither=0)
        raw_pal = q_strip.getpalette()[:45]
        atk_pals[name] = [rgb_to_565(0, 0, 0)] + [
            rgb_to_565(raw_pal[i * 3], raw_pal[i * 3 + 1], raw_pal[i * 3 + 2]) for i in range(15)
        ]
        q_px = list(q_strip.getdata())
        atk_grids[name] = []
        for f in range(3):
            m = masks[f]
            fg = [[0 if not m[y][x] else (q_px[y * 126 + f * 42 + x] + 1) for x in range(42)] for y in range(42)]
            atk_grids[name].append(fg)
    return grids, pals, atk_grids, atk_pals


def gen_party_sprites():
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
    return {
        name: [[int(ch, 16) for ch in row] for row in rows]
        for name, rows in [("fighter", fighter), ("wmage", wmage), ("sorceress", sorceress), ("cursor", cursor)]
    }


def pack_nibbles(grid):
    out = []
    for row in grid:
        for i in range(0, len(row), 2):
            hi = row[i] & 0x0F
            lo = row[i + 1] & 0x0F
            out.append((hi << 4) | lo)
    return out


def load_splash():
    from PIL import Image
    src = ROOT / "assets" / "splash.jpg"
    im = Image.open(src).convert("RGB")
    sm = im.resize((240, 240), Image.LANCZOS)
    q = sm.quantize(colors=256, method=Image.MEDIANCUT, dither=1)
    raw_pal = q.getpalette()[: 256 * 3]
    pal565 = []
    for i in range(256):
        r, g, b = raw_pal[i * 3 : i * 3 + 3]
        pal565.append(rgb_to_565(r, g, b))
    px = list(q.getdata())
    PREVIEWS.mkdir(parents=True, exist_ok=True)
    q.convert("RGB").save(PREVIEWS / "splash_preview.png")
    return pal565, px


def main():
    PREVIEWS.mkdir(parents=True, exist_ok=True)
    OUT_H.parent.mkdir(parents=True, exist_ok=True)

    zones_idx = []
    zones_pal = []
    for z in range(9):
        idx, pal = load_or_gen_zone(z)
        zones_idx.append(pack_nibbles(idx))
        zones_pal.append([rgb_to_565(*c) for c in pal])

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
        rgb_to_565(0, 0, 0), rgb_to_565(12, 28, 60), rgb_to_565(180, 232, 255),
        rgb_to_565(128, 198, 240), rgb_to_565(220, 248, 255), rgb_to_565(92, 160, 218),
        rgb_to_565(64, 136, 212), rgb_to_565(40, 96, 176), rgb_to_565(228, 248, 255),
        rgb_to_565(168, 216, 248), rgb_to_565(112, 196, 244), rgb_to_565(72, 148, 212),
        rgb_to_565(200, 244, 255), rgb_to_565(88, 152, 204), rgb_to_565(160, 240, 255),
        rgb_to_565(255, 255, 255),
    ]
    sprites = gen_party_sprites()
    party_32, party_pals, atk_grids, atk_pals = load_party_sprites()
    boss_grids, boss_pals = [], []
    for z in range(9):
        bg_grid, bg_pal = load_boss_sprite(z)
        boss_grids.append(pack_nibbles(bg_grid))
        boss_pals.append(bg_pal)
    splash_pal, splash_data = load_splash()

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
        "    uint16_t party_pals[3][16];",
        "    uint8_t  party_sprites[3][882];",
        "    uint16_t party_atk_pals[3][16];",
        "    uint8_t  party_atk[3][3][882];",
        "    uint8_t  cursor_sprite[128];",
        "    uint16_t boss_pals[9][16];",
        "    uint8_t  boss_sprites[9][4608];",
        "    uint16_t splash_pal[256];",
        "    uint8_t  splash_data[57600];",
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
    lines.append("    .party_pals = {")
    for name in ("fighter", "wmage", "sorceress"):
        vals = ", ".join(f"0x{c:04X}u" for c in party_pals[name])
        lines.append(f"        {{ {vals} }},")
    lines.append("    },")
    lines.append("    .party_sprites = {")
    for name in ("fighter", "wmage", "sorceress"):
        sdata = pack_nibbles(party_32[name])
        chunk = ", ".join(f"0x{b:02X}u" for b in sdata)
        lines.append(f"        {{ {chunk} }},")
    lines.append("    },")
    lines.append("    .party_atk_pals = {")
    for name in ("fighter", "wmage", "sorceress"):
        vals = ", ".join(f"0x{c:04X}u" for c in atk_pals[name])
        lines.append(f"        {{ {vals} }},")
    lines.append("    },")
    lines.append("    .party_atk = {")
    for name in ("fighter", "wmage", "sorceress"):
        lines.append("        {")
        for f in range(3):
            sdata = pack_nibbles(atk_grids[name][f])
            chunk = ", ".join(f"0x{b:02X}u" for b in sdata)
            lines.append(f"            {{ {chunk} }},")
        lines.append("        },")
    lines.append("    },")
    cdata = pack_nibbles(sprites["cursor"])
    lines.append("    .cursor_sprite = { " + ", ".join(f"0x{b:02X}u" for b in cdata) + " },")
    lines.append("    .boss_pals = {")
    for z in range(9):
        vals = ", ".join(f"0x{c:04X}u" for c in boss_pals[z])
        lines.append(f"        {{ {vals} }},")
    lines.append("    },")
    lines.append("    .boss_sprites = {")
    for z in range(9):
        lines.append("        {")
        data = boss_grids[z]
        for off in range(0, len(data), 24):
            chunk = ", ".join(f"0x{b:02X}u" for b in data[off:off + 24])
            lines.append(f"            {chunk},")
        lines.append("        },")
    lines.append("    },")
    lines.append("    .splash_pal = { " + ", ".join(f"0x{c:04X}u" for c in splash_pal) + " },")
    lines.append("    .splash_data = {")
    for off in range(0, len(splash_data), 24):
        chunk = ", ".join(f"0x{b:02X}u" for b in splash_data[off:off + 24])
        lines.append(f"        {chunk},")
    lines.append("    }")
    lines.append("};")
    lines.append("")
    lines.append("#define G_FREEZE_PALETTE (fantasia_assets.freeze_pal)")
    lines.append("")
    lines.append("#endif /* FANTASIA_ASSETS_H */")

    OUT_H.write_text("\n".join(lines) + "\n")
    print(f"Generated {OUT_H} and 9 previews in {PREVIEWS}")


if __name__ == "__main__":
    main()
