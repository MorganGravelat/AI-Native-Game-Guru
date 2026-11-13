# Grant Money / AI-Native Game Guru - Claude Rules

## Primary Goal

Help develop the Unreal Engine Grant Money project efficiently while preserving context and avoiding unnecessary file reads.

This repository is an Unreal Engine C++ project.

## Token and Context Rules

1. Do not scan the entire repository unless explicitly asked.
2. Start with the smallest relevant set of files.
3. Prefer targeted search by class name, function name, Jira key, filename, or symbol.
4. Do not repeatedly reread files already inspected unless they changed or the missing context requires it.
5. Do not dump entire large files into context when only one class/function/section is relevant.
6. Read headers before implementations when trying to understand an unfamiliar class.
7. When investigating a bug, first identify likely files and explain which files you intend to inspect.
8. Stop searching once enough evidence exists to perform the requested task.
9. Do not recursively inspect generated Unreal directories.
10. Never try to interpret binary Unreal assets as text.

## Never Read

Do not inspect or attempt to parse:

- Binaries/
- Intermediate/
- Saved/
- DerivedDataCache/
- .git/
- .vs/
- .vscode/
- *.uasset
- *.umap
- *.dll
- *.exe
- *.pdb
- *.obj
- *.pak
- *.ucas
- *.utoc
- images
- videos
- audio
- archives

If information from an Unreal asset is necessary, ask for its relevant Editor settings or use available Unreal-readable metadata rather than attempting to read the binary.

## Preferred Project Files

Prioritize:

- Game/Source/**/*.h
- Game/Source/**/*.cpp
- Game/Config/**/*.ini
- Game/*.uproject
- docs/**/*.md
- scripts/**/*.py
- .gitignore
- .gitattributes

## Unreal Rules

- Gameplay logic should remain primarily in C++.
- Do not introduce Blueprint gameplay logic unless explicitly requested.
- Existing Blueprint/widget assets may be used for visual configuration where already part of the project.
- Preserve existing MapArrays and TileGen abstractions.
- Do not expose generated TileGen filenames throughout gameplay code.
- Prefer reusable/data-driven systems over map-specific hardcoded coordinates.
- Preserve existing systems unless the requested change requires modifying them.

## Testing Rules (mandatory)

Every new C++ file with gameplay or logic code gets an Automation test in the same change.

- Put the test next to the code in `Private/`, named `<ClassName>Tests.cpp`
  (see `Game/Source/GrantMoney/Private/EnvironmentPlacerTests.cpp` for the house style).
- Wrap the whole file in `#if WITH_DEV_AUTOMATION_TESTS` and include `Misc/AutomationTest.h`.
- Use `IMPLEMENT_SIMPLE_AUTOMATION_TEST` (or `BEGIN_DEFINE_SPEC` for many related cases) with
  `EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter`.
- Test names: `GrantMoney.<Area>.<Class>.<Case>`, e.g. `GrantMoney.Environment.Placer.BoundsPaddingSpacing`.
- Prefer testing pure logic against synthetic data (as the EnvironmentPlacer tests do) over
  loading maps or assets.
- When you change existing logic, update or add tests that cover the change.
- Files with no testable logic (pure UI glue, module boilerplate) may skip tests only with a
  comment line `// NO-TEST: <reason>` in the .cpp. Use this sparingly.

Run the tests with:

    python scripts/run_tests.py            # build GrantMoneyEditor + run GrantMoney.* tests
    python scripts/run_tests.py --skip-build --filter GrantMoney.Environment

The script finds Unreal Engine on any machine (UE_ROOT env var, else the .uproject's
EngineAssociation via the registry / Epic launcher). Never hardcode an engine or user path.
If it reports "Test setup problem", tell the user; don't work around it.

A Stop hook (`scripts/claude_test_gate.py`, configured in `.claude/settings.json`) enforces this:
it blocks finishing while a new .cpp lacks a test or the tests fail. Do not edit or disable the
hook to get past it. If the Unreal Editor is open with Live Coding, the build fails; ask the user
to close it.

## Verification Scope (mandatory)

Headless Automation tests are the AI's job; everything that needs eyes on the screen is the
developer's job. Keep the two separate:

- **Write a test for every new/changed piece of logic** (per Testing Rules above) — not optional.
- **Run only the tests for what was just built or changed this session**, e.g.
  `python scripts/run_tests.py --filter GrantMoney.<Area>`. Do not run the whole `GrantMoney.*`
  suite by default; only do that if the targeted run fails or something looks genuinely broken and
  a wider check is needed to localize it. The developer runs the full suite and does all other
  verification themselves.
- **Never try to observe on-screen/gameplay behavior yourself.** No headless PIE runs, no scripted
  gameplay simulation, no other workaround to approximate what a test can't check directly (visual
  feel, UI layout, stacking/physics "look," Blueprint widget behavior). Don't spend time or tokens
  engineering a way to simulate it — the developer will try it in the Editor and report back.
- **If a scoped test run surfaces a real bug, fix it in the same pass** (fixing what you already
  found is expected), but do not go hunting for more issues beyond what was actually asked for and
  scoped this session.
- Once implementation and the scoped automation tests pass, hand off: ask the developer a short
  batch of yes/no quiz-style questions for anything needing manual/in-editor confirmation (see
  Course Documentation Workflow below), rather than writing more tests or speculative documentation
  to compensate.
- Creating or editing Blueprint/widget assets (`.uasset`) is always the developer's step; the AI
  cannot do it (binary assets) and should give exact, concrete Editor instructions instead.

## Workflow

Before modifying code:

1. Identify the Jira task or requested goal.
2. Inspect only directly relevant files.
3. State the likely files to change.
4. Make the smallest implementation that satisfies the acceptance criteria.
5. Compile/test when possible.
6. Do not claim successful testing unless it actually occurred.
7. Review the diff before considering the task complete.

After completing a milestone, report:

- What changed
- Files changed
- Why each file changed
- How it was tested
- Known limitations
- Suggested Git commit message

## Course Documentation Workflow

While implementing a Jira task, maintain a reproducible instructional trail.

For every meaningful implementation step:

- identify what was done;
- preserve useful AI prompts;
- note AI suggestions that failed or required correction;
- identify a useful screenshot or short recording;
- keep screenshot/clip numbering aligned with the implementation steps.

Do not invent screenshots, recordings, test results, or development steps.

**Manual/in-editor verification:** when the AI needs the developer to confirm something it cannot check itself (PIE behavior, UI, etc. — see Verification Scope above for what that covers), ask a short batch of yes/no quiz-style questions (e.g. via an elicitation tool) rather than a long free-form question or writing speculative documentation. In the ScriptDocumentation.md, just note that this quiz method was used and whether everything was confirmed; do not transcribe each individual question/answer.

At the end of a task, be able to produce:

1. implementation summary;
2. file/change list;
3. reproducible step-by-step guide;
4. AI workflow log;
5. common mistakes / failures;
6. verification procedure;
7. video voice-over script.

## Source Control

- One Jira issue -> one branch -> one pull request.
- Branch names should follow SCRUM-XXX-description (Jira keys are SCRUM-XXX; older GMS-XXX branches are legacy).
- Do not commit generated Unreal folders.
- Do not commit raw video recordings.
- Unreal binary assets should use Git LFS where appropriate.
- Never commit or push unless explicitly instructed.
- Always review changes and ensure compliance with the project's contribution guidelines before pushing.

## Discord User Map To Jira Members

| Discord User | Jira Member |
|--------------|-------------|
| MorganG | Morgan Gravelat |
| Austin | Austin Long | 
| Kai | Kai Gerry |
| Julio | Julio Maldonado |
| Rohaan Mansoor | ro376504 |






