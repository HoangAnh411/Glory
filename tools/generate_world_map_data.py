import json

props = [
    # =========================================================================
    # ZONE 1: TRONG THÀNH (SANCTUARY CITADEL - SAFE CITY) [Z: +18 to +75]
    # =========================================================================
    # Central Town Square Fountain
    {"type": "stone_fountain", "position": [0.0, 0.0, 42.0], "yaw": 0.0, "scale": 1.0, "collision": [-3.0, 0.0, -3.0, 3.0, 2.8, 3.0]},

    # Western Street Houses (Facing East)
    {"type": "house_medieval", "position": [-14.0, 0.0, 30.0], "yaw": 90.0, "scale": 1.0, "collision": [-3.2, 0.0, -4.2, 3.2, 7.5, 4.2]},
    {"type": "house_medieval", "position": [-14.0, 0.0, 44.0], "yaw": 90.0, "scale": 1.0, "collision": [-3.2, 0.0, -4.2, 3.2, 7.5, 4.2]},
    {"type": "house_medieval", "position": [-14.0, 0.0, 58.0], "yaw": 90.0, "scale": 1.0, "collision": [-3.2, 0.0, -4.2, 3.2, 7.5, 4.2]},

    # Eastern Street Houses (Facing West)
    {"type": "house_medieval", "position": [14.0, 0.0, 30.0], "yaw": -90.0, "scale": 1.0, "collision": [-3.2, 0.0, -4.2, 3.2, 7.5, 4.2]},
    {"type": "house_medieval", "position": [14.0, 0.0, 44.0], "yaw": -90.0, "scale": 1.0, "collision": [-3.2, 0.0, -4.2, 3.2, 7.5, 4.2]},
    {"type": "house_medieval", "position": [14.0, 0.0, 58.0], "yaw": -90.0, "scale": 1.0, "collision": [-3.2, 0.0, -4.2, 3.2, 7.5, 4.2]},

    # Southern Back Houses (Facing North)
    {"type": "house_medieval", "position": [-4.5, 0.0, 68.0], "yaw": 180.0, "scale": 1.0, "collision": [-4.2, 0.0, -3.2, 4.2, 7.5, 3.2]},
    {"type": "house_medieval", "position": [4.5, 0.0, 68.0], "yaw": 180.0, "scale": 1.0, "collision": [-4.2, 0.0, -3.2, 4.2, 7.5, 3.2]},

    # Supply Crates in Town
    {"type": "crate_wooden", "position": [-9.5, 0.0, 32.0], "yaw": 15.0, "scale": 1.0, "collision": [-0.5, 0.0, -0.5, 0.5, 1.0, 0.5]},
    {"type": "crate_wooden", "position": [-9.0, 0.0, 33.2], "yaw": -20.0, "scale": 1.0, "collision": [-0.5, 0.0, -0.5, 0.5, 1.0, 0.5]},
    {"type": "crate_wooden", "position": [9.5, 0.0, 46.0], "yaw": 30.0, "scale": 1.2, "collision": [-0.6, 0.0, -0.6, 0.6, 1.2, 0.6]},

    # Town Lantern Posts
    {"type": "lantern_post", "position": [-5.5, 0.0, 36.0], "yaw": 90.0, "scale": 1.0, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},
    {"type": "lantern_post", "position": [5.5, 0.0, 36.0], "yaw": -90.0, "scale": 1.0, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},
    {"type": "lantern_post", "position": [-5.5, 0.0, 48.0], "yaw": 90.0, "scale": 1.0, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},
    {"type": "lantern_post", "position": [5.5, 0.0, 48.0], "yaw": -90.0, "scale": 1.0, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},
    {"type": "lantern_post", "position": [-5.0, 0.0, 24.0], "yaw": 90.0, "scale": 1.0, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},
    {"type": "lantern_post", "position": [5.0, 0.0, 24.0], "yaw": -90.0, "scale": 1.0, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},

    # Town Trees in Courtyards
    {"type": "tree_oak", "position": [-8.0, 0.0, 54.0], "yaw": 40.0, "scale": 1.1, "collision": [-0.5, 0.0, -0.5, 0.5, 4.0, 0.5]},
    {"type": "tree_oak", "position": [8.0, 0.0, 54.0], "yaw": -30.0, "scale": 1.1, "collision": [-0.5, 0.0, -0.5, 0.5, 4.0, 0.5]},

    # Fortified City Wall & Gate at Z: +16.0
    # Central Arch Gate: opening x in [-3, 3], bastions at [-12..-3] and [3..12]
    {"type": "city_wall_gate", "position": [0.0, 0.0, 16.0], "yaw": 0.0, "scale": 1.0,
     "multi_collision": [
         [-12.0, 0.0, -1.5, -3.0, 7.5, 1.5],  # Left bastion
         [3.0, 0.0, -1.5, 12.0, 7.5, 1.5]     # Right bastion
     ]},
    # Left Wall Extension
    {"type": "city_wall_gate", "position": [-22.0, 0.0, 16.0], "yaw": 0.0, "scale": 1.0,
     "collision": [-12.0, 0.0, -1.5, 12.0, 7.5, 1.5]},
    # Right Wall Extension
    {"type": "city_wall_gate", "position": [22.0, 0.0, 16.0], "yaw": 0.0, "scale": 1.0,
     "collision": [-12.0, 0.0, -1.5, 12.0, 7.5, 1.5]},

    # Gate Entry Lanterns
    {"type": "lantern_post", "position": [-3.8, 0.0, 18.2], "yaw": 45.0, "scale": 1.1, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},
    {"type": "lantern_post", "position": [3.8, 0.0, 18.2], "yaw": -45.0, "scale": 1.1, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},
    {"type": "lantern_post", "position": [-3.8, 0.0, 13.8], "yaw": 135.0, "scale": 1.1, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},
    {"type": "lantern_post", "position": [3.8, 0.0, 13.8], "yaw": -135.0, "scale": 1.1, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},

    # =========================================================================
    # ZONE 2: NGOÀI THÀNH / DÃ NGOẠI (OUTER FOREST FRONTIER) [Z: -25 to +14]
    # =========================================================================
    # Winding Trail Wooden Fences (Flanking road from city gate towards wilderness)
    {"type": "fence_wood", "position": [-4.5, 0.0, 10.0], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},
    {"type": "fence_wood", "position": [-4.5, 0.0, 5.5], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},
    {"type": "fence_wood", "position": [-4.5, 0.0, 1.0], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},
    {"type": "fence_wood", "position": [-4.5, 0.0, -3.5], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},
    {"type": "fence_wood", "position": [-4.5, 0.0, -8.0], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},
    {"type": "fence_wood", "position": [-4.5, 0.0, -12.5], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},
    {"type": "fence_wood", "position": [-4.5, 0.0, -17.0], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},

    {"type": "fence_wood", "position": [4.5, 0.0, 10.0], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},
    {"type": "fence_wood", "position": [4.5, 0.0, 5.5], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},
    {"type": "fence_wood", "position": [4.5, 0.0, 1.0], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},
    {"type": "fence_wood", "position": [4.5, 0.0, -3.5], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},
    {"type": "fence_wood", "position": [4.5, 0.0, -8.0], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},
    {"type": "fence_wood", "position": [4.5, 0.0, -12.5], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},
    {"type": "fence_wood", "position": [4.5, 0.0, -17.0], "yaw": 0.0, "scale": 1.0, "collision": [-1.8, 0.0, -0.15, 1.8, 1.3, 0.15]},

    # Trail Lanterns
    {"type": "lantern_post", "position": [-5.2, 0.0, 7.5], "yaw": 90.0, "scale": 1.0, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},
    {"type": "lantern_post", "position": [5.2, 0.0, -1.5], "yaw": -90.0, "scale": 1.0, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},
    {"type": "lantern_post", "position": [-5.2, 0.0, -10.0], "yaw": 90.0, "scale": 1.0, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},
    {"type": "lantern_post", "position": [5.2, 0.0, -19.0], "yaw": -90.0, "scale": 1.0, "collision": [-0.3, 0.0, -0.3, 0.3, 3.0, 0.3]},

    # Oak Trees in Wilderness (Western Woods)
    {"type": "tree_oak", "position": [-11.0, 0.0, 8.0], "yaw": 12.0, "scale": 1.2, "collision": [-0.5, 0.0, -0.5, 0.5, 4.0, 0.5]},
    {"type": "tree_oak", "position": [-16.0, 0.0, 2.0], "yaw": 45.0, "scale": 1.4, "collision": [-0.6, 0.0, -0.6, 0.6, 4.5, 0.6]},
    {"type": "tree_oak", "position": [-12.5, 0.0, -7.0], "yaw": 90.0, "scale": 1.1, "collision": [-0.5, 0.0, -0.5, 0.5, 4.0, 0.5]},
    {"type": "tree_oak", "position": [-18.0, 0.0, -16.0], "yaw": 25.0, "scale": 1.5, "collision": [-0.7, 0.0, -0.7, 0.7, 5.0, 0.7]},
    {"type": "tree_oak", "position": [-25.0, 0.0, -5.0], "yaw": 60.0, "scale": 1.3, "collision": [-0.6, 0.0, -0.6, 0.6, 4.0, 0.6]},

    # Oak Trees in Wilderness (Eastern Woods)
    {"type": "tree_oak", "position": [11.0, 0.0, 8.0], "yaw": -20.0, "scale": 1.2, "collision": [-0.5, 0.0, -0.5, 0.5, 4.0, 0.5]},
    {"type": "tree_oak", "position": [17.0, 0.0, 1.0], "yaw": -50.0, "scale": 1.3, "collision": [-0.6, 0.0, -0.6, 0.6, 4.0, 0.6]},
    {"type": "tree_oak", "position": [13.0, 0.0, -8.0], "yaw": -80.0, "scale": 1.1, "collision": [-0.5, 0.0, -0.5, 0.5, 4.0, 0.5]},
    {"type": "tree_oak", "position": [19.0, 0.0, -16.0], "yaw": -15.0, "scale": 1.4, "collision": [-0.6, 0.0, -0.6, 0.6, 4.5, 0.6]},
    {"type": "tree_oak", "position": [26.0, 0.0, -6.0], "yaw": -45.0, "scale": 1.2, "collision": [-0.5, 0.0, -0.5, 0.5, 4.0, 0.5]},

    # Coniferous Pine Trees
    {"type": "tree_pine", "position": [-20.0, 0.0, 10.0], "yaw": 33.0, "scale": 1.3, "collision": [-0.4, 0.0, -0.4, 0.4, 6.0, 0.4]},
    {"type": "tree_pine", "position": [-28.0, 0.0, 0.0], "yaw": 75.0, "scale": 1.5, "collision": [-0.4, 0.0, -0.4, 0.4, 6.5, 0.4]},
    {"type": "tree_pine", "position": [21.0, 0.0, 9.0], "yaw": -30.0, "scale": 1.3, "collision": [-0.4, 0.0, -0.4, 0.4, 6.0, 0.4]},
    {"type": "tree_pine", "position": [27.0, 0.0, 1.0], "yaw": -70.0, "scale": 1.4, "collision": [-0.4, 0.0, -0.4, 0.4, 6.5, 0.4]},

    # Natural Rock Boulders
    {"type": "rock_boulder", "position": [-8.0, 0.0, 3.0], "yaw": 40.0, "scale": 1.0, "collision": [-1.2, 0.0, -0.9, 1.2, 1.4, 0.9]},
    {"type": "rock_boulder", "position": [8.5, 0.0, -4.0], "yaw": -60.0, "scale": 1.2, "collision": [-1.4, 0.0, -1.1, 1.4, 1.6, 1.1]},
    {"type": "rock_boulder", "position": [-10.0, 0.0, -14.0], "yaw": 110.0, "scale": 1.1, "collision": [-1.3, 0.0, -1.0, 1.3, 1.5, 1.0]},
    {"type": "rock_boulder", "position": [9.0, 0.0, -15.0], "yaw": -25.0, "scale": 1.3, "collision": [-1.5, 0.0, -1.2, 1.5, 1.7, 1.2]},

    # =========================================================================
    # ZONE 3: KHU VỰC NGUY HIỂM / PHẾ TÍCH CỔ (RUINS OF DESOLATION) [Z: -25 to -75]
    # =========================================================================
    # Danger Warning Entrance Braziers (Marking entrance to ruins)
    {"type": "brazier_fire", "position": [-5.5, 0.0, -25.0], "yaw": 0.0, "scale": 1.2, "collision": [-0.8, 0.0, -0.8, 0.8, 2.0, 0.8]},
    {"type": "brazier_fire", "position": [5.5, 0.0, -25.0], "yaw": 0.0, "scale": 1.2, "collision": [-0.8, 0.0, -0.8, 0.8, 2.0, 0.8]},

    # Ring of Weathered Ruin Pillars (Colosseum ruins arena)
    {"type": "ruin_pillar", "position": [-10.0, 0.0, -35.0], "yaw": 20.0, "scale": 1.3, "collision": [-0.8, 0.0, -0.8, 0.8, 5.0, 0.8]},
    {"type": "ruin_pillar", "position": [10.0, 0.0, -35.0], "yaw": -35.0, "scale": 1.3, "collision": [-0.8, 0.0, -0.8, 0.8, 5.0, 0.8]},
    {"type": "ruin_pillar", "position": [-16.0, 0.0, -48.0], "yaw": 50.0, "scale": 1.4, "collision": [-0.9, 0.0, -0.9, 0.9, 5.5, 0.9]},
    {"type": "ruin_pillar", "position": [16.0, 0.0, -48.0], "yaw": -65.0, "scale": 1.4, "collision": [-0.9, 0.0, -0.9, 0.9, 5.5, 0.9]},
    {"type": "ruin_pillar", "position": [-11.0, 0.0, -62.0], "yaw": 80.0, "scale": 1.3, "collision": [-0.8, 0.0, -0.8, 0.8, 5.0, 0.8]},
    {"type": "ruin_pillar", "position": [11.0, 0.0, -62.0], "yaw": -95.0, "scale": 1.3, "collision": [-0.8, 0.0, -0.8, 0.8, 5.0, 0.8]},
    {"type": "ruin_pillar", "position": [0.0, 0.0, -68.0], "yaw": 180.0, "scale": 1.5, "collision": [-1.0, 0.0, -1.0, 1.0, 6.0, 1.0]},

    # Burning Braziers inside the Boss Arena
    {"type": "brazier_fire", "position": [-13.0, 0.0, -48.0], "yaw": 0.0, "scale": 1.3, "collision": [-0.8, 0.0, -0.8, 0.8, 2.0, 0.8]},
    {"type": "brazier_fire", "position": [13.0, 0.0, -48.0], "yaw": 0.0, "scale": 1.3, "collision": [-0.8, 0.0, -0.8, 0.8, 2.0, 0.8]},
    {"type": "brazier_fire", "position": [0.0, 0.0, -64.0], "yaw": 0.0, "scale": 1.4, "collision": [-0.9, 0.0, -0.9, 0.9, 2.2, 0.9]},

    # Ancient Rock Formations in Danger Zone
    {"type": "rock_boulder", "position": [-19.0, 0.0, -40.0], "yaw": 15.0, "scale": 1.6, "collision": [-1.8, 0.0, -1.4, 1.8, 2.0, 1.4]},
    {"type": "rock_boulder", "position": [19.0, 0.0, -40.0], "yaw": -45.0, "scale": 1.6, "collision": [-1.8, 0.0, -1.4, 1.8, 2.0, 1.4]},
    {"type": "rock_boulder", "position": [-18.0, 0.0, -60.0], "yaw": 60.0, "scale": 1.7, "collision": [-1.9, 0.0, -1.5, 1.9, 2.2, 1.5]},
    {"type": "rock_boulder", "position": [18.0, 0.0, -60.0], "yaw": -75.0, "scale": 1.7, "collision": [-1.9, 0.0, -1.5, 1.9, 2.2, 1.5]},

    # Dead Pine Trees near danger boundary
    {"type": "tree_pine", "position": [-18.0, 0.0, -28.0], "yaw": 15.0, "scale": 1.2, "collision": [-0.4, 0.0, -0.4, 0.4, 5.0, 0.4]},
    {"type": "tree_pine", "position": [18.0, 0.0, -28.0], "yaw": -25.0, "scale": 1.2, "collision": [-0.4, 0.0, -0.4, 0.4, 5.0, 0.4]}
]

world_data = {
    "worldName": "Glory Sanctuary & Frontier",
    "zones": [
        {"id": "citadel", "name": "Thành Trấn Khởi Nguyên (Sanctuary Citadel - Safe Zone)", "minZ": 16.0, "maxZ": 90.0},
        {"id": "frontier", "name": "Vùng Dã Ngoại (Outer Wilds - Forest Trail)", "minZ": -25.0, "maxZ": 16.0},
        {"id": "danger", "name": "Phế Tích Cấm Địa (Ruins of Desolation - Danger Zone)", "minZ": -90.0, "maxZ": -25.0}
    ],
    "props": props
}

with open("assets/data/world_map.json", "w", encoding="utf-8") as f:
    json.dump(world_data, f, indent=2, ensure_ascii=False)

print(f"Generated assets/data/world_map.json with {len(props)} static world objects!")
