import struct
import json
import math
import os

# Helper to create quaternion from axis-angle (degrees)
def quat_from_axis_angle(axis, degrees):
    rad = math.radians(degrees) * 0.5
    s = math.sin(rad)
    c = math.cos(rad)
    length = math.sqrt(axis[0]**2 + axis[1]**2 + axis[2]**2)
    if length < 1e-6:
        return [0.0, 0.0, 0.0, 1.0]
    return [axis[0]/length * s, axis[1]/length * s, axis[2]/length * s, c]

# Multiply two quaternions q1 * q2
def quat_multiply(q1, q2):
    x1, y1, z1, w1 = q1
    x2, y2, z2, w2 = q2
    return [
        w1*x2 + x1*w2 + y1*z2 - z1*y2,
        w1*y2 - x1*z2 + y1*w2 + z1*x2,
        w1*z2 + x1*y2 - y1*x2 + z1*w2,
        w1*w2 - x1*x2 - y1*y2 - z1*z2
    ]

# Vertex container
positions = []
normals = []
texcoords = []
joints = []
weights = []
indices = []

def add_box(min_pt, max_pt, joint_idx, u_range=(0.0, 1.0), v_range=(0.0, 1.0)):
    x0, y0, z0 = min_pt
    x1, y1, z1 = max_pt
    u0, u1 = u_range
    v0, v1 = v_range

    # 6 faces: +Z, -Z, +X, -X, +Y, -Y
    faces = [
        # +Z Front
        ([(x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)], (0, 0, 1)),
        # -Z Back
        ([(x1, y0, z0), (x0, y0, z0), (x0, y1, z0), (x1, y1, z0)], (0, 0, -1)),
        # +X Right
        ([(x1, y0, z1), (x1, y0, z0), (x1, y1, z0), (x1, y1, z1)], (1, 0, 0)),
        # -X Left
        ([(x0, y0, z0), (x0, y0, z1), (x0, y1, z1), (x0, y1, z0)], (-1, 0, 0)),
        # +Y Top
        ([(x0, y1, z1), (x1, y1, z1), (x1, y1, z0), (x0, y1, z0)], (0, 1, 0)),
        # -Y Bottom
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
        # Two triangles
        indices.extend([base_idx, base_idx + 1, base_idx + 2, base_idx, base_idx + 2, base_idx + 3])

# Palettes UV ranges for warrior_palette.png (128x128):
# Quad 0,0: Polished Steel Armor -> (0.05, 0.45), (0.05, 0.45)
# Quad 1,0: Royal Blue Cloth with Gold Weave -> (0.55, 0.95), (0.05, 0.45)
# Quad 0,1: Radiant Gold / Visor -> (0.05, 0.45), (0.55, 0.95)
# Quad 1,1: Razor Steel Blade -> (0.55, 0.95), (0.55, 0.95)

UV_STEEL = ((0.05, 0.45), (0.05, 0.45))
UV_ROYAL_BLUE = ((0.55, 0.95), (0.05, 0.45))
UV_GOLD = ((0.05, 0.45), (0.55, 0.95))
UV_BLADE = ((0.55, 0.95), (0.55, 0.95))

# Build Warrior parts (7 Joints: 0:Root, 1:Torso, 2:Head, 3:LeftArm, 4:RightArm, 5:LeftLeg, 6:RightLeg)
# Head & Visor (Joint 2)
add_box((-0.18, 1.45, -0.18), (0.18, 1.80, 0.18), 2, UV_STEEL[0], UV_STEEL[1])
add_box((-0.14, 1.54, 0.18), (0.14, 1.66, 0.21), 2, UV_GOLD[0], UV_GOLD[1]) # Golden visor

# Torso, Chestplate & Belt (Joint 1)
add_box((-0.26, 1.05, -0.18), (0.26, 1.45, 0.18), 1, UV_STEEL[0], UV_STEEL[1]) # Front/Back steel plate
add_box((-0.23, 0.75, -0.16), (0.23, 1.05, 0.16), 1, UV_ROYAL_BLUE[0], UV_ROYAL_BLUE[1]) # Royal blue tunic & belt

# Left Arm & Pauldron (Joint 3)
add_box((-0.44, 1.25, -0.16), (-0.26, 1.46, 0.16), 3, UV_STEEL[0], UV_STEEL[1]) # Pauldron
add_box((-0.38, 0.95, -0.12), (-0.26, 1.25, 0.12), 3, UV_ROYAL_BLUE[0], UV_ROYAL_BLUE[1]) # Upper arm sleeve
add_box((-0.38, 0.65, -0.12), (-0.26, 0.95, 0.12), 3, UV_STEEL[0], UV_STEEL[1]) # Gauntlet

# Right Arm & Pauldron (Joint 4)
add_box((0.26, 1.25, -0.16), (0.44, 1.46, 0.16), 4, UV_STEEL[0], UV_STEEL[1]) # Pauldron
add_box((0.26, 0.95, -0.12), (0.38, 1.25, 0.12), 4, UV_ROYAL_BLUE[0], UV_ROYAL_BLUE[1]) # Upper arm sleeve
add_box((0.26, 0.65, -0.12), (0.38, 0.95, 0.12), 4, UV_STEEL[0], UV_STEEL[1]) # Gauntlet

# Broadsword attached to Right Arm (Joint 4)
add_box((0.29, 0.60, 0.05), (0.35, 0.70, 0.22), 4, UV_ROYAL_BLUE[0], UV_ROYAL_BLUE[1]) # Leather Grip
add_box((0.14, 0.70, 0.10), (0.50, 0.76, 0.18), 4, UV_GOLD[0], UV_GOLD[1]) # Golden Crossguard
add_box((0.28, 0.76, 0.12), (0.36, 1.88, 0.16), 4, UV_BLADE[0], UV_BLADE[1]) # Razor Steel Blade!

# Left Leg & Boot (Joint 5)
add_box((-0.23, 0.38, -0.13), (-0.05, 0.75, 0.13), 5, UV_ROYAL_BLUE[0], UV_ROYAL_BLUE[1]) # Royal blue pants
add_box((-0.23, 0.00, -0.13), (-0.05, 0.38, 0.17), 5, UV_STEEL[0], UV_STEEL[1]) # Steel boot

# Right Leg & Boot (Joint 6)
add_box((0.05, 0.38, -0.13), (0.23, 0.75, 0.13), 6, UV_ROYAL_BLUE[0], UV_ROYAL_BLUE[1]) # Royal blue pants
add_box((0.05, 0.00, -0.13), (0.23, 0.38, 0.17), 6, UV_STEEL[0], UV_STEEL[1]) # Steel boot

# Inverse bind matrices for the 7 joints (Root=0, Torso=1, Head=2, LeftArm=3, RightArm=4, LeftLeg=5, RightLeg=6)
# Each matrix is 4x4 column-major: Translation(-world_pos)
def make_inv_bind(tx, ty, tz):
    return [
        1.0, 0.0, 0.0, 0.0,
        0.0, 1.0, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        -tx, -ty, -tz, 1.0
    ]

inv_bind_matrices = [
    make_inv_bind(0.0, 0.0, 0.0),       # 0: Root
    make_inv_bind(0.0, 0.75, 0.0),      # 1: Torso
    make_inv_bind(0.0, 1.45, 0.0),      # 2: Head
    make_inv_bind(-0.35, 1.35, 0.0),    # 3: LeftArm
    make_inv_bind(0.35, 1.35, 0.0),     # 4: RightArm
    make_inv_bind(-0.14, 0.75, 0.0),    # 5: LeftLeg
    make_inv_bind(0.14, 0.75, 0.0),     # 6: RightLeg
]

# Build animations
# Keyframe times
idle_times = [0.0, 0.75, 1.5]
run_times  = [0.0, 0.25, 0.5, 0.75, 1.0]
attack_times = [0.0, 0.15, 0.30, 0.50, 0.80]

# Idle rotations
idle_torso = [
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), -2.0),
    quat_from_axis_angle((1, 0, 0), 0.0)
]
idle_right_arm = [
    quat_from_axis_angle((1, 0, 0), -15.0),
    quat_from_axis_angle((1, 0, 0), -12.0),
    quat_from_axis_angle((1, 0, 0), -15.0)
]
idle_left_arm = [
    quat_from_axis_angle((1, 0, 0), 5.0),
    quat_from_axis_angle((1, 0, 0), 7.0),
    quat_from_axis_angle((1, 0, 0), 5.0)
]

# Run rotations
run_left_leg = [
    quat_from_axis_angle((1, 0, 0), 28.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), -28.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), 28.0)
]
run_right_leg = [
    quat_from_axis_angle((1, 0, 0), -28.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), 28.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), -28.0)
]
run_left_arm = [
    quat_from_axis_angle((1, 0, 0), -30.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), 30.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), -30.0)
]
run_right_arm = [
    quat_from_axis_angle((1, 0, 0), 25.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), -25.0),
    quat_from_axis_angle((1, 0, 0), 0.0),
    quat_from_axis_angle((1, 0, 0), 25.0)
]
run_torso = [
    quat_from_axis_angle((1, 0, 0), 5.0),
    quat_from_axis_angle((1, 0, 0), 7.0),
    quat_from_axis_angle((1, 0, 0), 5.0),
    quat_from_axis_angle((1, 0, 0), 7.0),
    quat_from_axis_angle((1, 0, 0), 5.0)
]

# Attack rotations: Right arm winds back, slashes across with tremendous force, then recovers
attack_right_arm = [
    quat_from_axis_angle((1, 0, 0), -15.0),
    quat_from_axis_angle((1, 0.3, 0), 50.0),   # Windup high
    quat_from_axis_angle((1, -0.4, 0), -65.0), # Heavy downward slash
    quat_from_axis_angle((1, -0.2, 0), -25.0), # Follow-through
    quat_from_axis_angle((1, 0, 0), -15.0)     # Return to guard
]
attack_torso = [
    quat_from_axis_angle((0, 1, 0), 0.0),
    quat_from_axis_angle((0, 1, 0), -25.0),  # Torso twists back
    quat_from_axis_angle((0, 1, 0), 35.0),   # Torso twists with slash
    quat_from_axis_angle((0, 1, 0), 10.0),
    quat_from_axis_angle((0, 1, 0), 0.0)
]

# Write Binary Buffer
bin_data = bytearray()

def write_floats(data):
    start = len(bin_data)
    for val in data:
        bin_data.extend(struct.pack('<f', val))
    return start, len(bin_data) - start

def write_uint16(data):
    start = len(bin_data)
    for val in data:
        bin_data.extend(struct.pack('<H', val))
    return start, len(bin_data) - start

# 1. Positions
flat_positions = [coord for pt in positions for coord in pt]
pos_offset, pos_len = write_floats(flat_positions)

# 2. Normals
flat_normals = [coord for n in normals for coord in n]
norm_offset, norm_len = write_floats(flat_normals)

# 3. Texcoords
flat_tex = [coord for uv in texcoords for coord in uv]
tex_offset, tex_len = write_floats(flat_tex)

# 4. Joints (uint16 vec4)
flat_joints = [j for j4 in joints for j in j4]
joint_offset, joint_len = write_uint16(flat_joints)

# 5. Weights (float vec4)
flat_weights = [w for w4 in weights for w in w4]
weight_offset, weight_len = write_floats(flat_weights)

# 6. Indices (uint16 scalar)
idx_offset, idx_len = write_uint16(indices)

# 7. Inverse Bind Matrices (16 floats per joint)
flat_ibm = [val for mat in inv_bind_matrices for val in mat]
ibm_offset, ibm_len = write_floats(flat_ibm)

# Animation buffers
# Times
idle_t_off, idle_t_len = write_floats(idle_times)
run_t_off, run_t_len = write_floats(run_times)
atk_t_off, atk_t_len = write_floats(attack_times)

# Idle Quats
idle_torso_off, idle_torso_len = write_floats([c for q in idle_torso for c in q])
idle_rarm_off, idle_rarm_len = write_floats([c for q in idle_right_arm for c in q])
idle_larm_off, idle_larm_len = write_floats([c for q in idle_left_arm for c in q])

# Run Quats
run_lleg_off, run_lleg_len = write_floats([c for q in run_left_leg for c in q])
run_rleg_off, run_rleg_len = write_floats([c for q in run_right_leg for c in q])
run_larm_off, run_larm_len = write_floats([c for q in run_left_arm for c in q])
run_rarm_off, run_rarm_len = write_floats([c for q in run_right_arm for c in q])
run_torso_off, run_torso_len = write_floats([c for q in run_torso for c in q])

# Attack Quats
atk_rarm_off, atk_rarm_len = write_floats([c for q in attack_right_arm for c in q])
atk_torso_off, atk_torso_len = write_floats([c for q in attack_torso for c in q])

# Save bin file
os.makedirs("assets/models", exist_ok=True)
bin_path = "assets/models/hero_warrior.bin"
with open(bin_path, "wb") as f:
    f.write(bin_data)

print(f"Generated {bin_path} ({len(bin_data)} bytes, {len(positions)} vertices, {len(indices)//3} triangles)")

# Build glTF JSON
buffer_views = [
    # 0: Positions
    {"buffer": 0, "byteOffset": pos_offset, "byteLength": pos_len, "target": 34962},
    # 1: Normals
    {"buffer": 0, "byteOffset": norm_offset, "byteLength": norm_len, "target": 34962},
    # 2: Texcoords
    {"buffer": 0, "byteOffset": tex_offset, "byteLength": tex_len, "target": 34962},
    # 3: Joints
    {"buffer": 0, "byteOffset": joint_offset, "byteLength": joint_len, "target": 34962},
    # 4: Weights
    {"buffer": 0, "byteOffset": weight_offset, "byteLength": weight_len, "target": 34962},
    # 5: Indices
    {"buffer": 0, "byteOffset": idx_offset, "byteLength": idx_len, "target": 34963},
    # 6: Inverse Bind Matrices
    {"buffer": 0, "byteOffset": ibm_offset, "byteLength": ibm_len},
    # 7: Idle Times
    {"buffer": 0, "byteOffset": idle_t_off, "byteLength": idle_t_len},
    # 8: Run Times
    {"buffer": 0, "byteOffset": run_t_off, "byteLength": run_t_len},
    # 9: Attack Times
    {"buffer": 0, "byteOffset": atk_t_off, "byteLength": atk_t_len},
    # 10: Idle Torso
    {"buffer": 0, "byteOffset": idle_torso_off, "byteLength": idle_torso_len},
    # 11: Idle Right Arm
    {"buffer": 0, "byteOffset": idle_rarm_off, "byteLength": idle_rarm_len},
    # 12: Idle Left Arm
    {"buffer": 0, "byteOffset": idle_larm_off, "byteLength": idle_larm_len},
    # 13: Run Left Leg
    {"buffer": 0, "byteOffset": run_lleg_off, "byteLength": run_lleg_len},
    # 14: Run Right Leg
    {"buffer": 0, "byteOffset": run_rleg_off, "byteLength": run_rleg_len},
    # 15: Run Left Arm
    {"buffer": 0, "byteOffset": run_larm_off, "byteLength": run_larm_len},
    # 16: Run Right Arm
    {"buffer": 0, "byteOffset": run_rarm_off, "byteLength": run_rarm_len},
    # 17: Run Torso
    {"buffer": 0, "byteOffset": run_torso_off, "byteLength": run_torso_len},
    # 18: Attack Right Arm
    {"buffer": 0, "byteOffset": atk_rarm_off, "byteLength": atk_rarm_len},
    # 19: Attack Torso
    {"buffer": 0, "byteOffset": atk_torso_off, "byteLength": atk_torso_len},
]

# Calculate position bounding box
min_x = min(p[0] for p in positions)
max_x = max(p[0] for p in positions)
min_y = min(p[1] for p in positions)
max_y = max(p[1] for p in positions)
min_z = min(p[2] for p in positions)
max_z = max(p[2] for p in positions)

accessors = [
    # 0: Positions
    {"bufferView": 0, "componentType": 5126, "count": len(positions), "type": "VEC3",
     "min": [min_x, min_y, min_z], "max": [max_x, max_y, max_z]},
    # 1: Normals
    {"bufferView": 1, "componentType": 5126, "count": len(normals), "type": "VEC3"},
    # 2: Texcoords
    {"bufferView": 2, "componentType": 5126, "count": len(texcoords), "type": "VEC2"},
    # 3: Joints
    {"bufferView": 3, "componentType": 5123, "count": len(joints), "type": "VEC4"},
    # 4: Weights
    {"bufferView": 4, "componentType": 5126, "count": len(weights), "type": "VEC4"},
    # 5: Indices
    {"bufferView": 5, "componentType": 5123, "count": len(indices), "type": "SCALAR"},
    # 6: Inverse Bind Matrices (7 joints)
    {"bufferView": 6, "componentType": 5126, "count": len(inv_bind_matrices), "type": "MAT4"},
    # 7: Idle Times
    {"bufferView": 7, "componentType": 5126, "count": len(idle_times), "type": "SCALAR", "min": [0.0], "max": [idle_times[-1]]},
    # 8: Run Times
    {"bufferView": 8, "componentType": 5126, "count": len(run_times), "type": "SCALAR", "min": [0.0], "max": [run_times[-1]]},
    # 9: Attack Times
    {"bufferView": 9, "componentType": 5126, "count": len(attack_times), "type": "SCALAR", "min": [0.0], "max": [attack_times[-1]]},
    # 10: Idle Torso
    {"bufferView": 10, "componentType": 5126, "count": len(idle_torso), "type": "VEC4"},
    # 11: Idle Right Arm
    {"bufferView": 11, "componentType": 5126, "count": len(idle_right_arm), "type": "VEC4"},
    # 12: Idle Left Arm
    {"bufferView": 12, "componentType": 5126, "count": len(idle_left_arm), "type": "VEC4"},
    # 13: Run Left Leg
    {"bufferView": 13, "componentType": 5126, "count": len(run_left_leg), "type": "VEC4"},
    # 14: Run Right Leg
    {"bufferView": 14, "componentType": 5126, "count": len(run_right_leg), "type": "VEC4"},
    # 15: Run Left Arm
    {"bufferView": 15, "componentType": 5126, "count": len(run_left_arm), "type": "VEC4"},
    # 16: Run Right Arm
    {"bufferView": 16, "componentType": 5126, "count": len(run_right_arm), "type": "VEC4"},
    # 17: Run Torso
    {"bufferView": 17, "componentType": 5126, "count": len(run_torso), "type": "VEC4"},
    # 18: Attack Right Arm
    {"bufferView": 18, "componentType": 5126, "count": len(attack_right_arm), "type": "VEC4"},
    # 19: Attack Torso
    {"bufferView": 19, "componentType": 5126, "count": len(attack_torso), "type": "VEC4"},
]

# Nodes:
# Node 0: WarriorMesh (mesh 0, skin 0)
# Node 1: RootJoint (children: [2, 6, 7], translation: [0, 0, 0])
# Node 2: TorsoJoint (children: [3, 4, 5], translation: [0, 0.75, 0])
# Node 3: HeadJoint (translation: [0, 0.70, 0])
# Node 4: LeftArmJoint (translation: [-0.35, 0.60, 0])
# Node 5: RightArmJoint (translation: [0.35, 0.60, 0])
# Node 6: LeftLegJoint (translation: [-0.14, 0.75, 0])
# Node 7: RightLegJoint (translation: [0.14, 0.75, 0])

nodes = [
    {"name": "WarriorMesh", "mesh": 0, "skin": 0},
    {"name": "RootJoint", "children": [2, 6, 7], "translation": [0.0, 0.0, 0.0]},
    {"name": "TorsoJoint", "children": [3, 4, 5], "translation": [0.0, 0.75, 0.0]},
    {"name": "HeadJoint", "translation": [0.0, 0.70, 0.0]},
    {"name": "LeftArmJoint", "translation": [-0.35, 0.60, 0.0]},
    {"name": "RightArmJoint", "translation": [0.35, 0.60, 0.0]},
    {"name": "LeftLegJoint", "translation": [-0.14, 0.75, 0.0]},
    {"name": "RightLegJoint", "translation": [0.14, 0.75, 0.0]},
]

gltf = {
    "asset": {"version": "2.0", "generator": "Glory Humanoid Warrior Generator"},
    "scene": 0,
    "scenes": [{"nodes": [0, 1]}],
    "nodes": nodes,
    "meshes": [{
        "name": "Humanoid Warrior",
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
        "name": "Warrior Skeleton",
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
                {"input": 7, "output": 12, "interpolation": "LINEAR"},
            ],
            "channels": [
                {"sampler": 0, "target": {"node": 2, "path": "rotation"}},
                {"sampler": 1, "target": {"node": 5, "path": "rotation"}},
                {"sampler": 2, "target": {"node": 4, "path": "rotation"}},
            ]
        },
        {
            "name": "Run",
            "samplers": [
                {"input": 8, "output": 13, "interpolation": "LINEAR"},
                {"input": 8, "output": 14, "interpolation": "LINEAR"},
                {"input": 8, "output": 15, "interpolation": "LINEAR"},
                {"input": 8, "output": 16, "interpolation": "LINEAR"},
                {"input": 8, "output": 17, "interpolation": "LINEAR"},
            ],
            "channels": [
                {"sampler": 0, "target": {"node": 6, "path": "rotation"}},
                {"sampler": 1, "target": {"node": 7, "path": "rotation"}},
                {"sampler": 2, "target": {"node": 4, "path": "rotation"}},
                {"sampler": 3, "target": {"node": 5, "path": "rotation"}},
                {"sampler": 4, "target": {"node": 2, "path": "rotation"}},
            ]
        },
        {
            "name": "Attack",
            "samplers": [
                {"input": 9, "output": 18, "interpolation": "LINEAR"},
                {"input": 9, "output": 19, "interpolation": "LINEAR"},
            ],
            "channels": [
                {"sampler": 0, "target": {"node": 5, "path": "rotation"}},
                {"sampler": 1, "target": {"node": 2, "path": "rotation"}},
            ],
            "extras": {
                "events": [
                    {"time": 0.25, "name": "hit_frame", "parameters": {"damage": "25"}},
                    {"time": 0.15, "name": "swing_sound"}
                ]
            }
        }
    ],
    "materials": [{
        "pbrMetallicRoughness": {
            "baseColorFactor": [1.0, 1.0, 1.0, 1.0],
            "metallicFactor": 0.55,
            "roughnessFactor": 0.35,
            "baseColorTexture": {"index": 0}
        }
    }],
    "textures": [{"source": 0}],
    "images": [{"uri": "../textures/warrior_palette.png"}],
    "buffers": [{
        "uri": "hero_warrior.bin",
        "byteLength": len(bin_data)
    }],
    "bufferViews": buffer_views,
    "accessors": accessors
}

gltf_path = "assets/models/hero_warrior.gltf"
with open(gltf_path, "w", encoding="utf-8") as f:
    json.dump(gltf, f, indent=2)

print(f"Generated {gltf_path} successfully!")
