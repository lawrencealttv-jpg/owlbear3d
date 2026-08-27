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
- Click-drag floor, wall, door, and fog brushes
- Placeable crate, table, column, statue, and chest props
- Select, move, rotate, copy, and delete props and pieces
- Piece sizes, conditions, and GM-hidden markers
- 50-step undo/redo
- Autosave after editing
- On-screen GM toolbar
- Distance ruler, d20 roller, and lightweight initiative tracker
- Nine persistent campaign scenes
- Replicated board, doors, fog, pieces, props, HP, sizes, and conditions
- Direct host/join multiplayer foundation
- GM-only editing with player piece movement
- Hidden pieces remain visible to the GM but disappear for remote players
- Blender generator for 11 coherent low-poly starter miniatures
- Automatic fallback to primitive miniatures until generated models are imported
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
| Toolbar / `T` | Cycle floor, wall, door, prop, and fog tools |
| Click and drag | Paint or erase with the active build tool |
| `P` | Cycle prop type |
| `R` | Rotate selected piece/prop |
| `C` | Duplicate selected piece/prop |
| `Z` / `Y` | Undo/redo |
| `K` | Cycle selected piece size |
| `N` | Cycle selected piece condition |
| `H` | Mark selected piece hidden from players |
| `V` / **Model** | Cycle the selected piece's 3D model |
| `M` | Toggle ruler |
| `L` | Roll d20 |
| `I` / `O` | Add selected piece to initiative / next turn |
| `F5` | Save current map |
| `F9` | Load current map |
| `WASD` | Pan camera |
| `Q` / `E` | Rotate camera 45 degrees |
| Mouse wheel | Zoom |

## Multiplayer alpha

1. The GM clicks **Host**. The current scene saves and reloads as a listen server.
2. Players enter the GM's IP address (optionally `address:7777`) and click **Join**.
3. LAN works directly. Internet hosting currently requires UDP port `7777` forwarded to the GM, or a shared virtual LAN such as Tailscale.

This is direct-connect alpha networking. Epic Online Services invitations and account-free join codes come after the core replication test.

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

- Campaign and multi-scene browser
- Direct host/join multiplayer
- Character/monster asset library
- Proper visibility separation between GM and players
