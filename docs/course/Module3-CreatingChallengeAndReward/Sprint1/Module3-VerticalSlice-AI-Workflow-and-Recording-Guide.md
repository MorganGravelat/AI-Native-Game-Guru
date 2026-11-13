# Grant Money Survivor — Module 3 Vertical Slice
## Development, AI Workflow, and Course Recording Guide

**Audience** · The developer reproducing the slice and the instructor recording how it was built.

**Starting point** · One playable TileGen map, a player with an automatic magic orb weapon, a slime that pursues the player, and shared health, stats, and pickup systems. Slimes already dropped XP. The team had also implemented rare stat boosts that last for the run (SCRUM-53). The work below starts at the nine Copilot prompts for turning those pieces into a complete run.

**Sources** · The project conversation, the attached `SCRUM-31-30-38-42-ScriptDocumentation.md` and `SCRUM-53-ScriptDocumentation.md`, and the repository source previously inspected for the boss setup. Use the two SCRUM documents for their full diffs, tests, and original recording notes. Prompt blocks below are condensed for reproduction. Asset settings and outcomes described as test targets still need to be shown in the editor.

## Part A — Change report and milestone order

| Order | Jira scope | What the prompt asked for | Evidence available for recording |
|---|---|---|---|
| 1 | SCRUM-31, 30, 38, 42 | Start a run after the map and player are ready. Spawn slimes at safe positions, animate them rising, and switch to Boss Phase at zero. | Spawner guide records a successful build, 100/100 Automation tests, and a headless run on PlayGame1. |
| 2 | SCRUM-44 | Connect player death to a loss state, show results, and restart the same level with clean state. | Game Over widget binding names and a damage command were provided. End-to-end results were not supplied. |
| 3 | SCRUM-36 | Turn XP into levels and upgrade choices. Apply choices through existing weapon stats and cap stacks. | Level Up widget setup was discussed. Choice, cap, multi-level pickup, and restart results were not supplied. |
| 4 | SCRUM-37, 29 | Increase spawn pressure over time and make direct pursuit handle ordinary obstacles. | The spawn ramp is already in the combined spawning milestone. Terrain traversal needs its own gameplay check. |
| 5 | SCRUM-43, 42 | Spawn a prototype boss when time expires. Resolve victory when it dies. | Repo inspection confirmed boss encounter/class configuration and the slime lunge. Custom boss mesh was reported created. In-game assignment and victory still need evidence. |
| 6 | SCRUM-45 | Show timer, health, level, XP, and boss state with a minimal HUD. | Initial run HUD is documented. Check the final HUD in the current build before recording. |
| 7 | SCRUM-39 | Add a temporary world reward using the existing stat modifier system. | Totem charging sound directions were developed. Gameplay, activation, expiry, and reset still need evidence. |
| 8 | SCRUM-30, 40 | Replace visual placeholders and integrate combat, reward, boss, and menu audio. | Developer confirmed the Meshy GLB worked and the regular slime spawner setup worked. Audio prompts were iterated. Final audio assignments and mix need capture. |
| 9 | SCRUM-46, 42, 34/41/47 | Assemble the single level, verify win/loss/restart, package for Windows, and prepare course material. | This guide and the ticket guides support recording. A packaged build result was not supplied. |

The original plan proposed survival time as score. During implementation the developer chose **character level as the progression readout**, and the spawner guide records removal of the separate score line. The same revision moved the difficulty ramp into the spawner. Reuse that ramp for prompt 4.

### Source and asset landmarks

| Location | What to inspect |
|---|---|
| `Game/Source/GrantMoney/` | `RunController`, `EnemySpawner`, `EnemySpawnPlacement`, `EnemyEmergenceComponent`, `RunHUD`, `GrantMoneyGameMode`, `TileMapRenderer` |
| Same module | `HealthComponent`, `ContactDamageComponent`, `StatsComponent`, `StarterWeaponComponent`, `MagicOrbProjectile`, `XpOrbPickup`, `StatBoostPickup` |
| Boss code | `BossEncounter` selects the class and drives the fight lifecycle. `BossSlimeEnemy` supplies the boss visuals and lunge settings. |
| Unreal content | `WBP_GameOver`, `WBP_LevelUp`, `BP_SlimeEnemy`, `BP_EnemySpawner`, `BP_GrantMoneyGameMode`, and boss Blueprint assets described below. Check their actual paths in the recording checkout. |
| `scripts/run_tests.py` and `docs/course/Module3-CreatingChallengeAndReward/` | Repo test entry point and course documentation. Record the checkout commit and map used in the video. |

## Part B — Reproduce the implementation with Copilot

### Give Copilot the same working context at each milestone

Open the Unreal repository in VS Code and give Copilot one milestone at a time. Have it identify the classes it will reuse before making changes. Preserve the ticket prompt, the relevant diff, and the first useful failure or correction while the details are fresh.

```text
Read CLAUDE.md and applicable AGENTS.md files. Inspect the current checkout and map this ticket to existing classes before editing. Reuse the current run, combat, stats, and pickup systems. Implement the smallest C++ change that meets the ticket and expose the values I need to set in Unreal.

Run the required build and Automation checks when available. Report what ran and what needs Play In Editor. Add a ticket ScriptDocumentation.md with the goal, prompt, changed files, editor setup, actual failures and fixes, test evidence, and SAY / SHOW / VERIFY cues for a later recording.
```

### 1. Spawn slimes and start the clock

```text
Wait until the map and player are ready. Start one configurable survival timer and expose its run state. Spawn slimes in a ring around the player on valid ground, with clearance checks, bounded retries, and an active cap. Grow the visual from a flat shape while keeping its base at ground level. Enable targeting, pursuit, and contact damage when emergence finishes. Freeze the timer on pause and enter Boss Phase once at zero.
```

The documented setup starts after a one-second Starting phase, uses 180 seconds of survival time, and ramps from a 3.5-second spawn interval and six-enemy cap to a 0.75-second interval and 30-enemy cap over 150 seconds. Those values describe the recorded milestone. Read the active Blueprint or placed actor for current demo values. In the course, show the early run and a later wave so viewers can see the effect of the interval, batch size, and cap together.

### 2. Wire player death and restart

```text
Use the existing health depletion event to enter Lost once. Stop spawning, combat, movement, and collection while the result UI is up. Show survival time and XP. Restart the same level with a fresh player, timer, XP, modifiers, enemies, pickups, projectiles, and boss state. Verify two loss and restart cycles.
```

1. Create `Content/UI/WBP_GameOver` as a Widget Blueprint with parent `GameOverWidget`.
2. In Designer, use a centered Vertical Box with a title, Text widgets named `SurvivalTimeText` and `ExperienceText`, and Buttons named `RestartButton` and `QuitButton`. Put a label inside each button. Enable **Is Variable** on the four named controls and compile.
3. Confirm the local controller's widget class setting or expected asset path. The parent handles the bindings and button actions.
4. In a development play session, use `GrantMoney.DamagePlayer 1000 3` if the command exists in the checkout. The delay gets past the Starting phase. Confirm the game freezes and the result values and Restart work. Use Standalone Game for the Escape test so the editor's Escape shortcut does not end the session.

### 3. Turn XP into upgrade choices

```text
Read the XP already stored by StatsComponent. Use configurable thresholds, carry excess XP forward, and queue a choice for every level earned. Present up to three eligible options. Apply capped damage, attack speed, projectile size, penetration, and multishot upgrades through the current weapon and stat systems. Pause gameplay until the queued choices are resolved. Check interaction with permanent drops and reset on restart.
```

Create `Content/UI/WBP_LevelUp` with parent `LevelUpWidget`. Make three Buttons named `Option1Button` through `Option3Button`. Give each its own Text child named `Option1Text` through `Option3Text`, respectively. Enable **Is Variable** for all six and compile. If the class is not resolved automatically, assign `WBP_LevelUp` in the level-up widget class field on `BP_MapCameraPlayerController`, if exposed. Verify that the map's active GameMode uses that controller.

Test a visible upgrade first, then stack one upgrade to its cap. For a multi-level pickup, temporarily raise one orb's XP or lower two thresholds. Confirm that both selections appear and excess XP carries over. Restore the normal values after the test. Record the next run starting at its base level and stats.

### 4. Verify difficulty and direct pursuit

```text
Inspect the ramp already in EnemySpawner. Tune its start/end interval, batch size, and cap on the demo map. Reuse existing placement checks and keep the minimum safe radius. Inspect SCRUM-29 separately. Demonstrate direct pursuit over a small obstacle with bounded step or slide behavior, and check that enemies stay inside the playable arena.
```

### 5. Connect the boss phase

```text
Use the existing run state and spawn placement checks to create one boss on entry to Boss Phase. Reuse shared health, targeting, and damage. Give the boss a distinct telegraphed lunge. On boss death enter Won once and open the results UI. Test loss during the fight, a successful win, and restart from both outcomes.
```

Use the existing `BossSlimeEnemy` and `BossEncounter` classes where present. The asset setup in Part C gives the existing boss its own model without adding a second fight controller.

### 6. Finish the HUD and input flow

```text
Read the run timer, player health, level, XP progress, and boss health from their existing owners. Display boss health during the fight. Coordinate cursor and focus across pause, level-up, Game Over, and Victory. Check that modal UI closes or stays open as intended when the run changes state, including on restart.
```

### 7. Add one temporary totem reward

```text
Use the current stat modifier system to make a world totem with a configurable temporary attack-speed buff. Keep the timed modifier on the player so removing the totem does not end the buff. Remove its own modifier on expiry and preserve permanent boosts. Show charge or activation feedback and verify duration, pause, and restart.
```

Teach the difference in lifetime. SCRUM-53 drops grant boosts for the run. The totem demonstrates a timed modifier that can expire independently.

### 8. Add assets and event-driven audio

```text
Find the existing asset fields and gameplay events before adding new hooks. Make the regular and boss slime visuals replaceable in Blueprint. Connect approved sounds to weapon fire, hit, death, XP, reward, level-up, boss arrival, lunge, results, and UI events. Let the first boss damage hit play immediately and gate subsequent hurt sounds with an editable per-boss cooldown. Check the mix in a multishot fight.
```

### 9. Assemble and verify the vertical slice

```text
Run the full loop on one gameplay map from startup through win and loss. Check pause, upgrades, rewards, countdown, boss arrival, and clean restart. Build and test from the recording checkout and verify the packaged Windows build with its assigned maps, widgets, materials, and audio. Log actual checks and prepare a capture list with pending items marked.
```

## Part C — Make and wire the assets

### Meshy — regular slime and boss models

1. Generate or texture a simple low-poly slime. A useful starting prompt is **“Single stylized low-poly green jelly slime for a 3D arcade game. Round compact body, flat base, and a clear cartoon face visible from a distance.”** For the boss, ask for a broader silhouette and a darker palette with a distinct face. The developer made a separate boss mesh.
2. Rotate the textured **3D preview** in Meshy and confirm the face is visible on the model. Download that textured model as **GLB**. Meshy documents that GLB embeds the textures. Import into a dedicated Unreal content folder with materials and textures enabled. Open the Static Mesh and check the face and material slots before assigning it anywhere. [Meshy texture export guide](https://help.meshy.ai/en/articles/15724161-how-to-export-meshy-models-with-colors-and-textures)
3. The first import showed green color but the face was missing on the intended mesh. The material preview sphere had shown parts of the face, so it was an unreliable check of the model's UVs and material assignment. The developer confirmed that a GLB import worked. The original cause of the missing face was not isolated. Reuse the working mesh and its imported material as a pair. Clear an old component material override if it masks the mesh material.
4. Create `BP_SlimeEnemy` as a child of C++ `SlimeEnemy`. Assign the GLB Static Mesh on **PlaceholderMesh**. Align the model's base with the bottom of the capsule and its face with the forward arrow. Fit the visual to the roughly 60 cm capsule. Adjust the mesh component transform so the collision and emergence systems keep their existing dimensions.
5. If a spawner is placed in the map, set its **Enemy Class** to `BP_SlimeEnemy`. Otherwise create `BP_EnemySpawner` under C++ `EnemySpawner`, set **Enemy Class** in Class Defaults, and assign the spawner Blueprint under **Enemy Spawner Class** on `BP_GrantMoneyGameMode`. Compile and save. Play the map and check a freshly spawned enemy.
6. For the separate boss mesh, create `BP_BossSlimeEnemy` under C++ `BossSlimeEnemy`. Fit **PlaceholderMesh** to the boss's roughly 180 cm capsule. Set **Body Material Base** in Class Defaults to the imported material used by slot 0. The C++ boss assigns that field to slot 0 at spawn, so check the result during Play. The optional lunge color flash needs a vector parameter matching **Color Parameter Name**.
7. If a **BossEncounter** is placed, set its **Boss Class** to `BP_BossSlimeEnemy`. Otherwise make `BP_BossEncounter` under C++ `BossEncounter`, set **Boss Class** in Class Defaults, and assign the encounter Blueprint as **Boss Encounter Class** in `BP_GrantMoneyGameMode`. Verify the gameplay map is using that GameMode and watch what actually spawns at zero. [Boss encounter source](https://github.com/MorganGravelat/AI-Native-Game-Guru/blob/main/Game/Source/GrantMoney/Public/BossEncounter.h)

### ElevenLabs — effects that work during combat

Use **Sound Effects**. Prompt for one event at a time, describing the attack, texture, and tail. Generate several takes. Trim leading silence so playback starts with the gameplay event, then audition the clip with music and other effects. The times below are target edits. For very short clips, trim a longer generation if needed. Export or convert the selected clips to WAV for the Unreal import workflow. [ElevenLabs Sound Effects](https://elevenlabs.io/docs/help-center/product/core-capabilities/sound-effects/what-is-sound-effects)

| Cue | Example generation direction | Playback check |
|---|---|---|
| Orb fire | One bright magic pop with an immediate outward zip, clean and dry. | Audition rapid fire and multishot. |
| Slime hit and death | Hit is one small rubbery wet plip. Death is a deeper jelly collapse with two short droplet sounds. Generate each separately. | A death should read clearly in a crowd. |
| Player hurt | One muted thup like a beanbag hitting a thick jacket, with a small fabric rustle. | Keep repeated damage understated. |
| XP, power-up, and level-up | XP is one tiny glass plink. Power-up is a two-note rising chime. Level-up is a short three-note synth-bell fanfare. | The common pickup should be quieter than the rarer reward. |
| Boss lunge | A giant slime launches like a released slingshot. Start with one sharp rubber snap, immediately followed by a thick rush of air. About 0.3 seconds, clean cutoff. | Play the existing charge-up during windup and this sound at launch. Test consecutive lunges. |
| Boss hurt | One low, muffled jelly impact like tapping a thick silicone gel pad. Tiny wet wobble, dry and close, about 0.15 seconds. | Start at a shared 0.3-second cooldown per boss. First hit plays immediately, while every hit still deals damage. Test with rapid multishot. |
| Boss arrival and death | Arrival is a deep gooey thump with a short rising warning. Death is a heavy hollow pop into a jelly collapse. | Keep these on their own events so a recent hurt sound cannot block them. |
| Totem charging and ready | Charge is a soft ceramic resonance rising with a faint shimmer. Ready is a separate bright chime. | Match a fixed charge duration, or loop a steady hum and drive pitch from progress. Fade it when charging stops. |
| Game over, victory, UI | Descending arcade synth for loss, a short resolved synth-brass fanfare for victory, and a soft dry plastic tick for menus. | Check the result stings against boss death and test frequent button presses. |

**Iteration to record.** The first dash generations did not work for the developer. The revised prompt described the instant of launch and removed the charge and impact from the clip. A separate problem was repeated boss pain audio during multishot. The proposed fix combined a short, low jelly impact with a gameplay cooldown. Record the raw take and the in-game result if available. Label the cooldown as a proposed setting until it is implemented and tested.

### Suno — gameplay and boss loops

Use **Custom** and turn on **Instrumental**. Paste the style direction into the style field. Generate alternatives and compare them during actual gameplay. The reference MP3, *Wednesday Morning Bonk It's 9AM* from the MEGABONK OST, was used to discuss tempo and energy. Signal analysis found a pulse near **143 BPM**, also countable around **72 BPM**. The first prompt's 112 BPM was an estimate and was revised after the analysis. The instrument choices below are creative directions. [Suno Custom Mode](https://help.suno.com/en/articles/3197377)

**Regular gameplay**

```text
Instrumental retro arcade funk for a low-poly survivor game. Around 143 BPM with a relaxed half-time bounce. Springy syncopated bass, punchy dry drums, bright electric-key stabs, and a short playful synth hook. Start on the groove, use small drum variations, and return to the opening bass and drum pattern. Leave room for weapon and pickup sounds. No vocals, long intro, or fade-out.
```

Other explored directions were PS1-style jungle with chopped breaks, cartoon surf rock with guitar and organ, and industrial arcade techno for a heavier boss fight.

**Slime boss**

```text
Instrumental dark arcade funk for a giant slime boss, around 146 BPM. Heavy half-time snare, tight breakbeat fills, thick rubbery bass in a minor key, a sly descending riff, and short detuned organ stabs. Tense and sinister with some arcade swagger. Start on the groove and leave gaps for the boss windup and dash cues. Repeat a 16-bar phrase and return cleanly to the opening riff. No vocals, long intro, or fade-out.
```

The first boss direction was 155 BPM. The revision made it slightly slower and more sinister in response to the developer's feedback. In an audio editor, find a steady 16- or 32-bar phrase, cut from a downbeat to just before the matching downbeat, and listen to at least two repeats. Fix the seam before importing. Preview the boss theme entering after the arrival sting and make sure the windup stays audible over the music.

## Part D — AI workflow log

| Decision or problem | AI contribution | Developer decision or evidence |
|---|---|---|
| Scope was too wide for a single implementation prompt | ChatGPT mapped nine prompts to Jira and identified the dependencies. | Work proceeded from spawn/timer to loss and progression, then boss and asset setup. |
| Spawns failed in early tests | Copilot added temporary logging and isolated the clearance overlap with the ground. | The spawning guide documents the fix, rerun, and headless smoke test. |
| Difficulty felt too fast and a separate score was unnecessary | Copilot added an editable time ramp and removed the separate score. | Developer chose level as progression and asked for a slow start with increasing pressure. |
| UI controls needed to bind to native widgets | ChatGPT translated required names into exact Designer hierarchy and class assignment steps. | Editor behavior still needs to be recorded on the current checkout. |
| Imported slime showed green with no face | ChatGPT suggested comparing the textured Meshy model with a clean GLB import on the actual mesh. | Developer reported the GLB and regular slime spawner worked. |
| Boss needed its own look | ChatGPT checked the code for spawn class selection and the runtime material assignment. | Developer made a separate boss mesh. Capture the final Blueprint and Play In Editor result. |
| Lunge and repeated hurt audio needed revision | ChatGPT wrote more specific ElevenLabs prompts and suggested gating repeated hurt playback. | Developer already had a charge-up. Final selected clips and mix settings need evidence. |
| Music needed a closer groove and darker boss version | ChatGPT analyzed the reference's rhythmic pulse and revised the Suno prompts. | Developer requested a slightly slower, more sinister boss direction. Final track selection and loop test need evidence. |

## Part E — Recording script and verification

| Beat | SAY | SHOW | VERIFY |
|---|---|---|---|
| 1. Opening | “We had combat and pickups. Our next job was making those systems into one playable run.” | Early gameplay, the nine-prompt order, and the two ticket guides. | Show the starting systems and the milestone dependencies. |
| 2. Spawn and timer | “I asked Copilot to read the map and enemy systems first. The spawner waits for the map, checks placement, and ramps pressure as the timer runs.” | PlayGame1, emergence, early and late waves, one real test failure and correction. | Show safe placement, increasing pressure, and the exact evidence for the earlier build and tests. |
| 3. XP and decisions | “The level-up menu reads the existing XP and offers upgrades. Permanent drops and temporary rewards use the same stats with different lifetimes.” | XP collection, one visible upgrade, then a totem activation and expiry if implemented. | Show an upgrade cap, two choices from one pickup, and timed buff expiry. |
| 4. Meshy and Unreal | “The material preview showed green, but the actual slime lost its face. Importing the textured GLB gave us a working mesh and material together.” | Meshy preview, Static Mesh preview, `BP_SlimeEnemy`, spawner defaults, and separate boss mesh. | Show the face on a newly spawned slime and the boss material during Play. |
| 5. Sound and music | “The lunge needed a clean launch cue. Multishot also made repeated hurt sounds a mix problem, so we tested the hits together.” | ElevenLabs takes, trimmed waveforms, charge then dash, and Suno gameplay-to-boss transition. | Confirm cue timing, audible windup, controlled hurt repetition, and loop seams. |
| 6. Finish the run | “The timer starts the boss fight. Defeating it wins the run. Losing and restarting should give us a fresh game.” | Boss arrival, victory, a separate death, and restart. | Verify one boss, one terminal result, and reset XP, level, modifiers, timer, enemies, and pickups. |

### Capture checklist

| Suggested file | Capture |
|---|---|
| `01-prompt-and-diff.png` | Actual Copilot prompt, inspected classes, and relevant changes. |
| `02-spawn-ramp.mp4` | Starting phase, emergence, countdown, and later waves. |
| `03-xp-upgrade-totem.mp4` | XP, choice, visible stat change, and temporary reward if available. |
| `04-meshy-to-unreal.mp4` | Textured preview, GLB import, Blueprint fields, and a newly spawned slime. |
| `05-boss-audio.mp4` | Boss model, charge and dash, multishot damage, and music transition. |
| `06-win-loss-restart.mp4` | Victory, a separate loss, and a fresh run after Restart. |
| `07-build-and-tests.png` | Actual checkout, build result, Automation result, and packaged run evidence where available. |

Keep raw captures outside Git. If recording later, label recreated steps as demonstrations and use the original test logs when describing what happened during development. The combined spawning milestone's **100/100** result is dated **November 12, 2026** and applies to that documented checkout. SCRUM-53 records a developer-reported successful build and Automation run. A headless smoke test verifies log behavior, while mesh alignment, widget interaction, audio balance, and the final packaged loop require their own visible checks.
