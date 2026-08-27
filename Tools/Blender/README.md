# Blender starter-pack generator

This generator creates 11 original, flat-shaded low-poly FBX miniatures at Unreal scale:

- Adventurer
- Paladin
- Mage
- Goblin
- Orc
- Skeleton
- Wolf
- Spider
- Slime
- Warforged
- Drake

## Generate

From a terminal with Blender available:

```powershell
blender --background --python Tools\Blender\generate_starter_pack.py
```

Or open the script in Blender's **Scripting** workspace and press **Run Script**.

The generated files appear under `Tools/Blender/GeneratedFBX`. Import them into Unreal under `/Game/StarterPack/Creatures` with a uniform scale of `1.0` and **Combine Meshes** enabled.

The meshes and generator are original Owlbear 3D project assets and may be modified with the project.

