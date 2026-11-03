# Guide: Changing the Player Visual (Model, Animations, Scale, Orientation)

**Audience:** Any teammate who needs to change how the player looks or animates in Grant Money.
**Scope:** Epic's Manny is the default. All values below can be changed without touching gameplay code, except where noted.

---

## 1. How it works (30 seconds)

- `AGrantMoneyPlayerCharacter` ships with **built-in defaults**: Manny mesh, idle and jog animations. These are set in its constructor.
- An optional data asset, **`DA_PlayerVisual`**, overrides those defaults when the game starts. It lives at:

```text
Game/Content/Characters/Player/DA_PlayerVisual
```

- The character looks for that exact path. If the asset is missing, renamed or moved, it **silently** uses the built-in defaults. Do not rename or move it.
- Changes to the data asset need **no recompile**. Save the asset and press Play.

| What the data asset controls | Field |
|---|---|
| Model | Skeletal Mesh |
| Animation Blueprint (advanced) | Anim Class |
| A single looping animation | Preview Animation |
| Position adjustment | Mesh Offset |
| Orientation | Mesh Rotation |
| Size | Mesh Scale |

---

## 2. Before you change anything

1. Close Unreal before switching branches.
2. Create a ticket branch from a fresh `main`:

```bash
git checkout main
git pull origin main
git lfs pull
git checkout -b GMS-XXX-short-description
```

3. Tell the team before editing `DA_PlayerVisual.uasset`. It is a binary file tracked by Git LFS and **cannot be merged**. Only one person should change it at a time.

---

## 3. Common changes

### Change the model (Manny to Quinn, or any mesh)

1. Open `DA_PlayerVisual`.
2. Set **Skeletal Mesh** to the new mesh (for example `SKM_Quinn_Simple`).
3. Save (`Ctrl+S`) and press Play. No rebuild needed.

**Condition:** the new mesh must use the **same skeleton** as the animations in use. Manny and Quinn share the Mannequin skeleton, so this works. A mesh with a different skeleton needs section 4.

### Change the idle or jog animation (built-in logic)

The idle and jog animations are not data asset fields. They are defaults in the character's constructor.

1. Open `Game/Source/GrantMoney/Private/GrantMoneyPlayerCharacter.cpp`.
2. Find the two `FObjectFinder<UAnimationAsset>` lookups (`IdleFinder`, `JogFinder`).
3. Change the asset path strings. Copy the exact path from the Content Browser (right-click the asset, then Copy Reference).
4. **Close the editor and do a full build** (`Development Editor | Win64`), then test.

The new animations must be built for the same skeleton as the mesh.

### Play one fixed animation (testing a clip)

1. In `DA_PlayerVisual`, set **Preview Animation** to the clip.
2. Save and press Play.

**Side effect:** setting Preview Animation (or Anim Class) **switches off the built-in idle/jog switching**. The character will play that one clip whether standing or moving. Clear the field to restore idle/jog.

### Change scale

1. In `DA_PlayerVisual`, change **Mesh Scale** (for example `1.5, 1.5, 1.5`). Keep the three values equal.
2. Save and press Play.

**Warning:** this scales only the mesh. The capsule (collision) does not scale with it, so a larger mesh can visibly overlap walls. The Manny mesh is about 180 uu tall against 100 uu tiles. If you need the collision to match, change the capsule in C++ and tell the team, because it affects movement and collision.

### Change orientation (which way he faces)

1. In `DA_PlayerVisual`, change **Mesh Rotation**. The default is **Yaw = -90**.
2. In the editor's Rotation fields, **Yaw is the Z field**, not the first one.
3. If the character faces sideways while moving, adjust Yaw in steps of 90.

The Manny mesh faces +Y in its own space, which is why the default is -90. A mesh from another source may need a different value.

### Move the mesh up or down

1. In `DA_PlayerVisual`, change **Mesh Offset**. It is added to the automatic "feet at the capsule bottom" position.
2. Use Z to fix floating or sinking feet. Positive Z raises the mesh.

---

## 4. Using a mesh with a different skeleton

If the new mesh doesn't use the Mannequin skeleton, the Manny idle and jog animations will **not** play correctly. Two options:

**A. Retarget the existing animations**
1. Create an **IK Rig** for the Mannequin skeleton and one for the new skeleton. The Mannequin ships IK Rigs in its `Rigs` folder, so check there first.
2. Create an **IK Retargeter** that maps one rig to the other.
3. In the retargeter, select the animations and use **Export Selected Animations**.
4. Point the character at the exported animations (constructor paths, as above) or use Preview Animation to test.

**B. Use the new model's own animations**
1. Import the model and its animations (FBX or glTF) into `Content/Characters/`.
2. Record the import settings (below).
3. Set the mesh in `DA_PlayerVisual`, then update the idle and jog constructor paths.

**Import settings to record** (they go in the task doc): skeleton (new or assigned), Convert Scene, forward axis, uniform scale, and whether animations were imported.

---

## 5. Animation Blueprint route (later work, GMS-104)

The data asset has an **Anim Class** field for a proper Animation Blueprint. When it is set, the built-in idle/jog logic switches off and the Animation Blueprint takes over.

Confirm with the technical lead before using it. Team rules allow no Blueprint gameplay logic, so the planned pattern is a C++ `UAnimInstance` subclass computing values like speed, with the Animation Blueprint holding only the state machine. This project has no Animation Blueprint today.

---

## 6. Verification checklist (run after every change)

- [ ] Output Log has no missing-asset or `[VisualDebug]` warnings.
- [ ] The character is visible and not black. (If black, the level is probably unlit. Ask who owns lighting in `PlayGame.umap`.)
- [ ] Feet touch the tile plane, with no floating or sinking.
- [ ] The character faces the direction it walks.
- [ ] Idle plays when still and jog plays when moving with WASD.
- [ ] Scale looks right against the 100 uu tiles and the capsule.
- [ ] Camera, pause menu and tile rendering are unchanged.
- [ ] `git lfs ls-files` lists every new `.uasset`.

---

## 7. Troubleshooting

| Symptom | Likely cause | What to do |
|---|---|---|
| Changing the mesh in the data asset does nothing | Asset name, folder or class is wrong, or the edit was never saved | Copy the asset's reference and compare to `/Game/Characters/Player/DA_PlayerVisual`. Hover to confirm the class is `GrantMoneyCharacterVisualData`. Save all. |
| Same, but the path is correct | A Blueprint subclass or a level instance saved its own value | Check the Visual Data field on any character Blueprint or placed character. Reset it to default. |
| Changes don't appear after a C++ edit | The editor is running an old build | Close Unreal and do a full build. Don't rely on Live Coding for header changes. |
| Idle plays but jog never does | Ticking is off, or the jog animation is null | Confirm `PrimaryActorTick.bCanEverTick = true`. Check Jog Animation isn't `None` in the Details panel during Play (press F8 to eject first). |
| The character stays in one pose | Preview Animation or Anim Class is set in the data asset | Clear the field to restore idle/jog. |
| Character is a T-pose or distorted | Skeleton mismatch between mesh and animation | Use section 4. |
| Mesh sideways or backward | Orientation value | Change Mesh Rotation Yaw (Z field) in steps of 90. |
| Mesh is black | Unlit level | See the verification checklist. |
| Build error "declaration of Mesh hides a class member" | A local variable is named `Mesh` | Rename it (for example `LoadedMesh`). `ACharacter` already has a member named `Mesh`. |

If a log line is needed, ask the code owner to add temporary `UE_LOG` lines inside `ApplyVisualData()`. Every exit in that function returns quietly otherwise.

---

## 8. Committing your change

```bash
git status
git diff --cached --stat
git lfs ls-files
git add Game/Content/Characters/ Game/Source/
git commit -m "GMS-XXX: describe the change"
git push -u origin GMS-XXX-short-description
```

Run `git status` first and add only the paths you meant to change. Open a PR with screenshots of the before and after in `PlayGame`, then move the Jira item to In Review.
