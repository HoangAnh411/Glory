import os
import json
import struct
import math
from PIL import Image, ImageDraw

os.makedirs("assets/textures", exist_ok=True)
os.makedirs("assets/models", exist_ok=True)

# ==============================================================================
# 1. GENERATE PALETTES & TEXTURES
# ==============================================================================

# Palette 256x256 with distinct regions:
# (0..63,   0..63)   : Stone brick & cobblestone (gray-beige)
# (64..127, 0..63)   : Dark timber & wood planks (warm dark brown)
# (128..191,0..63)   : Terracotta roof tiles (burnt orange-red)
# (192..255,0..63)   : Glowing lantern glass (warm glowing amber/yellow)
# (0..63,   64..127) : Oak tree leaf canopy (lush emerald green)
# (64..127, 64..127) : Pine needle canopy (dark evergreen)
# (128..191,64..127) : Tree bark (deep rugged brown)
# (192..255,64..127) : Natural granite rock (mottled gray-slate)
# (0..63,   128..191): Wrought iron & metal trim (dark metallic charcoal)
# (64..127, 128..191): Raging brazier fire & molten embers (bright fiery red-orange)
# (128..191,128..191): Ancient ruin stone with moss (weathered greenish-gray)
# (192..255,128..191): Glistening fountain water (azure cyan)
# (0..63,   192..255): Dark scorched dungeon stone (obsidian ash)
# (64..127, 192..255): Golden brass trim & ornaments (radiant gold)

palette = Image.new("RGBA", (256, 256))
draw = ImageDraw.Draw(palette)

def fill_gradient(x0, y0, x1, y1, c1, c2, noise=5):
    import random
    random.seed(42 + x0 * 7 + y0 * 13)
    for y in range(y0, y1):
        for x in range(x0, x1):
            t = (y - y0) / max(1, (y1 - y0))
            r = int(c1[0] * (1 - t) + c2[0] * t + random.randint(-noise, noise))
            g = int(c1[1] * (1 - t) + c2[1] * t + random.randint(-noise, noise))
            b = int(c1[2] * (1 - t) + c2[2] * t + random.randint(-noise, noise))
            palette.putpixel((x, y), (max(0, min(255, r)), max(0, min(255, g)), max(0, min(255, b)), 255))

# Fill regions
fill_gradient(0, 0, 64, 64, (160, 155, 145), (115, 110, 105), 8)         # 0: Stone brick
fill_gradient(64, 0, 128, 64, (110, 72, 45), (70, 42, 22), 6)            # 1: Dark timber
fill_gradient(128, 0, 192, 64, (185, 75, 45), (140, 48, 28), 8)          # 2: Terracotta roof
fill_gradient(192, 0, 256, 64, (255, 240, 140), (255, 175, 30), 4)       # 3: Glowing lantern glass
fill_gradient(0, 64, 64, 128, (65, 155, 55), (35, 105, 30), 8)           # 4: Oak leaves
fill_gradient(64, 64, 128, 128, (35, 95, 75), (18, 55, 42), 6)           # 5: Pine needles
fill_gradient(128, 64, 192, 128, (75, 50, 32), (45, 28, 16), 5)          # 6: Tree bark
fill_gradient(192, 64, 256, 128, (135, 138, 145), (85, 90, 95), 8)       # 7: Granite rock
fill_gradient(0, 128, 64, 192, (55, 58, 65), (25, 26, 30), 4)            # 8: Wrought iron
fill_gradient(64, 128, 128, 192, (255, 120, 20), (240, 40, 10), 8)       # 9: Brazier fire
fill_gradient(128, 128, 192, 192, (120, 130, 115), (75, 88, 72), 7)      # 10: Ancient mossy stone
fill_gradient(192, 128, 256, 192, (75, 180, 235), (35, 120, 195), 6)     # 11: Fountain water
fill_gradient(0, 192, 64, 256, (40, 38, 45), (20, 18, 22), 5)            # 12: Scorched dungeon stone
fill_gradient(64, 192, 128, 256, (235, 195, 55), (180, 135, 25), 6)      # 13: Gold brass

# Add stone brick lines to region 0
for y in range(0, 64, 12):
    draw.line([(0, y), (63, y)], fill=(80, 75, 70, 255), width=1)
for x in range(0, 64, 16):
    draw.line([(x, 0), (x, 63)], fill=(80, 75, 70, 255), width=1)

palette.save("assets/textures/environment_palette.png")
print("Saved assets/textures/environment_palette.png")

# UV Helper for 64x64 blocks in 256x256 palette
# block (col, row): col in [0..3], row in [0..3]
def uv_block(col, row, u_min=0.1, v_min=0.1, u_max=0.9, v_max=0.9):
    u0 = (col * 64 + u_min * 64) / 256.0
    u1 = (col * 64 + u_max * 64) / 256.0
    v0 = (row * 64 + v_min * 64) / 256.0
    v1 = (row * 64 + v_max * 64) / 256.0
    return [(u0, v0), (u1, v0), (u1, v1), (u0, v1)]

UV_STONE_BRICK = uv_block(0, 0)
UV_TIMBER      = uv_block(1, 0)
UV_ROOF        = uv_block(2, 0)
UV_LANTERN     = uv_block(3, 0)
UV_OAK_LEAF    = uv_block(0, 1)
UV_PINE_LEAF   = uv_block(1, 1)
UV_TREE_BARK   = uv_block(2, 1)
UV_ROCK        = uv_block(3, 1)
UV_IRON        = uv_block(0, 2)
UV_FIRE        = uv_block(1, 2)
UV_MOSS_STONE  = uv_block(2, 2)
UV_WATER       = uv_block(3, 2)
UV_SCORCHED    = uv_block(0, 3)
UV_GOLD        = uv_block(1, 3)

# Multi-Zone Ground Texture (512x512)
ground_img = Image.new("RGBA", (512, 512))
g_draw = ImageDraw.Draw(ground_img)

# Town section (Top: y=0..180) -> Cobblestone pavers
# Trail section (y=180..320) -> Green meadows with center gravel road
# Danger section (y=320..512) -> Dark cracked obsidian volcanic stone
import random
random.seed(999)
for y in range(512):
    for x in range(512):
        n = random.randint(-5, 5)
        if y < 190:
            # Cobblestone city
            grid = (x % 24 < 2) or (y % 20 < 2)
            c = (145 + n, 140 + n, 130 + n) if not grid else (85, 80, 75)
        elif y < 340:
            # Meadow & dirt road
            dist_road = abs(x - 256)
            if dist_road < 38 + math.sin(y * 0.05) * 12:
                # Road
                c = (140 + n, 120 + n, 95 + n)
            else:
                # Lush green grass
                c = (65 + n, 130 + n, 48 + n)
        else:
            # Danger dark scorched stone
            cracks = (x * 7 + y * 13) % 47 < 2
            c = (35 + n, 32 + n, 38 + n) if not cracks else (175, 45, 20)
        ground_img.putpixel((x, y), (max(0, min(255, c[0])), max(0, min(255, c[1])), max(0, min(255, c[2])), 255))

ground_img.save("assets/textures/world_ground.png")
print("Saved assets/textures/world_ground.png")

# ==============================================================================
# 2. GLTF MODEL BUILDER HELPER
# ==============================================================================

class MeshBuilder:
    def __init__(self):
        self.positions = []
        self.normals = []
        self.texcoords = []
        self.indices = []

    def add_quad(self, p0, p1, p2, p3, uv_coords):
        # Calculate normal
        v01 = (p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2])
        v02 = (p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2])
        nx = v01[1] * v02[2] - v01[2] * v02[1]
        ny = v01[2] * v02[0] - v01[0] * v02[2]
        nz = v01[0] * v02[1] - v01[1] * v02[0]
        length = math.sqrt(nx * nx + ny * ny + nz * nz)
        if length > 1e-6:
            norm = (nx / length, ny / length, nz / length)
        else:
            norm = (0.0, 1.0, 0.0)

        base_idx = len(self.positions)
        self.positions.extend([p0, p1, p2, p3])
        self.normals.extend([norm, norm, norm, norm])
        self.texcoords.extend(uv_coords)
        self.indices.extend([base_idx, base_idx + 1, base_idx + 2, base_idx, base_idx + 2, base_idx + 3])

    def add_box(self, min_pt, max_pt, uv_coords):
        x0, y0, z0 = min_pt
        x1, y1, z1 = max_pt
        # Front (+Z)
        self.add_quad((x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1), uv_coords)
        # Back (-Z)
        self.add_quad((x1, y0, z0), (x0, y0, z0), (x0, y1, z0), (x1, y1, z0), uv_coords)
        # Top (+Y)
        self.add_quad((x0, y1, z1), (x1, y1, z1), (x1, y1, z0), (x0, y1, z0), uv_coords)
        # Bottom (-Y)
        self.add_quad((x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1), uv_coords)
        # Left (-X)
        self.add_quad((x0, y0, z0), (x0, y0, z1), (x0, y1, z1), (x0, y1, z0), uv_coords)
        # Right (+X)
        self.add_quad((x1, y0, z1), (x1, y0, z0), (x1, y1, z0), (x1, y1, z1), uv_coords)

    def add_pyramid(self, base_min, base_max, tip_y, uv_coords):
        x0, y0, z0 = base_min
        x1, _, z1 = base_max
        cx = (x0 + x1) * 0.5
        cz = (z0 + z1) * 0.5
        tip = (cx, tip_y, cz)

        # 4 triangular faces
        for (p0, p1) in [((x0, y0, z1), (x1, y0, z1)),
                         ((x1, y0, z1), (x1, y0, z0)),
                         ((x1, y0, z0), (x0, y0, z0)),
                         ((x0, y0, z0), (x0, y0, z1))]:
            self.add_quad(p0, p1, tip, tip, uv_coords)

    def save_gltf(self, model_name, texture_uri="../textures/environment_palette.png"):
        bin_data = bytearray()
        def write_floats(data):
            start = len(bin_data)
            for val in data: bin_data.extend(struct.pack('<f', val))
            return start, len(bin_data) - start

        def write_uint16(data):
            start = len(bin_data)
            for val in data: bin_data.extend(struct.pack('<H', val))
            return start, len(bin_data) - start

        pos_off, pos_len = write_floats([c for pt in self.positions for c in pt])
        norm_off, norm_len = write_floats([c for n in self.normals for c in n])
        tex_off, tex_len = write_floats([c for uv in self.texcoords for c in uv])
        idx_off, idx_len = write_uint16(self.indices)

        bin_file = f"assets/models/{model_name}.bin"
        with open(bin_file, "wb") as f:
            f.write(bin_data)

        min_x = min(p[0] for p in self.positions) if self.positions else 0.0
        max_x = max(p[0] for p in self.positions) if self.positions else 0.0
        min_y = min(p[1] for p in self.positions) if self.positions else 0.0
        max_y = max(p[1] for p in self.positions) if self.positions else 0.0
        min_z = min(p[2] for p in self.positions) if self.positions else 0.0
        max_z = max(p[2] for p in self.positions) if self.positions else 0.0

        gltf = {
            "asset": {"version": "2.0", "generator": "Glory Environment Generator"},
            "scene": 0,
            "scenes": [{"nodes": [0]}],
            "nodes": [{"name": model_name, "mesh": 0}],
            "meshes": [{
                "name": model_name + "_Mesh",
                "primitives": [{
                    "attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2},
                    "indices": 3,
                    "material": 0
                }]
            }],
            "materials": [{
                "name": "Mat_" + model_name,
                "pbrMetallicRoughness": {
                    "baseColorTexture": {"index": 0},
                    "metallicFactor": 0.1,
                    "roughnessFactor": 0.8
                }
            }],
            "textures": [{"source": 0}],
            "images": [{"uri": texture_uri}],
            "buffers": [{"uri": f"{model_name}.bin", "byteLength": len(bin_data)}],
            "bufferViews": [
                {"buffer": 0, "byteOffset": pos_off, "byteLength": pos_len, "target": 34962},
                {"buffer": 0, "byteOffset": norm_off, "byteLength": norm_len, "target": 34962},
                {"buffer": 0, "byteOffset": tex_off, "byteLength": tex_len, "target": 34962},
                {"buffer": 0, "byteOffset": idx_off, "byteLength": idx_len, "target": 34963}
            ],
            "accessors": [
                {"bufferView": 0, "componentType": 5126, "count": len(self.positions), "type": "VEC3",
                 "min": [min_x, min_y, min_z], "max": [max_x, max_y, max_z]},
                {"bufferView": 1, "componentType": 5126, "count": len(self.normals), "type": "VEC3"},
                {"bufferView": 2, "componentType": 5126, "count": len(self.texcoords), "type": "VEC2"},
                {"bufferView": 3, "componentType": 5123, "count": len(self.indices), "type": "SCALAR"}
            ]
        }

        gltf_file = f"assets/models/{model_name}.gltf"
        with open(gltf_file, "w") as f:
            json.dump(gltf, f, indent=2)
        print(f"Generated {gltf_file} & {bin_file}")

# ==============================================================================
# 3. BUILD INDIVIDUAL 3D MODELS
# ==============================================================================

# --- MODEL 1: Medieval House (house_medieval) ---
b = MeshBuilder()
# Stone Ground Floor (6m wide x 8m long x 2.8m high)
b.add_box((-3.0, 0.0, -4.0), (3.0, 2.8, 4.0), UV_STONE_BRICK)
# Timber Second Floor (slightly jutting out: 6.4m x 8.4m x 2.4m)
b.add_box((-3.2, 2.8, -4.2), (3.2, 5.2, 4.2), UV_TIMBER)
# Vertical corner beam trims
b.add_box((-3.25, 2.8, -4.25), (-2.95, 5.2, -3.95), UV_TIMBER)
b.add_box((2.95, 2.8, -4.25), (3.25, 5.2, -3.95), UV_TIMBER)
b.add_box((-3.25, 2.8, 3.95), (-2.95, 5.2, 4.25), UV_TIMBER)
b.add_box((2.95, 2.8, 3.95), (3.25, 5.2, 4.25), UV_TIMBER)
# Front Wooden Door
b.add_box((-0.9, 0.0, 4.02), (0.9, 2.2, 4.12), UV_TIMBER)
# Front Window with glowing light
b.add_box((-1.8, 3.4, 4.22), (-0.8, 4.4, 4.28), UV_LANTERN)
b.add_box((0.8, 3.4, 4.22), (1.8, 4.4, 4.28), UV_LANTERN)
# Sloped Gabled Roof (Terracotta)
# Left roof slope
b.add_quad((-3.6, 5.0, 4.5), (0.0, 7.4, 4.5), (0.0, 7.4, -4.5), (-3.6, 5.0, -4.5), UV_ROOF)
# Right roof slope
b.add_quad((0.0, 7.4, 4.5), (3.6, 5.0, 4.5), (3.6, 5.0, -4.5), (0.0, 7.4, -4.5), UV_ROOF)
# Front triangular gable
b.add_quad((-3.2, 5.2, 4.2), (3.2, 5.2, 4.2), (0.0, 7.2, 4.2), (0.0, 7.2, 4.2), UV_TIMBER)
# Back triangular gable
b.add_quad((3.2, 5.2, -4.2), (-3.2, 5.2, -4.2), (0.0, 7.2, -4.2), (0.0, 7.2, -4.2), UV_TIMBER)
# Stone Chimney
b.add_box((2.2, 4.0, 1.2), (3.2, 8.2, 2.2), UV_STONE_BRICK)
b.save_gltf("house_medieval")

# --- MODEL 2: Grand Fortress City Wall & Archway Gate (city_wall_gate) ---
b = MeshBuilder()
# Left Bastion (Width 8m, Height 7.5m, Depth 3m)
b.add_box((-12.0, 0.0, -1.5), (-3.0, 7.0, 1.5), UV_STONE_BRICK)
# Right Bastion (Width 8m, Height 7.5m, Depth 3m)
b.add_box((3.0, 0.0, -1.5), (12.0, 7.0, 1.5), UV_STONE_BRICK)
# Top Archway Bridge (connecting bastions above the 6m passage)
b.add_box((-3.0, 5.0, -1.5), (3.0, 7.0, 1.5), UV_STONE_BRICK)
# Battlements / Crenellations on top (Left)
for x in [-11.0, -8.0, -5.0, 4.0, 7.0, 10.0]:
    b.add_box((x, 7.0, 1.2), (x + 1.8, 8.0, 1.6), UV_STONE_BRICK)
    b.add_box((x, 7.0, -1.6), (x + 1.8, 8.0, -1.2), UV_STONE_BRICK)
# Open Wooden Heavy Gates on hinges (Framing the opening)
b.add_box((-2.8, 0.0, -0.2), (-1.2, 4.8, 0.2), UV_TIMBER)
b.add_box((1.2, 0.0, -0.2), (2.8, 4.8, 0.2), UV_TIMBER)
# Gold crest emblem above arch
b.add_box((-0.8, 5.5, 1.52), (0.8, 6.5, 1.62), UV_GOLD)
b.save_gltf("city_wall_gate")

# --- MODEL 3: Street Lamp Post (lantern_post) ---
b = MeshBuilder()
# Stone base
b.add_box((-0.25, 0.0, -0.25), (0.25, 0.35, 0.25), UV_STONE_BRICK)
# Wrought iron post
b.add_box((-0.08, 0.35, -0.08), (0.08, 2.9, 0.08), UV_IRON)
# Hanging horizontal arm bracket
b.add_box((-0.06, 2.8, -0.06), (0.6, 2.95, 0.06), UV_IRON)
# Lantern cage
b.add_box((0.4, 2.2, -0.2), (0.8, 2.8, 0.2), UV_IRON)
# Lantern glowing glass core
b.add_box((0.44, 2.25, -0.16), (0.76, 2.75, 0.16), UV_LANTERN)
# Top peak of lantern
b.add_pyramid((0.4, 2.8, -0.2), (0.8, 2.8, 0.2), 3.1, UV_IRON)
b.save_gltf("lantern_post")

# --- MODEL 4: Town Square Stone Fountain (stone_fountain) ---
b = MeshBuilder()
# Octagonal / Quad stone pool rim (Diameter ~6m, height 0.65m)
b.add_box((-3.0, 0.0, -3.0), (3.0, 0.65, 3.0), UV_STONE_BRICK)
# Inner pool cutout filled with water
b.add_box((-2.6, 0.45, -2.6), (2.6, 0.55, 2.6), UV_WATER)
# Central decorative pillar pedestal
b.add_box((-0.7, 0.0, -0.7), (0.7, 1.6, 0.7), UV_STONE_BRICK)
# Upper fountain bowl
b.add_box((-1.2, 1.6, -1.2), (1.2, 1.9, 1.2), UV_STONE_BRICK)
# Top finial / spire
b.add_box((-0.3, 1.9, -0.3), (0.3, 2.8, 0.3), UV_GOLD)
b.save_gltf("stone_fountain")

# --- MODEL 5: Broadleaf Fantasy Oak Tree (tree_oak) ---
b = MeshBuilder()
# Trunk (tapered)
b.add_box((-0.35, 0.0, -0.35), (0.35, 3.2, 0.35), UV_TREE_BARK)
# Roots flare
b.add_box((-0.6, 0.0, -0.6), (0.6, 0.4, 0.6), UV_TREE_BARK)
# Branches
b.add_box((-1.2, 2.6, -0.3), (1.2, 3.2, 0.3), UV_TREE_BARK)
# Multi-tiered lush green foliage canopy
b.add_box((-2.2, 3.0, -2.2), (2.2, 4.8, 2.2), UV_OAK_LEAF)
b.add_box((-1.7, 4.6, -1.7), (1.7, 6.2, 1.7), UV_OAK_LEAF)
b.add_box((-1.0, 6.0, -1.0), (1.0, 7.2, 1.0), UV_OAK_LEAF)
b.save_gltf("tree_oak")

# --- MODEL 6: Evergreen Coniferous Pine (tree_pine) ---
b = MeshBuilder()
# Tall straight trunk
b.add_box((-0.25, 0.0, -0.25), (0.25, 4.5, 0.25), UV_TREE_BARK)
# Tier 1 (Lowest, widest)
b.add_pyramid((-2.2, 2.0, -2.2), (2.2, 2.0, 2.2), 4.5, UV_PINE_LEAF)
# Tier 2
b.add_pyramid((-1.7, 3.6, -1.7), (1.7, 3.6, 1.7), 6.0, UV_PINE_LEAF)
# Tier 3 (Highest, narrowest)
b.add_pyramid((-1.1, 5.2, -1.1), (1.1, 5.2, 1.1), 7.8, UV_PINE_LEAF)
b.save_gltf("tree_pine")

# --- MODEL 7: Ancient Ruin Pillar (ruin_pillar) ---
b = MeshBuilder()
# Weathered stone base
b.add_box((-0.7, 0.0, -0.7), (0.7, 0.45, 0.7), UV_MOSS_STONE)
# Column shaft
b.add_box((-0.45, 0.45, -0.45), (0.45, 3.8, 0.45), UV_MOSS_STONE)
# Cracked capital / top
b.add_box((-0.6, 3.8, -0.6), (0.6, 4.3, 0.6), UV_MOSS_STONE)
# Broken fallen pillar chunk on ground nearby
b.add_box((0.7, 0.0, 0.3), (1.8, 0.55, 1.1), UV_MOSS_STONE)
b.save_gltf("ruin_pillar")

# --- MODEL 8: Danger Zone Fire Brazier (brazier_fire) ---
b = MeshBuilder()
# Stone pedestal
b.add_box((-0.5, 0.0, -0.5), (0.5, 0.5, 0.5), UV_SCORCHED)
# Iron tripod legs
b.add_box((-0.35, 0.5, -0.35), (0.35, 1.2, 0.35), UV_IRON)
# Big iron bowl
b.add_box((-0.8, 1.2, -0.8), (0.8, 1.55, 0.8), UV_IRON)
# Burning molten coals in bowl
b.add_box((-0.7, 1.45, -0.7), (0.7, 1.6, 0.7), UV_FIRE)
# Leaping flame quads
b.add_pyramid((-0.5, 1.55, -0.5), (0.5, 1.55, 0.5), 2.6, UV_FIRE)
b.save_gltf("brazier_fire")

# --- MODEL 9: Wooden Fence (fence_wood) ---
b = MeshBuilder()
# Two vertical posts
b.add_box((-1.8, 0.0, -0.1), (-1.6, 1.25, 0.1), UV_TIMBER)
b.add_box((1.6, 0.0, -0.1), (1.8, 1.25, 0.1), UV_TIMBER)
# Two horizontal cross rails
b.add_box((-1.85, 0.4, -0.06), (1.85, 0.55, 0.06), UV_TIMBER)
b.add_box((-1.85, 0.85, -0.06), (1.85, 1.0, 0.06), UV_TIMBER)
b.save_gltf("fence_wood")

# --- MODEL 10: Natural Granite Rock Boulder (rock_boulder) ---
b = MeshBuilder()
# Main body
b.add_box((-1.2, 0.0, -0.9), (1.2, 1.3, 0.9), UV_ROCK)
# Secondary facet offset
b.add_box((-0.9, 0.2, 0.4), (0.8, 1.6, 1.3), UV_ROCK)
b.add_box((-1.4, 0.0, -0.4), (-0.7, 0.8, 0.7), UV_ROCK)
b.save_gltf("rock_boulder")

# --- MODEL 11: Wooden Supply Crate (crate_wooden) ---
b = MeshBuilder()
# Wood box
b.add_box((-0.5, 0.0, -0.5), (0.5, 1.0, 0.5), UV_TIMBER)
# Iron corner banding
b.add_box((-0.52, 0.0, -0.52), (-0.42, 1.02, -0.42), UV_IRON)
b.add_box((0.42, 0.0, -0.52), (0.52, 1.02, -0.42), UV_IRON)
b.add_box((-0.52, 0.0, 0.42), (-0.42, 1.02, 0.52), UV_IRON)
b.add_box((0.42, 0.0, 0.42), (0.52, 1.02, 0.52), UV_IRON)
b.save_gltf("crate_wooden")

print("All 11 3D environment models generated successfully!")
