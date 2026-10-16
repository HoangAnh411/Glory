import json

def build_world_map():
    props = []

    def add_prop(prop_type, pos, yaw=0.0, scale=1.0, collision=None, multi_collision=None):
        entry = {
            "type": prop_type,
            "position": [round(pos[0], 2), round(pos[1], 2), round(pos[2], 2)],
            "yaw": round(yaw, 1),
            "scale": round(scale, 2)
        }
        if collision:
            entry["collision"] = [round(c, 2) for c in collision]
        if multi_collision:
            entry["multi_collision"] = [[round(c, 2) for c in mc] for mc in multi_collision]
        props.append(entry)

    # -------------------------------------------------------------
    # 1. VILLAGE SQUARE (CENTER) - X: -10 to +10, Z: -12 to +14
    # -------------------------------------------------------------
    # Village Well at the heart of the square
    add_prop("village_well", [5.0, 0.0, 6.0], yaw=15.0, scale=1.0,
             collision=[-1.2, 0.0, -1.2, 1.2, 2.6, 1.2])

    # Grand Village Oak Tree with surrounding flowerbeds
    add_prop("tree_village_oak", [8.5, 0.0, -5.0], yaw=45.0, scale=1.1,
             collision=[-0.9, 0.0, -0.9, 0.9, 8.5, 0.9])
    add_prop("bush_flower_cluster", [10.2, 0.0, -4.0], yaw=30.0, scale=1.0)
    add_prop("bush_flower_cluster", [7.0, 0.0, -6.5], yaw=80.0, scale=0.9)
    add_prop("bench_wooden", [7.0, 0.0, -3.2], yaw=20.0, scale=1.0,
             collision=[-1.0, 0.0, -0.4, 1.0, 0.9, 0.4])

    # Directional Signpost at town square fork
    add_prop("signpost_wooden", [2.5, 0.0, 3.5], yaw=40.0, scale=1.0,
             collision=[-0.3, 0.0, -0.3, 0.3, 2.5, 0.3])

    # Street Lanterns around the village square
    lantern_positions = [
        (3.8, 0.0, -1.5, 0.0),
        (-3.8, 0.0, -1.5, 180.0),
        (3.8, 0.0, 10.5, 45.0),
        (-3.8, 0.0, 10.5, 135.0),
        (0.0, 0.0, -8.5, 90.0),
    ]
    for x, y, z, yaw in lantern_positions:
        add_prop("lamp_wooden_lantern", [x, y, z], yaw=yaw, scale=1.0,
                 collision=[-0.3, 0.0, -0.3, 0.3, 3.2, 0.3])

    # Flowers around the village well
    add_prop("bush_flower_cluster", [6.8, 0.0, 6.8], yaw=0.0, scale=0.85)
    add_prop("bush_flower_cluster", [3.6, 0.0, 5.5], yaw=110.0, scale=0.85)

    # -------------------------------------------------------------
    # 2. WEST DISTRICT - BLACKSMITH & TOWN INN / TAVERN
    # -------------------------------------------------------------
    # Village Blacksmith Forge
    add_prop("house_blacksmith", [-15.0, 0.0, 7.0], yaw=90.0, scale=1.0,
             collision=[-3.5, 0.0, -4.5, 3.5, 6.5, 4.5])
    add_prop("barrel_stack", [-11.0, 0.0, 3.5], yaw=15.0, scale=1.0,
             collision=[-1.1, 0.0, -1.1, 1.1, 1.5, 1.1])
    add_prop("cart_wooden_hay", [-11.5, 0.0, 11.5], yaw=-20.0, scale=1.0,
             collision=[-1.4, 0.0, -2.0, 1.4, 2.0, 2.0])
    add_prop("lamp_wooden_lantern", [-10.8, 0.0, 6.5], yaw=90.0, scale=1.0,
             collision=[-0.3, 0.0, -0.3, 0.3, 3.2, 0.3])

    # Grand Town Inn / Tavern (2-story with balcony)
    add_prop("house_town_inn", [-16.0, 0.0, -13.0], yaw=90.0, scale=1.0,
             collision=[-4.5, 0.0, -6.5, 4.5, 8.5, 6.5])
    add_prop("bench_wooden", [-10.8, 0.0, -9.5], yaw=90.0, scale=1.0,
             collision=[-0.4, 0.0, -1.0, 0.4, 0.9, 1.0])
    add_prop("barrel_stack", [-11.2, 0.0, -16.0], yaw=45.0, scale=1.0,
             collision=[-1.1, 0.0, -1.1, 1.1, 1.5, 1.1])
    add_prop("lamp_wooden_lantern", [-11.0, 0.0, -12.5], yaw=90.0, scale=1.0,
             collision=[-0.3, 0.0, -0.3, 0.3, 3.2, 0.3])

    # Behind tavern: Trees and split rail fences
    add_prop("tree_village_oak", [-24.0, 0.0, -14.0], yaw=120.0, scale=1.0,
             collision=[-0.8, 0.0, -0.8, 0.8, 7.5, 0.8])
    add_prop("tree_conifer_pine", [-25.0, 0.0, 8.0], yaw=60.0, scale=1.2,
             collision=[-0.6, 0.0, -0.6, 0.6, 8.0, 0.6])

    # -------------------------------------------------------------
    # 3. EAST DISTRICT - VILLAGE COTTAGES & PASTURES
    # -------------------------------------------------------------
    # Cottage 1 (North-East)
    add_prop("house_cottage", [16.0, 0.0, 7.0], yaw=-90.0, scale=1.0,
             collision=[-3.0, 0.0, -4.0, 3.0, 5.8, 4.0])
    add_prop("bush_flower_cluster", [12.2, 0.0, 5.0], yaw=15.0, scale=0.9)
    add_prop("bush_flower_cluster", [12.2, 0.0, 9.2], yaw=45.0, scale=0.9)
    add_prop("lamp_wooden_lantern", [11.5, 0.0, 7.0], yaw=-90.0, scale=1.0,
             collision=[-0.3, 0.0, -0.3, 0.3, 3.2, 0.3])

    # Cottage 2 (South-East)
    add_prop("house_cottage", [16.0, 0.0, -13.0], yaw=-90.0, scale=1.0,
             collision=[-3.0, 0.0, -4.0, 3.0, 5.8, 4.0])
    add_prop("tree_birch_gold", [18.5, 0.0, -3.5], yaw=15.0, scale=1.0,
             collision=[-0.5, 0.0, -0.5, 0.5, 6.5, 0.5])
    add_prop("bench_wooden", [11.5, 0.0, -11.0], yaw=-90.0, scale=1.0,
             collision=[-0.4, 0.0, -1.0, 0.4, 0.9, 1.0])
    add_prop("barrel_stack", [12.0, 0.0, -15.5], yaw=80.0, scale=1.0,
             collision=[-1.1, 0.0, -1.1, 1.1, 1.5, 1.1])

    # Pasture fences east of Cottage 1
    for z in [3.0, 7.0, 11.0, 15.0]:
        add_prop("fence_farm_wood", [22.0, 0.0, z], yaw=90.0, scale=1.0,
                 collision=[-0.3, 0.0, -2.0, 0.3, 1.2, 2.0])

    # -------------------------------------------------------------
    # 4. NORTH GATE & WINDMILL HILL - Z: +20 to +70
    # -------------------------------------------------------------
    # North Gate Tower (twin watchtowers spanning road with passage)
    add_prop("village_gate_tower", [0.0, 0.0, 24.0], yaw=0.0, scale=1.0,
             multi_collision=[
                 [-6.2, 0.0, -2.5, -2.2, 10.0, 2.5],
                 [2.2, 0.0, -2.5, 6.2, 10.0, 2.5]
             ])
    add_prop("lamp_wooden_lantern", [-2.6, 0.0, 21.5], yaw=0.0, scale=1.0,
             collision=[-0.3, 0.0, -0.3, 0.3, 3.2, 0.3])
    add_prop("lamp_wooden_lantern", [2.6, 0.0, 21.5], yaw=0.0, scale=1.0,
             collision=[-0.3, 0.0, -0.3, 0.3, 3.2, 0.3])

    # North road lanterns
    add_prop("lamp_wooden_lantern", [3.2, 0.0, 36.0], yaw=45.0, scale=1.0,
             collision=[-0.3, 0.0, -0.3, 0.3, 3.2, 0.3])
    add_prop("lamp_wooden_lantern", [-3.2, 0.0, 48.0], yaw=-45.0, scale=1.0,
             collision=[-0.3, 0.0, -0.3, 0.3, 3.2, 0.3])

    # Windmill on rolling green hill (North-East)
    add_prop("terrain_hill_green", [24.0, 0.0, 46.0], yaw=0.0, scale=1.8)
    add_prop("village_windmill", [24.0, 0.0, 46.0], yaw=-25.0, scale=1.0,
             collision=[-3.2, 0.0, -3.2, 3.2, 12.0, 3.2])
    add_prop("cart_wooden_hay", [17.0, 0.0, 40.0], yaw=35.0, scale=1.0,
             collision=[-1.4, 0.0, -2.0, 1.4, 2.0, 2.0])

    # Trees around windmill hill
    add_prop("tree_birch_gold", [16.0, 0.0, 35.0], yaw=30.0, scale=1.1,
             collision=[-0.5, 0.0, -0.5, 0.5, 6.5, 0.5])
    add_prop("tree_birch_gold", [31.0, 0.0, 42.0], yaw=75.0, scale=1.0,
             collision=[-0.5, 0.0, -0.5, 0.5, 6.5, 0.5])
    add_prop("tree_conifer_pine", [28.0, 0.0, 55.0], yaw=15.0, scale=1.3,
             collision=[-0.6, 0.0, -0.6, 0.6, 8.5, 0.6])
    add_prop("tree_conifer_pine", [15.0, 0.0, 54.0], yaw=110.0, scale=1.2,
             collision=[-0.6, 0.0, -0.6, 0.6, 8.0, 0.6])

    # Farm fences along the road
    for z in [30.0, 34.0, 38.0, 42.0]:
        add_prop("fence_farm_wood", [8.0, 0.0, z], yaw=90.0, scale=1.0,
                 collision=[-0.3, 0.0, -2.0, 0.3, 1.2, 2.0])

    # -------------------------------------------------------------
    # 5. SOUTH GATE & WILDERNESS TRAIL - Z: -20 to -70
    # -------------------------------------------------------------
    # South Gate Tower (twin watchtowers spanning road with passage)
    add_prop("village_gate_tower", [0.0, 0.0, -24.0], yaw=180.0, scale=1.0,
             multi_collision=[
                 [-6.2, 0.0, -2.5, -2.2, 10.0, 2.5],
                 [2.2, 0.0, -2.5, 6.2, 10.0, 2.5]
             ])
    add_prop("lamp_wooden_lantern", [-2.6, 0.0, -21.5], yaw=180.0, scale=1.0,
             collision=[-0.3, 0.0, -0.3, 0.3, 3.2, 0.3])
    add_prop("lamp_wooden_lantern", [2.6, 0.0, -21.5], yaw=180.0, scale=1.0,
             collision=[-0.3, 0.0, -0.3, 0.3, 3.2, 0.3])

    # Wilderness trees heading south into the forest
    forest_trees = [
        ("tree_conifer_pine", [-10.0, 0.0, -35.0], 1.2, [-0.6, 0.0, -0.6, 0.6, 8.0, 0.6]),
        ("tree_conifer_pine", [10.0, 0.0, -36.0], 1.3, [-0.6, 0.0, -0.6, 0.6, 8.5, 0.6]),
        ("tree_village_oak", [-14.0, 0.0, -48.0], 1.1, [-0.8, 0.0, -0.8, 0.8, 7.5, 0.8]),
        ("tree_birch_gold", [13.0, 0.0, -46.0], 1.0, [-0.5, 0.0, -0.5, 0.5, 6.5, 0.5]),
        ("tree_conifer_pine", [-8.0, 0.0, -58.0], 1.4, [-0.6, 0.0, -0.6, 0.6, 9.0, 0.6]),
        ("tree_village_oak", [9.0, 0.0, -60.0], 1.1, [-0.8, 0.0, -0.8, 0.8, 7.5, 0.8]),
    ]
    for p_type, pos, scale, col in forest_trees:
        add_prop(p_type, pos, yaw=35.0, scale=scale, collision=col)

    # -------------------------------------------------------------
    # 6. VALLEY PERIMETER: CLIFFS & GREEN HILLS ENCLOSING THE VALLEY
    # -------------------------------------------------------------
    # West Mountain Ridge
    west_cliffs = [
        ("terrain_cliff_rock", [-38.0, 0.0, 0.0], 25.0, 1.8, [-6.0, 0.0, -6.0, 6.0, 16.0, 6.0]),
        ("terrain_cliff_rock", [-40.0, 0.0, 26.0], -15.0, 2.0, [-7.0, 0.0, -7.0, 7.0, 18.0, 7.0]),
        ("terrain_cliff_rock", [-38.0, 0.0, -26.0], 10.0, 1.9, [-6.5, 0.0, -6.5, 6.5, 17.0, 6.5]),
        ("terrain_hill_green", [-32.0, 0.0, 14.0], 0.0, 1.6, None),
        ("terrain_hill_green", [-30.0, 0.0, -14.0], 0.0, 1.5, None),
    ]
    for p_type, pos, yaw, scale, col in west_cliffs:
        add_prop(p_type, pos, yaw=yaw, scale=scale, collision=col)

    # East Mountain Ridge
    east_cliffs = [
        ("terrain_cliff_rock", [38.0, 0.0, 0.0], -25.0, 1.8, [-6.0, 0.0, -6.0, 6.0, 16.0, 6.0]),
        ("terrain_cliff_rock", [40.0, 0.0, 26.0], 15.0, 2.0, [-7.0, 0.0, -7.0, 7.0, 18.0, 7.0]),
        ("terrain_cliff_rock", [38.0, 0.0, -26.0], -10.0, 1.9, [-6.5, 0.0, -6.5, 6.5, 17.0, 6.5]),
        ("terrain_hill_green", [32.0, 0.0, 14.0], 0.0, 1.6, None),
        ("terrain_hill_green", [30.0, 0.0, -14.0], 0.0, 1.5, None),
    ]
    for p_type, pos, yaw, scale, col in east_cliffs:
        add_prop(p_type, pos, yaw=yaw, scale=scale, collision=col)

    # North & South Horizons (Massive cliffs and hills sealing the valley)
    horizon_features = [
        ("terrain_hill_green", [0.0, 0.0, 72.0], 0.0, 2.2, None),
        ("terrain_cliff_rock", [-24.0, 0.0, 75.0], 45.0, 2.2, [-8.0, 0.0, -8.0, 8.0, 20.0, 8.0]),
        ("terrain_cliff_rock", [24.0, 0.0, 75.0], -45.0, 2.2, [-8.0, 0.0, -8.0, 8.0, 20.0, 8.0]),
        ("terrain_cliff_rock", [-20.0, 0.0, -75.0], 30.0, 2.1, [-7.5, 0.0, -7.5, 7.5, 19.0, 7.5]),
        ("terrain_cliff_rock", [20.0, 0.0, -75.0], -30.0, 2.1, [-7.5, 0.0, -7.5, 7.5, 19.0, 7.5]),
        ("terrain_hill_green", [0.0, 0.0, -72.0], 0.0, 2.0, None),
    ]
    for p_type, pos, yaw, scale, col in horizon_features:
        add_prop(p_type, pos, yaw=yaw, scale=scale, collision=col)

    world_map_data = {
        "worldName": "Thanh Tan Thu - Glory Novice Village",
        "zones": [
            {
                "id": "novice_village",
                "name": "Thành Tân Thủ (Novice Town Center - Safe Zone)",
                "minZ": -24.0,
                "maxZ": 24.0
            },
            {
                "id": "windmill_pastures",
                "name": "Đồi Cối Xay Gió (Windmill Hill & North Pastures)",
                "minZ": 24.0,
                "maxZ": 75.0
            },
            {
                "id": "wilds_frontier",
                "name": "Rừng Rậm Hoang Sơ (Wilderness South Trail - Danger Zone)",
                "minZ": -75.0,
                "maxZ": -24.0
            }
        ],
        "props": props
    }

    with open("assets/data/world_map.json", "w", encoding="utf-8") as f:
        json.dump(world_map_data, f, indent=2, ensure_ascii=False)

    print(f"Generated world_map.json with {len(props)} static props across 3 scenic zones!")

if __name__ == "__main__":
    build_world_map()
