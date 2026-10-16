import zlib
import struct
import os

def create_png(width, height, get_pixel_fn, filename):
    raw_data = bytearray()
    for y in range(height):
        raw_data.append(0) # filter type 0 (None)
        for x in range(width):
            r, g, b, a = get_pixel_fn(x, y, width, height)
            r = max(0, min(255, int(r)))
            g = max(0, min(255, int(g)))
            b = max(0, min(255, int(b)))
            a = max(0, min(255, int(a)))
            raw_data.extend([r, g, b, a])
    
    compressed = zlib.compress(raw_data, 9)
    
    def make_chunk(chunk_type, data):
        length = struct.pack('>I', len(data))
        crc = struct.pack('>I', zlib.crc32(chunk_type + data) & 0xffffffff)
        return length + chunk_type + data + crc
    
    png = bytearray(b'\x89PNG\r\n\x1a\n')
    ihdr = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    png.extend(make_chunk(b'IHDR', ihdr))
    png.extend(make_chunk(b'IDAT', compressed))
    png.extend(make_chunk(b'IEND', b''))
    
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with open(filename, 'wb') as f:
        f.write(png)
    print(f"Created {filename} ({width}x{height})")

# Warrior Palette (128x128)
# Grid 2x2 quadrants:
# Quad (0,0) [x:0..63, y:0..63]: Polished Steel Armor (Silver-Blue)
# Quad (1,0) [x:64..127, y:0..63]: Royal Blue Tabard / Cloth
# Quad (0,1) [x:0..63, y:64..127]: Radiant Gold Trims & Visor
# Quad (1,1) [x:64..127, y:64..127]: Steel Blade with Sharp Edge
def warrior_pixel(x, y, w, h):
    u = x / w
    v = y / h
    # Top-Left: Steel Plate with metallic bevel
    if u < 0.5 and v < 0.5:
        base = 210 + int(35 * ((x % 32) / 32.0))
        return (base - 10, base, base + 15, 255)
    # Top-Right: Royal Blue Cloth with gold weave
    elif u >= 0.5 and v < 0.5:
        if (x % 16 == 0) or (y % 16 == 0):
            return (220, 180, 50, 255) # Gold weave lines
        return (28, 65, 145, 255)     # Rich Royal Blue
    # Bottom-Left: Radiant Gold / Visor
    elif u < 0.5 and v >= 0.5:
        # Visor / gold trim
        if (y % 8 < 3):
            return (255, 220, 70, 255) # Bright gold
        return (215, 160, 35, 255)     # Deep gold
    # Bottom-Right: Broadsword Blade (Reflective steel)
    else:
        # Blade fuller in middle
        if abs((x - 96)) < 4:
            return (130, 150, 180, 255) # Fuller groove
        return (245, 248, 255, 255)     # Shiny razor steel

# Boss Gorgon Palette (128x128)
# Dark Iron Armor with Molten Lava Runes
def boss_pixel(x, y, w, h):
    u = x / w
    v = y / h
    # Top-Left: Dark Iron Plate
    if u < 0.5 and v < 0.5:
        base = 38 + int(12 * ((x % 16) / 16.0))
        return (base, base + 2, base + 5, 255)
    # Top-Right: Molten Red / Orange Glowing Runes
    elif u >= 0.5 and v < 0.5:
        glow = int(180 + 75 * abs(((x + y) % 32) - 16) / 16.0)
        return (glow, int(glow * 0.3), 15, 255)
    # Bottom-Left: Warhammer Heavy Bronze / Steel
    elif u < 0.5 and v >= 0.5:
        return (75, 70, 68, 255)
    # Bottom-Right: Dark Spikes & Horns
    else:
        return (22, 22, 25, 255)

# Arena Floor Texture (256x256)
# Paved Colosseum flagstones with circular stone borders
def arena_pixel(x, y, w, h):
    cx = x - 128
    cy = y - 128
    dist = (cx*cx + cy*cy)**0.5
    # Circular arena ring pattern
    ring = int(dist) % 32
    if ring < 2 or ring == 31:
        return (145, 155, 170, 255) # Bright stone ring border
    if abs(dist - 64) < 4:
        return (195, 170, 90, 255)  # Gold combat circle
    # Stone tile mortar lines
    tx = (x % 32)
    ty = (y % 32)
    if tx == 0 or ty == 0:
        return (50, 55, 62, 255)     # Mortar groove
    # Subtle stone texture noise
    noise = int(((x * 17 + y * 31) % 23) - 11)
    base = 110 + noise
    return (base, base + 5, base + 10, 255)

create_png(128, 128, warrior_pixel, "assets/textures/warrior_palette.png")
create_png(128, 128, boss_pixel, "assets/textures/boss_palette.png")
create_png(256, 256, arena_pixel, "assets/textures/arena_floor.png")
