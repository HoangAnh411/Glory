import struct
import json
import math
import os

def quat_from_axis_angle(axis, degrees):
    rad = math.radians(degrees) * 0.5
    s = math.sin(rad)
    c = math.cos(rad)
    length = math.sqrt(axis[0]**2 + axis[1]**2 + axis[2]**2)
    if length < 1e-6:
        return [0.0, 0.0, 0.0, 1.0]
    return [axis[0]/length * s, axis[1]/length * s, axis[2]/length * s, c]

# Palettes UV ranges (128x128):
# Quad 0,0: Dark Iron Armor -> (0.05, 0.45), (0.05, 0.45)
# Quad 1,0: Glowing Magma Runes -> (0.55, 0.95), (0.05, 0.45)
# Quad 0,1: Heavy Bronze / Warhammer -> (0.05, 0.45), (0.55, 0.95)
# Quad 1,1: Black Spikes & Horns -> (0.55, 0.95), (0.55, 0.95)

UV_DARK_IRON = ((0.05, 0.45), (0.05, 0.45))
UV_MAGMA_RUNE = ((0.55, 0.95), (0.05, 0.45))
UV_HAMMER = ((0.05, 0.45), (0.55, 0.95))
UV_BLACK_SPIKE = ((0.55, 0.95), (0.55, 0.95))

positions = []
normals = []
texcoords = []
joints = []
weights = []
indices = []

def add_box(min_pt, max_pt, joint_idx, uv_range):
    x0, y0, z0 = min_pt
    x1, y1, z1 = max_pt
    (u0, u1), (v0, v1) = uv_range

    faces = [
        ([(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)], (0, 0, 1)),
        ([(x1, y0, z0), (x0, y0, z0), (x0, y1, z0), (x1, y1, z0)], (0, 0, -1)),
        ([(x1, y0, z1), (x1, y0, z0), (x1, y1, z0), (x1, y1, z1)], (1, 0, 0)),
        ([(x0, y0, z0), (x0, y0, z1), (x0, y1, z1), (x0, y1, z0)], (-1, 0, 0)),
        ([(x0, y1, z1), (x1, y1, z1), (x1, y1, z0), (x0, y1, z0)], (0, 1, 0)),
        ([(x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1)], (0, -1, 0)),
    ]

    for face_verts, norm in faces:
        base_idx = len(positions)
        uvs = [(u0, v0), (u1, v0), (u1, v1), (u0, v1)]
        for i in range(4):
            positions.append(face_verts[i])
            normals.append(norm)
            texcoords.append(uvs[i])
            joints.append((joint_idx, 0, 0, 0))
            weights.append((1.0, 0.0, 0.0, 0.0))
        indices.extend([base_idx, base_idx + 1, base_idx + 2, base_idx, base_idx + 2, base_idx + 3])

# Construct Boss Gorgon (The Iron Warden) - Height ~ 2.8m, very wide and imposing
# Joint 2: Head & Horns
add_box((-0.26, 1.85, -0.26), (0.26, 2.30, 0.26), 2, UV_DARK_IRON) # Greathelm
add_box((-0.20, 1.95, 0.25), (0.20, 2.05, 0.30), 2, UV_MAGMA_RUNE) # Glowing Red Visor Slit
# Left & Right Horns
add_box((-0.42, 2.15, -0.05), (-0.26, 2.55, 0.05), 2, UV_BLACK_SPIKE)
add_box((0.26, 2.15, -0.05), (0.42, 2.55, 0.05), 2, UV_BLACK_SPIKE)

# Joint 1: Torso & Spiked Chestplate
add_box((-0.45, 1.35, -0.32), (0.45, 1.85, 0.32), 1, UV_DARK_IRON) # Heavy Chest
add_box((-0.25, 1.45, 0.31), (0.25, 1.75, 0.35), 1, UV_MAGMA_RUNE) # Magma chest rune
add_box((-0.38, 0.95, -0.28), (0.38, 1.35, 0.28), 1, UV_DARK_IRON) # Waist / Belt

# Joint 3: Left Arm & Massive Spiked Pauldron
add_box((-0.72, 1.60, -0.28), (-0.45, 1.92, 0.28), 3, UV_DARK_IRON) # Huge Pauldron
add_box((-0.65, 1.92, -0.08), (-0.52, 2.22, 0.08), 3, UV_BLACK_SPIKE) # Shoulder Spike
add_box((-0.62, 1.20, -0.20), (-0.45, 1.60, 0.20), 3, UV_DARK_IRON) # Upper Arm
add_box((-0.62, 0.75, -0.20), (-0.45, 1.20, 0.20), 3, UV_BLACK_SPIKE) # Iron Gauntlet

# Joint 4: Right Arm (Warhammer Arm)
add_box((0.45, 1.60, -0.28), (0.72, 1.92, 0.28), 4, UV_DARK_IRON) # Huge Pauldron
add_box((0.52, 1.92, -0.08), (0.65, 2.22, 0.08), 4, UV_BLACK_SPIKE) # Shoulder Spike
add_box((0.45, 1.20, -0.20), (0.62, 1.60, 0.20), 4, UV_DARK_IRON) # Upper Arm
add_box((0.45, 0.75, -0.20), (0.62, 1.20, 0.20), 4, UV_BLACK_SPIKE) # Iron Gauntlet

# Colossal Warhammer attached to Right Arm (Joint 4)
add_box((0.50, 0.20, 0.10), (0.57, 1.90, 0.17), 4, UV_HAMMER) # Long Steel Shaft
# Giant Warhammer Head at the top
add_box((0.35, 1.70, -0.15), (0.72, 2.10, 0.42), 4, UV_HAMMER) # Huge Hammer Head
add_box((0.38, 1.75, 0.42), (0.69, 2.05, 0.52), 4, UV_MAGMA_RUNE) # Glowing impact face

# Joint 5: Left Leg
add_box((-0.38, 0.48, -0.22), (-0.10, 0.95, 0.22), 5, UV_DARK_IRON) # Heavy Thigh
add_box((-0.38, 0.00, -0.22), (-0.10, 0.48, 0.26), 5, UV_BLACK_SPIKE) # Spiked Boot

# Joint 6: Right Leg
add_box((0.10, 0.48, -0.22), (0.38, 0.95, 0.22), 6, UV_DARK_IRON) # Heavy Thigh
add_box((0.10, 0.00, -0.22), (0.38, 0.48, 0.26), 6, UV_BLACK_SPIKE) # Spiked Boot

# Inverse Bind Matrices
def make_inv_bind(tx, ty, tz):
    return [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        -tx, -ty, -tz, 1.0
    ]

inv_bind_matrices = [
    make_inv_bind(0.0, 0.0, 0.0),       # 0: Root
    make_inv_bind(0.0, 0.95, 0.0),      # 1: Torso
    make_inv_bind(0.0, 1.85, 0.0),      # 2: Head
    make_inv_bind(-0.50, 1.70, 0.0),    # 3: LeftArm
    make_inv_bind(0.50, 1.70, 0.0),     # 4: RightArm
    make_inv_bind(-0.24, 0.95, 0.0),    # 5: LeftLeg
    make_inv_bind(0.24, 0.95, 0.0),     # 6: RightLeg
]

# Animations
idle_times = [0.0, 1.0, 2.0]
run_times  = [0.0, 0.3, 0.6, 0.9, 1.2]
attack_times = [0.0, 0.35, 0.55, 0.85, 1.20]

idle_torso = [
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), -3.0),
    quat_from_axis_angle((1, 0, 0), 0.0)
]
idle_rarm = [
    quat_from_axis_angle((1, 0, 0), -20.0),
    quat_from_axis_angle((1, 0, 0), -16.0),
    quat_from_axis_angle((1, 0, 0), -20.0)
]

# Run
run_lleg = [
    quat_from_axis_angle((1, 0, 0), 22.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), -22.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), 22.0)
]
run_rleg = [
    quat_from_axis_angle((1, 0, 0), -22.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), 22.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), -22.0)
]
run_rarm = [
    quat_from_axis_angle((1, 0, 0), 20.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), -20.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), 20.0)
]

# Attack: Giant Hammer Overhead Slam
attack_rarm = [
    quat_from_axis_angle((1, 0, 0), -20.0),
    quat_from_axis_angle((1, 0, 0), 75.0),   # Raise high overhead
    quat_from_axis_angle((1, 0, 0), -80.0),  # Heavy Ground Slam!
    quat_from_axis_angle((1, 0, 0), -45.0),
    quat_from_axis_angle((1, 0, 0), -20.0)
]
attack_torso = [
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), -15.0),  # Arch back
    quat_from_axis_angle((1, 0, 0), 30.0),   # Slam forward
    quat_from_axis_angle((1, 0, 0), 10.0),
    quat_from_axis_angle((1, 0, 0), 0.0)
]

# Write Binary Buffer
bin_data = bytearray()
def write_floats(data):
    start = len(bin_data)
    for val in data: bin_data.extend(struct.pack('<f', val))
    return start, len(bin_data) - start

def write_uint16(data):
    start = len(bin_data)
    for val in data: bin_data.extend(struct.pack('<H', val))
    return start, len(bin_data) - start

pos_off, pos_len = write_floats([c for pt in positions for c in pt])
norm_off, norm_len = write_floats([c for n in normals for c in n])
tex_off, tex_len = write_floats([c for uv in texcoords for c in uv])
joint_off, joint_len = write_uint16([j for j4 in joints for j in j4])
weight_off, weight_len = write_floats([w for w4 in weights for w in w4])
idx_off, idx_len = write_uint16(indices)
ibm_off, ibm_len = write_floats([v for m in inv_bind_matrices for v in m])

idle_t_off, idle_t_len = write_floats(idle_times)
run_t_off, run_t_len = write_floats(run_times)
atk_t_off, atk_t_len = write_floats(attack_times)

idle_torso_off, idle_torso_len = write_floats([c for q in idle_torso for c in q])
idle_rarm_off, idle_rarm_len = write_floats([c for q in idle_rarm for c in q])

run_lleg_off, run_lleg_len = write_floats([c for q in run_lleg for c in q])
run_rleg_off, run_rleg_len = write_floats([c for q in run_rleg for c in q])
run_rarm_off, run_rarm_len = write_floats([c for q in run_rarm for c in q])

atk_rarm_off, atk_rarm_len = write_floats([c for q in attack_rarm for c in q])
atk_torso_off, atk_torso_len = write_floats([c for q in attack_torso for c in q])

os.makedirs("assets/models", exist_ok=True)
bin_path = "assets/models/boss_gorgon.bin"
with open(bin_path, "wb") as f: f.write(bin_data)

buffer_views = [
    {"buffer": 0, "byteOffset": pos_off, "byteLength": pos_len, "target": 34962},
    {"buffer": 0, "byteOffset": norm_off, "byteLength": norm_len, "target": 34962},
    {"buffer": 0, "byteOffset": tex_off, "byteLength": tex_len, "target": 34962},
    {"buffer": 0, "byteOffset": joint_off, "byteLength": joint_len, "target": 34962},
    {"buffer": 0, "byteOffset": weight_off, "byteLength": weight_len, "target": 34962},
    {"buffer": 0, "byteOffset": idx_off, "byteLength": idx_len, "target": 34963},
    {"buffer": 0, "byteOffset": ibm_off, "byteLength": ibm_len},
    {"buffer": 0, "byteOffset": idle_t_off, "byteLength": idle_t_len},
    {"buffer": 0, "byteOffset": run_t_off, "byteLength": run_t_len},
    {"buffer": 0, "byteOffset": atk_t_off, "byteLength": atk_t_len},
    {"buffer": 0, "byteOffset": idle_torso_off, "byteLength": idle_torso_len},
    {"buffer": 0, "byteOffset": idle_rarm_off, "byteLength": idle_rarm_len},
    {"buffer": 0, "byteOffset": run_lleg_off, "byteLength": run_lleg_len},
    {"buffer": 0, "byteOffset": run_rleg_off, "byteLength": run_rleg_len},
    {"buffer": 0, "byteOffset": run_rarm_off, "byteLength": run_rarm_len},
    {"buffer": 0, "byteOffset": atk_rarm_off, "byteLength": atk_rarm_len},
    {"buffer": 0, "byteOffset": atk_torso_off, "byteLength": atk_torso_len},
]

min_x, max_x = min(p[0] for p in positions), max(p[0] for p in positions)
min_y, max_y = min(p[1] for p in positions), max(p[1] for p in positions)
min_z, max_z = min(p[2] for p in positions), max(p[2] for p in positions)

accessors = [
    {"bufferView": 0, "componentType": 5126, "count": len(positions), "type": "VEC3", "min": [min_x, min_y, min_z], "max": [max_x, max_y, max_z]},
    {"bufferView": 1, "componentType": 5126, "count": len(normals), "type": "VEC3"},
    {"bufferView": 2, "componentType": 5126, "count": len(texcoords), "type": "VEC2"},
    {"bufferView": 3, "componentType": 5123, "count": len(joints), "type": "VEC4"},
    {"bufferView": 4, "componentType": 5126, "count": len(weights), "type": "VEC4"},
    {"bufferView": 5, "componentType": 5123, "count": len(indices), "type": "SCALAR"},
    {"bufferView": 6, "componentType": 5126, "count": len(inv_bind_matrices), "type": "MAT4"},
    {"bufferView": 7, "componentType": 5126, "count": len(idle_times), "type": "SCALAR", "min": [0.0], "max": [idle_times[-1]]},
    {"bufferView": 8, "componentType": 5126, "count": len(run_times), "type": "SCALAR", "min": [0.0], "max": [run_times[-1]]},
    {"bufferView": 9, "componentType": 5126, "count": len(attack_times), "type": "SCALAR", "min": [0.0], "max": [attack_times[-1]]},
    {"bufferView": 10, "componentType": 5126, "count": len(idle_torso), "type": "VEC4"},
    {"bufferView": 11, "componentType": 5126, "count": len(idle_rarm), "type": "VEC4"},
    {"bufferView": 12, "componentType": 5126, "count": len(run_lleg), "type": "VEC4"},
    {"bufferView": 13, "componentType": 5126, "count": len(run_rleg), "type": "VEC4"},
    {"bufferView": 14, "componentType": 5126, "count": len(run_rarm), "type": "VEC4"},
    {"bufferView": 15, "componentType": 5126, "count": len(attack_rarm), "type": "VEC4"},
    {"bufferView": 16, "componentType": 5126, "count": len(attack_torso), "type": "VEC4"},
]

nodes = [
    {"name": "BossMesh", "mesh": 0, "skin": 0},
    {"name": "RootJoint", "children": [2, 6, 7], "translation": [0.0, 0.0, 0.0]},
    {"name": "TorsoJoint", "children": [3, 4, 5], "translation": [0.0, 0.95, 0.0]},
    {"name": "HeadJoint", "translation": [0.0, 0.90, 0.0]},
    {"name": "LeftArmJoint", "translation": [-0.50, 0.75, 0.0]},
    {"name": "RightArmJoint", "translation": [0.50, 0.75, 0.0]},
    {"name": "LeftLegJoint", "translation": [-0.24, 0.95, 0.0]},
    {"name": "RightLegJoint", "translation": [0.24, 0.95, 0.0]},
]

gltf = {
    "asset": {"version": "2.0", "generator": "Glory Boss Gorgon Generator"},
    "scene": 0,
    "scenes": [{"nodes": [0, 1]}],
    "nodes": nodes,
    "meshes": [{
        "name": "Boss Gorgon",
        "primitives": [{
            "attributes": {
                "POSITION": 0,
                "NORMAL": 1,
                "TEXCOORD_0": 2,
                "JOINTS_0": 3,
                "WEIGHTS_0": 4
            },
            "indices": 5,
            "material": 0
        }]
    }],
    "skins": [{
        "name": "Boss Skeleton",
        "inverseBindMatrices": 6,
        "skeleton": 1,
        "joints": [1, 2, 3, 4, 5, 6, 7]
    }],
    "animations": [
        {
            "name": "Idle",
            "samplers": [
                {"input": 7, "output": 10, "interpolation": "LINEAR"},
                {"input": 7, "output": 11, "interpolation": "LINEAR"},
            ],
            "channels": [
                {"sampler": 0, "target": {"node": 2, "path": "rotation"}},
                {"sampler": 1, "target": {"node": 5, "path": "rotation"}},
            ]
        },
        {
            "name": "Run",
            "samplers": [
                {"input": 8, "output": 12, "interpolation": "LINEAR"},
                {"input": 8, "output": 13, "interpolation": "LINEAR"},
                {"input": 8, "output": 14, "interpolation": "LINEAR"},
            ],
            "channels": [
                {"sampler": 0, "target": {"node": 6, "path": "rotation"}},
                {"sampler": 1, "target": {"node": 7, "path": "rotation"}},
                {"sampler": 2, "target": {"node": 5, "path": "rotation"}},
            ]
        },
        {
            "name": "Attack",
            "samplers": [
                {"input": 9, "output": 15, "interpolation": "LINEAR"},
                {"input": 9, "output": 16, "interpolation": "LINEAR"},
            ],
            "channels": [
                {"sampler": 0, "target": {"node": 5, "path": "rotation"}},
                {"sampler": 1, "target": {"node": 2, "path": "rotation"}},
            ],
            "extras": {
                "events": [
                    {"time": 0.55, "name": "hit_frame", "parameters": {"damage": "40"}},
                    {"time": 0.45, "name": "swing_sound"}
                ]
            }
        }
    ],
    "materials": [{
        "pbrMetallicRoughness": {
            "baseColorFactor": [1.0, 1.0, 1.0, 1.0],
            "metallicFactor": 0.8,
            "roughnessFactor": 0.4,
            "baseColorTexture": {"index": 0}
        }
    }],
    "textures": [{"source": 0}],
    "images": [{"uri": "../textures/boss_palette.png"}],
    "buffers": [{
        "uri": "boss_gorgon.bin",
        "byteLength": len(bin_data)
    }],
    "bufferViews": buffer_views,
    "accessors": accessors
}

gltf_path = "assets/models/boss_gorgon.gltf"
with open(gltf_path, "w", encoding="utf-8") as f:
    json.dump(gltf, f, indent=2)

print(f"Generated {gltf_path} ({len(bin_data)} bytes, {len(positions)} vertices)")
