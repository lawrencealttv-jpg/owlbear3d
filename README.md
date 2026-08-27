# Owlbear 3D

A rules-light 3D virtual tabletop built in Unreal Engine 5.8: Owlbear-style controls, a runtime DungeonDraft-style map builder, and deliberately low-poly tabletop pieces.

## Current vertical slice

The project creates its first playable room entirely at runtime. No custom map or Marketplace assets are required.

- 16 x 12 square grid with boundary walls
- Isometric camera with pan, zoom, and 45-degree rotation
- Select and move pieces on snapped grid cells
- Spawn simple hero and goblin pieces
- Visible names and editable HP
- Runtime floor-tile editing
- Runtime wall-edge placement
- Save/load floors, walls, pieces, colours, names, and HP
- Multiplayer-ready actor structure (network session UI comes later)

## Controls

| Input | Action |
|---|---|
| Left click piece | Select piece |
| Left click floor | Move selected piece |
| `1` | Place hero at cursor |
| `2` | Place goblin at cursor |
| `+` / `-` | Change selected piece HP |
| `Delete` | Remove selected piece |
| `B` | Toggle build mode |
| `T` | Switch between floor and wall tools |
| Left click in floor mode | Add/remove floor tile |
| Left click near a tile edge in wall mode | Add/remove wall |
| `F5` | Save current map |
| `F9` | Load current map |
| `WASD` | Pan camera |
| `Q` / `E` | Rotate camera 45 degrees |
| Mouse wheel | Zoom |

## Open on Windows

1. Clone this repository beside the old prototype:

   ```powershell
   cd "C:\Users\krist\Documents\Unreal Projects"
   git clone https://github.com/lawrencealttv-jpg/owlbear3d.git
   ```

2. Right-click `Owlbear3D.uproject` and select **Generate Visual Studio project files**.
3. Open `Owlbear3D.sln` and build the **Development Editor / Win64** target, or double-click `Owlbear3D.uproject` and allow Unreal to compile it.
4. Press **Play** in Unreal Editor.

The project requires the Visual Studio 2022 **Game development with C++** workload plus the Windows SDK. Unreal Engine 5.8 itself is already the correct engine version.

## Development rule

Maps will be serialised as runtime data rather than custom `.umap` files. This keeps user-created maps saveable, shareable, undoable, and eventually synchronised in multiplayer.

## Next milestone

- Drag-to-draw walls and place doors
- Proper in-game GM toolbar
- Character/monster asset library
- Host/join multiplayer session
