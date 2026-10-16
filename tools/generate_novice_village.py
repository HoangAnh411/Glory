import os
import json
import struct
import math
import random
from PIL import Image, ImageDraw

os.makedirs("assets/textures", exist_ok=True)
os.makedirs("assets/models", exist_ok=True)
os.makedirs("assets/data", exist_ok=True)

# ==============================================================================
# 1. GENERATE NOVICE VILLAGE PALETTE (512x512)
# ==============================================================================
# 4x4 grid of 128x128 blocks:
# (0, 0): White-washed stucco plaster wall (cream/ivory with subtle grain)
# (1, 0): Dark Tudor oak timber framing (deep warm chocolate wood grain)
# (2, 0): Terracotta roof tiles (vibrant burnt orange-red with tile ridges)
# (3, 0): Glowing lantern & window glass (radiant warm amber-gold)
# (0, 1): Lush emerald green oak foliage
# (1, 1): Golden autumn maple foliage (rich radiant amber-yellow)
# (2, 1): Conifer pine needles (deep forest teal-green)
# (3, 1): Cobblestone & river stone (warm gray-beige stone blocks)
# (0, 2): Natural mountain cliff rock (stratified granite)
# (1, 2): Rugged tree bark (vertical fibrous dark brown)
# (2, 2): Golden farm straw & hay (warm harvest golden-yellow)
# (3, 2): Wildflower cluster (red, yellow, purple flower dots on green)
# (0, 3): Wrought iron & tool steel (charcoal dark metallic)
# (1, 3): Forge fire & molten coals (intense red-orange glow)
# (2, 3): Polished bronze & brass trim (radiant gold-bronze)
# (3, 3): Crystal spring water (cerulean cyan)

palette = Image.new("RGBA", (512, 512))
draw = ImageDraw.Draw(palette)

def fill_gradient_512(col, row, c1, c2, noise=6, seed=100):
    random.seed(seed + col * 17 + row * 31)
    x0, y0 = col * 128, row * 128
    x1, y1 = x0 + 128, y0 + 128
    for y in range(y0, y1):
        for x in range(x0, x1):
            t = (y - y0) / 128.0
            n = random.randint(-noise, noise)
            r = int(c1[0] * (1.0 - t) + c2[0] * t + n)
            g = int(c1[1] * (1.0 - t) + c2[1] * t + n)
            b = int(c1[2] * (1.0 - t) + c2[2] * t + n)
            palette.putpixel((x, y), (max(0, min(255, r)), max(0, min(255, g)), max(0, min(255, b)), 255))

# (0, 0): White-washed stucco plaster wall
fill_gradient_512(0, 0, (238, 234, 222), (218, 212, 198), noise=4, seed=1)
# (1, 0): Dark Tudor oak timber
fill_gradient_512(1, 0, (92, 58, 36), (58, 34, 18), noise=5, seed=2)
# Add wood grain lines
for y in range(0, 128, 6):
    draw.line([(128, y), (255, y)], fill=(48, 28, 14, 255), width=1)
# (2, 0): Terracotta roof tiles
fill_gradient_512(2, 0, (215, 82, 42), (165, 52, 24), noise=6, seed=3)
for y in range(0, 128, 16):
    draw.line([(256, y), (383, y)], fill=(120, 35, 15, 255), width=2)
for x in range(256, 384, 24):
    draw.line([(x, 0), (x, 127)], fill=(120, 35, 15, 255), width=1)
# (3, 0): Glowing lantern & window glass
fill_gradient_512(3, 0, (255, 248, 160), (255, 185, 45), noise=3, seed=4)
draw.ellipse([384 + 32, 32, 384 + 96, 96], fill=(255, 255, 210, 255))

# (0, 1): Lush emerald green foliage
fill_gradient_512(0, 1, (72, 168, 52), (38, 115, 28), noise=8, seed=5)
# (1, 1): Golden autumn maple foliage
fill_gradient_512(1, 1, (245, 192, 42), (205, 125, 22), noise=7, seed=6)
# (2, 1): Conifer pine needles
fill_gradient_512(2, 1, (36, 102, 78), (18, 62, 44), noise=6, seed=7)
# (3, 1): Cobblestone stone
fill_gradient_512(3, 1, (168, 162, 150), (122, 116, 108), noise=7, seed=8)
for y in range(128, 256, 16):
    draw.line([(384, y), (511, y)], fill=(85, 80, 72, 255), width=2)
for x in range(384, 512, 22):
    draw.line([(x, 128), (x, 255)], fill=(85, 80, 72, 255), width=1)

# (0, 2): Mountain cliff rock
fill_gradient_512(0, 2, (138, 142, 148), (92, 95, 102), noise=7, seed=9)
# (1, 2): Rugged tree bark
fill_gradient_512(1, 2, (78, 52, 34), (46, 28, 16), noise=5, seed=10)
for x in range(128, 256, 8):
    draw.line([(x, 256), (x, 383)], fill=(32, 18, 10, 255), width=1)
# (2, 2): Golden straw & hay
fill_gradient_512(2, 2, (235, 205, 95), (185, 150, 48), noise=8, seed=11)
# (3, 2): Wildflower cluster (green background with colorful dots)
fill_gradient_512(3, 2, (68, 145, 45), (42, 105, 28), noise=5, seed=12)
flower_colors = [(255, 65, 65), (255, 225, 45), (210, 95, 245), (255, 255, 255)]
for _ in range(60):
    fx = random.randint(384 + 8, 512 - 8)
    fy = random.randint(256 + 8, 384 - 8)
    fc = random.choice(flower_colors)
    draw.ellipse([fx - 2, fy - 2, fx + 2, fy + 2], fill=fc + (255,))

# (0, 3): Wrought iron
fill_gradient_512(0, 3, (58, 62, 68), (28, 30, 34), noise=4, seed=13)
# (1, 3): Forge fire & coals
fill_gradient_512(1, 3, (255, 145, 25), (235, 40, 10), noise=8, seed=14)
# (2, 3): Polished bronze/brass
fill_gradient_512(2, 3, (225, 185, 65), (165, 125, 30), noise=5, seed=15)
# (3, 3): Crystal spring water
fill_gradient_512(3, 3, (72, 192, 235), (32, 135, 195), noise=5, seed=16)

palette.save("assets/textures/novice_village_palette.png")
print("Saved assets/textures/novice_village_palette.png")

# Also copy/overwrite environment_palette.png so any existing references get this rich palette
palette.save("assets/textures/environment_palette.png")

# UV Helper for 128x128 blocks in 512x512 palette
def uv_block(col, row, u_min=0.08, v_min=0.08, u_max=0.92, v_max=0.92):
    u0 = (col * 128.0 + u_min * 128.0) / 512.0
    u1 = (col * 128.0 + u_max * 128.0) / 512.0
    v0 = (row * 128.0 + v_min * 128.0) / 512.0
    v1 = (row * 128.0 + v_max * 128.0) / 512.0
    return [(u0, v0), (u1, v0), (u1, v1), (u0, v1)]

UV_WHITE_PLASTER = uv_block(0, 0)
UV_TIMBER        = uv_block(1, 0)
UV_ROOF_RED      = uv_block(2, 0)
UV_GLOW_LANTERN  = uv_block(3, 0)
UV_OAK_LEAF      = uv_block(0, 1)
UV_MAPLE_GOLD    = uv_block(1, 1)
UV_PINE_LEAF     = uv_block(2, 1)
UV_COBBLESTONE   = uv_block(3, 1)
UV_CLIFF_ROCK    = uv_block(0, 2)
UV_TREE_BARK     = uv_block(1, 2)
UV_HAY_STRAW     = uv_block(2, 2)
UV_WILDFLOWER    = uv_block(3, 2)
UV_IRON          = uv_block(0, 3)
UV_FORGE_FIRE    = uv_block(1, 3)
UV_BRASS_GOLD    = uv_block(2, 3)
UV_WATER         = uv_block(3, 3)

# ==============================================================================
# 2. GENERATE CRISP HIGH-RES NOVICE VILLAGE GROUND TEXTURE (1024x1024)
# ==============================================================================
ground_img = Image.new("RGBA", (1024, 1024))
g_draw = ImageDraw.Draw(ground_img)

# Generate a lush, rich fantasy village meadow with warm dirt path and stone pavers
random.seed(777)
for y in range(1024):
    for x in range(1024):
        n = random.randint(-4, 4)
        # Village town square center: round cobblestone circle at x=512, y=680
        dx = x - 512
        dy = y - 680
        dist_sq = math.sqrt(dx * dx + dy * dy)
        dist_path = abs(dx) - (38.0 + math.sin(y * 0.03) * 16.0)

        if dist_sq < 140.0:
            # Center Cobblestone Plaza
            is_joint = (x % 24 < 3) or (y % 20 < 3)
            c = (152 + n, 146 + n, 138 + n) if not is_joint else (92, 86, 78)
        elif dist_path < 0.0 and y < 850:
            # Winding Village Sandy Dirt Path
            c = (176 + n, 148 + n, 112 + n)
        else:
            # Lush Green Grass with subtle field noise
            c = (82 + n, 162 + n, 58 + n)

        ground_img.putpixel((x, y), (max(0, min(255, c[0])), max(0, min(255, c[1])), max(0, min(255, c[2])), 255))

# Add scattered wildflowers to the grass
for _ in range(800):
    fx = random.randint(10, 1014)
    fy = random.randint(10, 1014)
    dx = fx - 512
    dy = fy - 680
    if math.sqrt(dx * dx + dy * dy) > 150.0 and (abs(dx) > 60.0 or fy >= 850):
        fc = random.choice([(255, 235, 50), (255, 80, 80), (230, 120, 255), (255, 255, 255)])
        g_draw.ellipse([fx - 2, fy - 2, fx + 2, fy + 2], fill=fc + (255,))

ground_img.save("assets/textures/novice_village_ground.png")
ground_img.save("assets/textures/world_ground.png") # Also overwrite world_ground.png
print("Saved assets/textures/novice_village_ground.png & world_ground.png")

# ==============================================================================
# 3. 3D GLTF MESH BUILDER
# ==============================================================================

class MeshBuilder:
    def __init__(self):
        self.positions = []
        self.normals = []
        self.texcoords = []
        self.indices = []

    def add_quad(self, p0, p1, p2, p3, uv_coords):
        v01 = (p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2])
        v02 = (p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2])
        nx = v01[1] * v02[2] - v01[2] * v02[1]
        ny = v01[2] * v02[0] - v01[0] * v02[2]
        nz = v01[0] * v02[1] - v01[1] * v02[0]
        length = math.sqrt(nx * nx + ny * ny + nz * nz)
        norm = (nx / length, ny / length, nz / length) if length > 1e-6 else (0.0, 1.0, 0.0)

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
        for (p0, p1) in [((x0, y0, z1), (x1, y0, z1)),
                         ((x1, y0, z1), (x1, y0, z0)),
                         ((x1, y0, z0), (x0, y0, z0)),
                         ((x0, y0, z0), (x0, y0, z1))]:
            self.add_quad(p0, p1, tip, tip, uv_coords)

    def add_cylinder(self, center, radius, height, sides, uv_coords):
        cx, cy, cz = center
        angles = [i * 2.0 * math.pi / sides for i in range(sides)]
        bottom_pts = [(cx + radius * math.cos(a), cy, cz + radius * math.sin(a)) for a in angles]
        top_pts = [(cx + radius * math.cos(a), cy + height, cz + radius * math.sin(a)) for a in angles]
        for i in range(sides):
            nxt = (i + 1) % sides
            self.add_quad(bottom_pts[i], bottom_pts[nxt], top_pts[nxt], top_pts[i], uv_coords)
        # Top cap
        top_center = (cx, cy + height, cz)
        for i in range(sides):
            nxt = (i + 1) % sides
            self.add_quad(top_pts[i], top_pts[nxt], top_center, top_center, uv_coords)

    def save_gltf(self, model_name, texture_uri="../textures/novice_village_palette.png"):
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
            "asset": {"version": "2.0", "generator": "Glory Novice Village Generator"},
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
                    "metallicFactor": 0.05,
                    "roughnessFactor": 0.85
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
        print(f"Generated {gltf_file}")

# ==============================================================================
# 4. BUILD 18 DISTINCT 3D NOVICE VILLAGE MODELS
# ==============================================================================

# --- 1. Quán Trọ Tân Thủ 2 Tầng (house_town_inn) ---
b = MeshBuilder()
# Tầng 1: Đá cuội kiên cố (Rộng 9m, Dài 11m, Cao 3.2m)
b.add_box((-4.5, 0.0, -5.5), (4.5, 3.2, 5.5), UV_COBBLESTONE)
# Tầng 2: Tường vôi trắng ngà Tudor (Rộng 4.8m x 5.8m x 3.0m)
b.add_box((-4.8, 3.2, -5.8), (4.8, 6.2, 5.8), UV_WHITE_PLASTER)
# Khung xà gỗ Tudor trang trí
for z in [-5.85, 5.85]:
    for x in [-4.8, -2.4, 0.0, 2.4, 4.8]:
        b.add_box((x - 0.15, 3.2, z - 0.05), (x + 0.15, 6.2, z + 0.05), UV_TIMBER)
for x in [-4.85, 4.85]:
    for z in [-5.8, -2.9, 0.0, 2.9, 5.8]:
        b.add_box((x - 0.05, 3.2, z - 0.15), (x + 0.05, 6.2, z + 0.15), UV_TIMBER)
# Ban công gỗ tầng 2 phía trước
b.add_box((-3.0, 3.2, 5.8), (3.0, 3.4, 7.2), UV_TIMBER)
b.add_box((-3.0, 3.4, 7.1), (3.0, 4.4, 7.2), UV_TIMBER)
# Cửa gỗ quán trọ
b.add_box((-1.2, 0.0, 5.52), (1.2, 2.6, 5.62), UV_TIMBER)
# Biển hiệu quán trọ gỗ treo trước cửa
b.add_box((-1.5, 3.2, 6.0), (-1.3, 3.8, 6.6), UV_TIMBER)
# Các cửa sổ phát sáng ấm áp
b.add_box((-3.6, 4.0, 5.82), (-2.0, 5.4, 5.88), UV_GLOW_LANTERN)
b.add_box((2.0, 4.0, 5.82), (3.6, 5.4, 5.88), UV_GLOW_LANTERN)
# Mái ngói đỏ cam dốc nghiêng
b.add_quad((-5.2, 6.0, 6.2), (0.0, 8.8, 6.2), (0.0, 8.8, -6.2), (-5.2, 6.0, -6.2), UV_ROOF_RED)
b.add_quad((0.0, 8.8, 6.2), (5.2, 6.0, 6.2), (5.2, 6.0, -6.2), (0.0, 8.8, -6.2), UV_ROOF_RED)
b.add_quad((-4.8, 6.2, 5.8), (4.8, 6.2, 5.8), (0.0, 8.6, 5.8), (0.0, 8.6, 5.8), UV_WHITE_PLASTER)
b.add_quad((4.8, 6.2, -5.8), (-4.8, 6.2, -5.8), (0.0, 8.6, -5.8), (0.0, 8.6, -5.8), UV_WHITE_PLASTER)
# Ống khói đá
b.add_box((3.2, 5.0, 2.0), (4.4, 9.6, 3.2), UV_COBBLESTONE)
b.save_gltf("house_town_inn")

# --- 2. Nhà Dân Thôn Quê Ấm Cúng (house_cottage) ---
b = MeshBuilder()
# Thân nhà vôi trắng Tudor (Rộng 6.5m, Dài 7m, Cao 3.2m)
b.add_box((-3.25, 0.0, -3.5), (3.25, 3.2, 3.5), UV_WHITE_PLASTER)
# Trụ xà gỗ 4 góc
for (cx, cz) in [(-3.3, -3.55), (3.2, -3.55), (-3.3, 3.45), (3.2, 3.45)]:
    b.add_box((cx, 0.0, cz), (cx + 0.2, 3.2, cz + 0.2), UV_TIMBER)
# Cửa gỗ đỏ
b.add_box((-0.9, 0.0, 3.52), (0.9, 2.2, 3.62), UV_TIMBER)
# Cửa sổ hai bên
b.add_box((-2.5, 1.2, 3.52), (-1.5, 2.2, 3.58), UV_GLOW_LANTERN)
b.add_box((1.5, 1.2, 3.52), (2.5, 2.2, 3.58), UV_GLOW_LANTERN)
# Bồn hoa nhỏ trước nhà
b.add_box((-2.6, 0.0, 3.55), (-1.4, 0.45, 4.15), UV_WILDFLOWER)
b.add_box((1.4, 0.0, 3.55), (2.6, 0.45, 4.15), UV_WILDFLOWER)
# Mái ngói đỏ cam dốc cao
b.add_quad((-3.8, 3.0, 3.9), (0.0, 6.2, 3.9), (0.0, 6.2, -3.9), (-3.8, 3.0, -3.9), UV_ROOF_RED)
b.add_quad((0.0, 6.2, 3.9), (3.8, 3.0, 3.9), (3.8, 3.0, -3.9), (0.0, 6.2, -3.9), UV_ROOF_RED)
b.add_quad((-3.25, 3.2, 3.5), (3.25, 3.2, 3.5), (0.0, 6.0, 3.5), (0.0, 6.0, 3.5), UV_WHITE_PLASTER)
b.add_quad((3.25, 3.2, -3.5), (-3.25, 3.2, -3.5), (0.0, 6.0, -3.5), (0.0, 6.0, -3.5), UV_WHITE_PLASTER)
# Ống khói nhỏ
b.add_box((-2.6, 4.0, -1.2), (-1.8, 7.0, -0.4), UV_COBBLESTONE)
b.save_gltf("house_cottage")

# --- 3. Tiệm Rèn Tân Thủ (house_blacksmith) ---
b = MeshBuilder()
# Nền xưởng đá (6.5m x 6m)
b.add_box((-3.25, 0.0, -3.0), (3.25, 0.3, 3.0), UV_COBBLESTONE)
# 4 Cột gỗ chống mái
for (cx, cz) in [(-3.0, -2.8), (3.0, -2.8), (-3.0, 2.8), (3.0, 2.8)]:
    b.add_box((cx - 0.2, 0.3, cz - 0.2), (cx + 0.2, 3.8, cz + 0.2), UV_TIMBER)
# Mái che gỗ ngói dốc
b.add_box((-3.5, 3.8, -3.3), (3.5, 4.2, 3.3), UV_ROOF_RED)
# Lò than rèn lửa đỏ rực
b.add_box((-2.5, 0.3, -2.5), (-0.8, 1.8, -1.0), UV_COBBLESTONE)
b.add_box((-2.2, 1.4, -2.2), (-1.1, 1.85, -1.3), UV_FORGE_FIRE)
# Ống khói lò rèn vươn qua mái
b.add_box((-2.3, 1.8, -2.3), (-1.0, 5.8, -1.1), UV_IRON)
# Đe sắt rèn kiếm
b.add_box((0.8, 0.3, -0.6), (1.4, 0.8, 0.0), UV_TIMBER)
b.add_box((0.6, 0.8, -0.8), (1.6, 1.3, 0.2), UV_IRON)
# Bể nước làm nguội thép
b.add_box((-2.5, 0.3, 1.0), (-1.2, 1.1, 2.4), UV_TIMBER)
b.add_box((-2.3, 0.9, 1.2), (-1.4, 1.05, 2.2), UV_WATER)
b.save_gltf("house_blacksmith")

# --- 4. Cối Xay Gió Thôn Quê (village_windmill) ---
b = MeshBuilder()
# Tháp đá tròn hình nón cụt
b.add_cylinder((0.0, 0.0, 0.0), 2.8, 8.5, 12, UV_COBBLESTONE)
# Cửa gỗ vào cối xay
b.add_box((-0.8, 0.0, 2.6), (0.8, 2.1, 2.9), UV_TIMBER)
# Mái chóp nhọn trên đỉnh
b.add_pyramid((-2.4, 8.5, -2.4), (2.4, 8.5, 2.4), 11.2, UV_ROOF_RED)
# Trục xoay cánh quạt
b.add_box((-0.2, 9.2, 2.3), (0.2, 9.6, 3.4), UV_TIMBER)
# 4 Cánh quạt gỗ đan nan
b.add_box((-4.5, 9.2, 3.3), (4.5, 9.6, 3.4), UV_TIMBER)
b.add_box((-0.2, 5.0, 3.3), (0.2, 13.8, 3.4), UV_TIMBER)
# Nan vải cánh quạt
b.add_box((0.5, 9.6, 3.32), (4.2, 10.6, 3.38), UV_WHITE_PLASTER)
b.add_box((-4.2, 8.2, 3.32), (-0.5, 9.2, 3.38), UV_WHITE_PLASTER)
b.add_box((0.2, 10.0, 3.32), (1.2, 13.5, 3.38), UV_WHITE_PLASTER)
b.add_box((-1.2, 5.3, 3.32), (-0.2, 8.8, 3.38), UV_WHITE_PLASTER)
b.save_gltf("village_windmill")

# --- 5. Cổng Làng & Tháp Canh Gỗ (village_gate_tower) ---
b = MeshBuilder()
# Tháp canh Trái (Rộng 3m, Sâu 3m, Cao 7.5m)
b.add_box((-8.5, 0.0, -1.5), (-5.5, 6.0, 1.5), UV_TIMBER)
b.add_pyramid((-6.0, 6.0, -1.8), (-3.0, 6.0, 1.8), 8.5, UV_ROOF_RED)
# Tháp canh Phải (Rộng 3m, Sâu 3m, Cao 7.5m)
b.add_box((5.5, 0.0, -1.5), (8.5, 6.0, 1.5), UV_TIMBER)
b.add_pyramid((3.0, 6.0, -1.8), (6.0, 6.0, 1.8), 8.5, UV_ROOF_RED)
# Cầu gỗ trên cổng nối hai tháp
b.add_box((-5.5, 4.2, -1.2), (5.5, 5.5, 1.2), UV_TIMBER)
# Lan can cầu gỗ
b.add_box((-5.5, 5.5, 1.1), (5.5, 6.4, 1.2), UV_TIMBER)
b.add_box((-5.5, 5.5, -1.2), (5.5, 6.4, -1.1), UV_TIMBER)
# Cổng gỗ mở rộng (lối đi x in [-4.5, 4.5], y in [0, 4.2])
b.add_box((-5.2, 0.0, -0.2), (-3.2, 4.0, 0.2), UV_TIMBER)
b.add_box((3.2, 0.0, -0.2), (5.2, 4.0, 0.2), UV_TIMBER)
# Đèn lồng treo hai bên cổng
b.add_box((-4.2, 3.0, 1.2), (-3.8, 3.8, 1.6), UV_GLOW_LANTERN)
b.add_box((3.8, 3.0, 1.2), (4.2, 3.8, 1.6), UV_GLOW_LANTERN)
b.save_gltf("village_gate_tower")

# --- 6. Giếng Nước Đá Cuội Làng Quê (village_well) ---
b = MeshBuilder()
# Vành giếng đá cuội tròn (Bán kính 1.4m, Cao 0.85m)
b.add_cylinder((0.0, 0.0, 0.0), 1.4, 0.85, 10, UV_COBBLESTONE)
# Mặt nước giếng trong vắt
b.add_cylinder((0.0, 0.65, 0.0), 1.1, 0.05, 8, UV_WATER)
# Hai cột gỗ chống mái
b.add_box((-1.2, 0.85, -0.1), (-1.0, 2.6, 0.1), UV_TIMBER)
b.add_box((1.0, 0.85, -0.1), (1.2, 2.6, 0.1), UV_TIMBER)
# Trục ròng rọc cuốn dây thừng
b.add_box((-1.0, 2.2, -0.1), (1.0, 2.4, 0.1), UV_TIMBER)
# Xô múc nước gỗ
b.add_box((-0.25, 1.2, -0.25), (0.25, 1.7, 0.25), UV_TIMBER)
# Mái che gỗ ngói đỏ
b.add_quad((-1.6, 2.4, 1.4), (0.0, 3.3, 1.4), (0.0, 3.3, -1.4), (-1.6, 2.4, -1.4), UV_ROOF_RED)
b.add_quad((0.0, 3.3, 1.4), (1.6, 2.4, 1.4), (1.6, 2.4, -1.4), (0.0, 3.3, -1.4), UV_ROOF_RED)
b.save_gltf("village_well")

# --- 7. Cây Đại Thụ Đầu Làng (tree_village_oak) ---
b = MeshBuilder()
# Thân đại thụ gồ ghề
b.add_cylinder((0.0, 0.0, 0.0), 0.75, 4.8, 8, UV_TREE_BARK)
# Rễ cây tỏa ra xung quanh
for angle in [0.0, 1.2, 2.4, 3.6, 4.8]:
    rx = 1.3 * math.cos(angle)
    rz = 1.3 * math.sin(angle)
    b.add_box((min(0, rx), 0.0, min(0, rz)), (max(0, rx), 0.6, max(0, rz)), UV_TREE_BARK)
# Tán lá tầng 1 (Rộng 8m)
b.add_cylinder((0.0, 3.8, 0.0), 3.8, 2.4, 10, UV_OAK_LEAF)
# Tán lá tầng 2 (Rộng 6m)
b.add_cylinder((0.0, 5.8, 0.0), 2.8, 2.4, 8, UV_OAK_LEAF)
# Tán lá đỉnh (Rộng 3.5m, chóp cao 10m)
b.add_pyramid((-1.8, 7.8, -1.8), (1.8, 7.8, 1.8), 10.2, UV_OAK_LEAF)
b.save_gltf("tree_village_oak")

# --- 8. Cây Phong Lá Vàng Mùa Thu (tree_birch_gold) ---
b = MeshBuilder()
# Thân gỗ sáng màu
b.add_cylinder((0.0, 0.0, 0.0), 0.35, 4.2, 6, UV_WHITE_PLASTER)
# Tán lá vàng rực rỡ bồng bềnh
b.add_cylinder((0.0, 3.5, 0.0), 2.4, 2.0, 8, UV_MAPLE_GOLD)
b.add_cylinder((0.0, 5.0, 0.0), 1.7, 1.8, 7, UV_MAPLE_GOLD)
b.add_pyramid((-1.2, 6.5, -1.2), (1.2, 6.5, 1.2), 8.2, UV_MAPLE_GOLD)
b.save_gltf("tree_birch_gold")

# --- 9. Cây Thông Xanh Triền Đồi (tree_conifer_pine) ---
b = MeshBuilder()
b.add_cylinder((0.0, 0.0, 0.0), 0.3, 5.0, 6, UV_TREE_BARK)
b.add_pyramid((-2.4, 2.2, -2.4), (2.4, 2.2, 2.4), 5.0, UV_PINE_LEAF)
b.add_pyramid((-1.8, 4.2, -1.8), (1.8, 4.2, 1.8), 7.0, UV_PINE_LEAF)
b.add_pyramid((-1.2, 6.2, -1.2), (1.2, 6.2, 1.2), 9.2, UV_PINE_LEAF)
b.save_gltf("tree_conifer_pine")

# --- 10. Bụi Hoa Dại Đầy Màu Sắc (bush_flower_cluster) ---
b = MeshBuilder()
b.add_cylinder((0.0, 0.0, 0.0), 1.2, 0.7, 8, UV_OAK_LEAF)
b.add_cylinder((0.0, 0.5, 0.0), 0.9, 0.6, 7, UV_WILDFLOWER)
b.save_gltf("bush_flower_cluster")

# --- 11. Cột Đèn Lồng Gỗ Thắp Sáng (lamp_wooden_lantern) ---
b = MeshBuilder()
# Đế đá
b.add_box((-0.35, 0.0, -0.35), (0.35, 0.4, 0.35), UV_COBBLESTONE)
# Cột gỗ mộc
b.add_box((-0.1, 0.4, -0.1), (0.1, 2.8, 0.1), UV_TIMBER)
# Thanh ngang treo đèn
b.add_box((-0.1, 2.6, -0.1), (0.65, 2.8, 0.1), UV_TIMBER)
# Đèn lồng kính phát sáng
b.add_box((0.45, 2.0, -0.2), (0.85, 2.6, 0.2), UV_GLOW_LANTERN)
# Mái che đèn
b.add_pyramid((0.4, 2.6, -0.25), (0.9, 2.6, 0.25), 2.9, UV_IRON)
b.save_gltf("lamp_wooden_lantern")

# --- 12. Xe Rơm Thôn Quê (cart_wooden_hay) ---
b = MeshBuilder()
# Thùng xe gỗ (Dài 3m, Rộng 1.8m)
b.add_box((-0.9, 0.5, -1.5), (0.9, 1.2, 1.5), UV_TIMBER)
# Đống rơm vàng chất cao ngun ngút
b.add_box((-0.85, 1.1, -1.4), (0.85, 2.2, 1.4), UV_HAY_STRAW)
b.add_pyramid((-0.7, 2.1, -1.2), (0.7, 2.1, 1.2), 2.7, UV_HAY_STRAW)
# 2 Bánh xe gỗ lớn
b.add_cylinder((-1.05, 0.0, 0.0), 0.65, 0.15, 8, UV_TIMBER)
b.add_cylinder((0.9, 0.0, 0.0), 0.65, 0.15, 8, UV_TIMBER)
# Càng xe kéo
b.add_box((-0.4, 0.6, 1.5), (-0.3, 0.7, 3.2), UV_TIMBER)
b.add_box((0.3, 0.6, 1.5), (0.4, 0.7, 3.2), UV_TIMBER)
b.save_gltf("cart_wooden_hay")

# --- 13. Cụm Thùng Rượu Gỗ Sồi (barrel_stack) ---
b = MeshBuilder()
# Thùng 1 (Dưới trái)
b.add_cylinder((-0.45, 0.0, -0.2), 0.42, 0.95, 8, UV_TIMBER)
# Thùng 2 (Dưới phải)
b.add_cylinder((0.45, 0.0, -0.2), 0.42, 0.95, 8, UV_TIMBER)
# Thùng 3 (Xếp chồng phía trên)
b.add_cylinder((0.0, 0.85, 0.1), 0.40, 0.95, 8, UV_TIMBER)
b.save_gltf("barrel_stack")

# --- 14. Băng Ghế Gỗ Dừng Chân (bench_wooden) ---
b = MeshBuilder()
# Mặt ghế
b.add_box((-1.1, 0.45, -0.3), (1.1, 0.55, 0.3), UV_TIMBER)
# Tựa lưng ghế
b.add_box((-1.1, 0.55, -0.3), (1.1, 1.05, -0.2), UV_TIMBER)
# 4 Chân ghế gỗ
for (bx, bz) in [(-0.95, -0.22), (0.95, -0.22), (-0.95, 0.22), (0.95, 0.22)]:
    b.add_box((bx - 0.06, 0.0, bz - 0.06), (bx + 0.06, 0.45, bz + 0.06), UV_TIMBER)
b.save_gltf("bench_wooden")

# --- 15. Hàng Rào Gỗ Nông Trang (fence_farm_wood) ---
b = MeshBuilder()
# 2 Cọc rào đứng
b.add_box((-1.8, 0.0, -0.08), (-1.65, 1.15, 0.08), UV_TIMBER)
b.add_box((1.65, 0.0, -0.08), (1.8, 1.15, 0.08), UV_TIMBER)
# 2 Thanh xà ngang
b.add_box((-1.85, 0.35, -0.05), (1.85, 0.48, 0.05), UV_TIMBER)
b.add_box((-1.85, 0.78, -0.05), (1.85, 0.91, 0.05), UV_TIMBER)
b.save_gltf("fence_farm_wood")

# --- 16. Biển Chỉ Đường Làng Quê (signpost_wooden) ---
b = MeshBuilder()
# Cột gỗ
b.add_box((-0.1, 0.0, -0.1), (0.1, 2.5, 0.1), UV_TIMBER)
# Bảng 1 chỉ trái ("Quán Trọ")
b.add_box((-1.2, 2.0, -0.04), (0.1, 2.3, 0.04), UV_TIMBER)
# Bảng 2 chỉ phải ("Cổng Làng")
b.add_box((-0.1, 1.6, -0.04), (1.2, 1.9, 0.04), UV_TIMBER)
# Bảng 3 chỉ thẳng ("Luyện Cấp")
b.add_box((-0.04, 1.2, -0.1), (0.04, 1.5, 1.2), UV_TIMBER)
b.save_gltf("signpost_wooden")

# --- 17. Đồi Cỏ Xanh Uốn Lượn (terrain_hill_green) ---
b = MeshBuilder()
# Đồi cỏ lớn (Rộng 28m, Dài 22m, Cao 8.5m)
b.add_pyramid((-14.0, 0.0, -11.0), (14.0, 0.0, 11.0), 8.5, UV_OAK_LEAF)
# Lớp cỏ đệm chân đồi
b.add_box((-14.5, 0.0, -11.5), (14.5, 1.5, 11.5), UV_OAK_LEAF)
b.save_gltf("terrain_hill_green")

# --- 18. Vách Đá Núi Sừng Sững (terrain_cliff_rock) ---
b = MeshBuilder()
# Khối vách đá cao tầng (Rộng 24m, Dài 12m, Cao 13m)
b.add_box((-12.0, 0.0, -6.0), (12.0, 7.0, 6.0), UV_CLIFF_ROCK)
b.add_box((-10.5, 6.8, -5.0), (10.5, 13.0, 5.0), UV_CLIFF_ROCK)
# Lớp rêu phủ đỉnh vách đá
b.add_box((-10.8, 12.8, -5.2), (10.8, 13.4, 5.2), UV_OAK_LEAF)
b.save_gltf("terrain_cliff_rock")

print("All 18 Novice Village 3D models generated successfully!")
