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
- Branch names should follow GMS-XXX-description.
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






