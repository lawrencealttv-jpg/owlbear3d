"""Generate Owlbear 3D's original flat-shaded starter miniatures.

Run from Blender's Scripting workspace or:
    blender --background --python generate_starter_pack.py

FBX files are written to Tools/Blender/GeneratedFBX.
"""

import math
import os
import bpy
from mathutils import Vector


OUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "GeneratedFBX")
os.makedirs(OUT_DIR, exist_ok=True)

PALETTE = {
    "skin": (0.63, 0.42, 0.27, 1),
    "green": (0.22, 0.48, 0.13, 1),
    "dark_green": (0.10, 0.25, 0.07, 1),
    "blue": (0.08, 0.25, 0.52, 1),
    "red": (0.48, 0.06, 0.045, 1),
    "purple": (0.26, 0.08, 0.38, 1),
    "leather": (0.24, 0.085, 0.025, 1),
    "bone": (0.72, 0.68, 0.52, 1),
    "steel": (0.28, 0.32, 0.36, 1),
    "dark_steel": (0.08, 0.10, 0.12, 1),
    "black": (0.015, 0.018, 0.022, 1),
    "white": (0.80, 0.82, 0.78, 1),
    "yellow": (0.72, 0.52, 0.05, 1),
    "slime": (0.18, 0.62, 0.37, 1),
}

MATERIALS = {}


def reset_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    MATERIALS.clear()


def material(name):
    if name not in MATERIALS:
        mat = bpy.data.materials.new(f"M_{name.title()}")
        mat.diffuse_color = PALETTE[name]
        mat.use_nodes = False
        mat.roughness = 0.9
        MATERIALS[name] = mat
    return MATERIALS[name]


def finish(obj, mat_name):
    obj.data.materials.append(material(mat_name))
    for poly in obj.data.polygons:
        poly.use_smooth = False
    return obj


def cube(name, location, scale, mat_name, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish(obj, mat_name)


def sphere(name, location, scale, mat_name, subdivisions=1):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=subdivisions, radius=1, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return finish(obj, mat_name)


def cylinder(name, location, radius, depth, mat_name, vertices=8, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cylinder_add(vertices=vertices, radius=radius, depth=depth, location=location, rotation=rotation)
    return finish(bpy.context.object, mat_name)


def cone(name, location, radius1, radius2, depth, mat_name, vertices=8, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cone_add(vertices=vertices, radius1=radius1, radius2=radius2, depth=depth,
                                   location=location, rotation=rotation)
    return finish(bpy.context.object, mat_name)


def limb(name, start, end, radius, mat_name, vertices=6):
    start_v, end_v = Vector(start), Vector(end)
    direction = end_v - start_v
    midpoint = (start_v + end_v) * 0.5
    obj = cylinder(name, midpoint, radius, direction.length, mat_name, vertices)
    obj.rotation_mode = "QUATERNION"
    obj.rotation_quaternion = Vector((0, 0, 1)).rotation_difference(direction.normalized())
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    return obj


def base(radius=0.43):
    return cylinder("Base", (0, 0, 0.035), radius, 0.07, "dark_steel", 16)


def humanoid(name, body_mat="blue", skin_mat="skin", scale=1.0, helmet=False, robe=False, weapon="sword"):
    base(0.43 * max(1.0, scale))
    leg_z = 0.43 * scale
    limb("Leg_L", (-0.13 * scale, 0, 0.10), (-0.13 * scale, 0, 0.76 * scale), 0.09 * scale, "leather")
    limb("Leg_R", (0.13 * scale, 0, 0.10), (0.13 * scale, 0, 0.76 * scale), 0.09 * scale, "leather")
    if robe:
        cone("Robe", (0, 0, 0.78 * scale), 0.36 * scale, 0.22 * scale, 0.92 * scale, body_mat, 8)
    else:
        cube("Torso", (0, 0, 1.03 * scale), (0.29 * scale, 0.18 * scale, 0.38 * scale), body_mat)
    limb("Arm_L", (-0.30 * scale, 0, 1.28 * scale), (-0.43 * scale, 0, 0.84 * scale), 0.075 * scale, skin_mat)
    limb("Arm_R", (0.30 * scale, 0, 1.28 * scale), (0.43 * scale, 0, 0.84 * scale), 0.075 * scale, skin_mat)
    sphere("Head", (0, 0, 1.60 * scale), (0.22 * scale, 0.20 * scale, 0.24 * scale), skin_mat)
    if helmet:
        cone("Helmet", (0, 0, 1.75 * scale), 0.25 * scale, 0.10 * scale, 0.28 * scale, "steel", 8)
    if weapon == "sword":
        limb("Sword", (0.43 * scale, -0.02, 0.83 * scale), (0.58 * scale, -0.02, 1.62 * scale), 0.035 * scale, "steel", 4)
    elif weapon == "staff":
        limb("Staff", (0.43 * scale, 0, 0.12 * scale), (0.43 * scale, 0, 1.70 * scale), 0.035 * scale, "leather", 6)
    elif weapon == "axe":
        limb("Axe_Handle", (0.43 * scale, 0, 0.34 * scale), (0.52 * scale, 0, 1.52 * scale), 0.04 * scale, "leather", 6)
        cube("Axe_Head", (0.52 * scale, 0, 1.50 * scale), (0.18 * scale, 0.055, 0.12 * scale), "steel")
    export(name)


def build_adventurer():
    humanoid("SM_Adventurer", "blue", "skin", 1.0, False, False, "sword")


def build_paladin():
    humanoid("SM_Paladin", "steel", "skin", 1.06, True, False, "sword")


def build_mage():
    humanoid("SM_Mage", "purple", "skin", 1.0, False, True, "staff")


def build_goblin():
    humanoid("SM_Goblin", "leather", "green", 0.72, False, False, "sword")


def build_orc():
    humanoid("SM_Orc", "red", "green", 1.18, False, False, "axe")


def build_skeleton():
    base()
    limb("Spine", (0, 0, 0.65), (0, 0, 1.40), 0.06, "bone")
    limb("Leg_L", (-0.11, 0, 0.10), (-0.10, 0, 0.75), 0.045, "bone")
    limb("Leg_R", (0.11, 0, 0.10), (0.10, 0, 0.75), 0.045, "bone")
    limb("Arm_L", (-0.34, 0, 1.30), (-0.48, 0, 0.78), 0.04, "bone")
    limb("Arm_R", (0.34, 0, 1.30), (0.48, 0, 0.78), 0.04, "bone")
    limb("Shoulders", (-0.32, 0, 1.31), (0.32, 0, 1.31), 0.05, "bone")
    for z in (0.90, 1.03, 1.16):
        limb(f"Rib_{z}", (-0.22, 0, z), (0.22, 0, z), 0.035, "bone")
    sphere("Skull", (0, 0, 1.62), (0.20, 0.17, 0.23), "bone")
    export("SM_Skeleton")


def build_wolf():
    base(0.55)
    cube("Body", (0, 0, 0.66), (0.48, 0.20, 0.24), "dark_steel")
    sphere("Head", (0.52, 0, 0.80), (0.22, 0.18, 0.22), "dark_steel")
    cone("Snout", (0.72, 0, 0.77), 0.13, 0.04, 0.32, "black", 6, (0, math.pi / 2, 0))
    for x, y in ((-0.31, -0.13), (-0.31, 0.13), (0.30, -0.13), (0.30, 0.13)):
        limb("Leg", (x, y, 0.12), (x, y, 0.55), 0.055, "dark_steel")
    limb("Tail", (-0.45, 0, 0.72), (-0.78, 0, 1.02), 0.055, "dark_steel")
    export("SM_Wolf")


def build_spider():
    base(0.55)
    sphere("Abdomen", (-0.15, 0, 0.48), (0.32, 0.27, 0.23), "black")
    sphere("Head", (0.25, 0, 0.48), (0.20, 0.18, 0.17), "red")
    for side in (-1, 1):
        for index, x in enumerate((-0.25, -0.08, 0.10, 0.27)):
            y = side * (0.18 + index * 0.02)
            limb("Leg", (x, y, 0.48), (x - 0.08, side * 0.64, 0.15), 0.035, "black")
    export("SM_Spider")


def build_slime():
    base(0.45)
    sphere("Slime", (0, 0, 0.37), (0.40, 0.38, 0.34), "slime", 2)
    sphere("Eye_L", (0.14, -0.32, 0.45), (0.045, 0.025, 0.06), "yellow")
    sphere("Eye_R", (-0.14, -0.32, 0.45), (0.045, 0.025, 0.06), "yellow")
    export("SM_Slime")


def build_warforged():
    humanoid("SM_Warforged", "dark_steel", "steel", 1.04, True, False, "sword")


def build_drake():
    base(0.72)
    sphere("Body", (0, 0, 0.72), (0.52, 0.28, 0.31), "red")
    sphere("Head", (0.56, 0, 0.91), (0.25, 0.20, 0.21), "red")
    cone("Snout", (0.80, 0, 0.88), 0.15, 0.04, 0.36, "dark_green", 6, (0, math.pi / 2, 0))
    limb("Tail", (-0.42, 0, 0.75), (-1.05, 0, 0.48), 0.10, "red")
    for x, y in ((-0.28, -0.18), (-0.28, 0.18), (0.30, -0.18), (0.30, 0.18)):
        limb("Leg", (x, y, 0.18), (x, y, 0.62), 0.075, "red")
    cone("Wing_L", (-0.05, 0.34, 1.02), 0.46, 0.02, 0.72, "red", 3, (math.pi / 2, 0, 0))
    cone("Wing_R", (-0.05, -0.34, 1.02), 0.46, 0.02, 0.72, "red", 3, (-math.pi / 2, 0, 0))
    export("SM_Drake")


def export(asset_name):
    bpy.ops.object.select_all(action="SELECT")
    bpy.context.view_layer.objects.active = bpy.context.selected_objects[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = asset_name
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    path = os.path.join(OUT_DIR, f"{asset_name}.fbx")
    bpy.ops.export_scene.fbx(
        filepath=path,
        use_selection=True,
        object_types={"MESH"},
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="-Y",
        axis_up="Z",
        bake_anim=False,
        add_leaf_bones=False,
    )
    print(f"Exported {path}")


GENERATORS = (
    build_adventurer,
    build_paladin,
    build_mage,
    build_goblin,
    build_orc,
    build_skeleton,
    build_wolf,
    build_spider,
    build_slime,
    build_warforged,
    build_drake,
)


for generator in GENERATORS:
    reset_scene()
    generator()

reset_scene()
print(f"Owlbear 3D starter pack complete: {len(GENERATORS)} FBX files in {OUT_DIR}")

