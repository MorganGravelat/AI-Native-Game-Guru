# Post-Milestone Documentation and Course Packaging Task

You are working on an AI-native game development project whose purpose is not
only to produce a working game, but to create a reproducible instructional
process that another developer can follow to build a similar result in their
own game.

The milestone that was just completed is:

[MILESTONE NAME]

Examples:
- Procedural Map Boundary Generator
- Dynamic Multi-Map Registration
- TileGen World Renderer
- Procedural Environment Population
- Third-Person Camera Integration
- Player Character Movement
- Enemy Spawning System
- Collectible / Reward System
- Save Game System
- AI Asset Import Pipeline

The intended learner is a software engineer who may know programming but may
not know this project, Unreal Engine, the exact tool, or this specific game.

The instructional goal is NOT:
"Copy GrantMoney exactly."

The instructional goal IS:
"Show a repeatable AI-assisted engineering process that can be adapted to
other games."

This project uses AI as an engineering collaborator. Every result must clearly
separate:
- what AI proposed,
- what the engineer decided,
- what had to be corrected,
- what was actually tested,
- and what remains unverified.

Do not invent tests, files, prompts, bugs, or results that did not occur.

---

# PHASE 1 — Inspect the Current State

Before writing documentation, inspect the repository and determine what
actually changed during this milestone.

Do not assume filenames from this prompt.

Inspect:
- git status
- git diff
- git diff --stat
- relevant C++ source/header files
- Unreal configuration/assets when visible to you
- existing task documentation
- related architecture files
- any AI-generated code produced for this milestone
- any test/log evidence available

Determine:

1. What the system did BEFORE this milestone.
2. What capability exists AFTER this milestone.
3. Which files were created.
4. Which files were modified.
5. Which files were intentionally NOT changed.
6. Whether generated files or binary Unreal assets were touched.
7. Whether any files changed only because of formatting, line endings,
   generated state, or unrelated editor activity.
8. What tests were actually run.
9. What tests still need human verification.
10. What problems were encountered and how they were corrected.

Do not modify unrelated files during this task.

---

# PHASE 2 — Produce a Milestone Change Report

Create a concise changelog with the following sections.

## Milestone Summary

Explain in plain language:
- what was added,
- what problem it solves,
- and why it matters architecturally.

## Files Created

Use a table:

| File | Purpose |
|------|---------|
| path/to/file | What the new file does |

## Files Modified

Use a table:

| File | What Changed | Why |
|------|--------------|-----|
| path/to/file | Specific change | Reason |

## Files Not Changed

Explicitly call out important systems that were intentionally preserved.

Examples:
- TileGen generated arrays
- MapArrays interface
- PlayGame.umap
- M_Tile
- camera settings
- imported textures
- packaging config

This section is important because the developer needs to know what they
should and should NOT stage in Git.

## Git Staging Recommendation

Give exact `git add` commands for only the relevant changed files.

Example format:

git add Game/Source/GrantMoney/Public/SomeFile.h
git add Game/Source/GrantMoney/Private/SomeFile.cpp
git add docs/course/...

Also include:

git status
git diff --cached --stat
git diff --cached

Recommend a commit message in this format:

GMS-XX: short milestone description

Do not commit, push, merge, or stage automatically.

---

# PHASE 3 — Write the Developer Documentation

Create a detailed Markdown implementation guide for this milestone.

The guide should be written so that another developer could reproduce the
same RESULT, even if:
- their game has a different name,
- their art is different,
- their map is different,
- their directory structure differs slightly,
- their level names differ,
- or they use the same architectural idea for a different genre.

Do not make the guide unnecessarily GrantMoney-specific.

Use GrantMoney as the implementation example, but explain the general
principle behind every project-specific action.

Use this structure:

# [Milestone Name]

## 1. Objective

Explain what the developer will have at the end.

## 2. Why This System Exists

Explain the engineering problem being solved.

Discuss:
- why the previous state was insufficient,
- why this architecture was chosen,
- and what alternatives were rejected or deferred.

## 3. Prerequisites

List everything that must already exist.

Examples:
- required Unreal version
- existing C++ project
- prior system/interface
- required assets
- required plugins
- required map/data
- successful previous milestone

## 4. Architecture Overview

Explain the data/control flow.

Include an ASCII diagram when useful.

Example:

Generator Output
    ↓
Stable Data Interface
    ↓
Runtime System
    ↓
Unreal Components
    ↓
Visible / Playable Result

Explain responsibility boundaries.

## 5. AI-Assisted Planning

Include the actual or reconstructed prompt that should be given to AI to
produce the implementation.

The prompt must:
- tell AI to inspect existing code first,
- preserve working systems,
- avoid inventing filenames/symbols,
- use actual repository state,
- keep the scope narrow,
- explain test criteria,
- and not claim unperformed tests succeeded.

Label this clearly:

### AI Prompt

Then explain:

### Expected AI Output

What type of answer/code should the engineer expect?

### Human Review

What assumptions should the engineer verify before accepting the AI output?

## 6. Step-by-Step Implementation

Use numbered steps.

Each step should include:

- WHERE the action happens:
  - Unreal Editor
  - Visual Studio
  - VS Code
  - Git Bash
  - external generation tool
  - website/service
- WHAT the developer does
- WHY it is required
- WHAT they should expect to see

Avoid vague instructions such as:
"Set everything up."

Prefer:

1. Open Edit → Plugins.
2. Search for ...
3. Enable ...
4. Restart ...
5. Verify ...

When code is important, include only the relevant verified snippets.

Do not duplicate thousands of generated values.

## 7. AI Prompts by Stage

Break the implementation into logical stages.

For each meaningful stage provide a short reusable AI prompt.

Example:

### Prompt A — Inspect Existing Architecture
...

### Prompt B — Implement the Core System
...

### Prompt C — Add Validation
...

### Prompt D — Debug the Result
...

### Prompt E — Refactor After Verification
...

These prompts should teach the learner how to use AI productively instead
of merely giving them finished code.

## 8. Common Mistakes

Use actual problems from development first.

For each issue:

### Symptom
What the developer sees.

### Likely Cause
Why it happened.

### Fix
Exact correction.

### Lesson
General engineering lesson.

Do not invent fake bugs merely to make the section longer.

## 9. Verification

Give explicit acceptance tests.

Use a table:

| Test | Action | Expected Result |
|------|--------|-----------------|

Clearly distinguish:
- tests already performed,
- tests still pending,
- editor-only verification,
- packaged-build verification.

Never mark a test passed unless evidence exists.

## 10. Known Limitations

Explain what this milestone intentionally does NOT solve.

Examples:
- placeholder meshes
- no runtime switching
- no multiplayer support yet
- no navmesh integration yet
- simple collision
- limited visual polish
- no performance test yet

## 11. Extension Ideas

Explain how this architecture could be adapted to other games.

Examples:
- dungeon rooms
- racing-track boundaries
- RTS territory generation
- arena walls
- survival-biome population
- roguelike level generation

Keep this conceptual rather than implementing the next milestone.

## 12. Source-Control Checkpoint

Include:
- Jira ticket placeholder
- branch naming suggestion
- changed files
- staging commands
- commit message
- PR title suggestion
- verification checklist

---

# PHASE 4 — Write the Instructional Video Script

Create a separate recording script for an instructor.

The style should follow the structure used in the existing Programming Sprint
transcripts.

The script should teach the PROCESS and engineering reasoning, not merely
describe the finished source code.

Use this structure.

# VIDEO SCRIPT — [Milestone Name]

## Opening

Write natural spoken narration that explains:
- where the learner is starting,
- what result will be built,
- and why this system matters.

Do not assume they are reproducing GrantMoney exactly.

Frame GrantMoney as one example implementation of a reusable method.

Example philosophy:

"Today we are going to use generated map data to automatically construct
world boundaries. The exact map, art style, and game do not matter. The
important idea is separating generated data from the Unreal system that
consumes it."

## Acceptance Criteria

Give a spoken checklist:

"At the end of this part..."

Include observable results.

## Engineering Principle

Introduce one memorable principle related to the milestone.

Examples:
- Generated data should be adapted at the boundary.
- Automate repetitive work, but verify what the automation changes.
- Separate data generation from world presentation.
- Prove the smallest useful path before adding complexity.
- Prefer reusable interfaces over one-off hardcoded implementations.
- Build systems around stable contracts, not temporary filenames.

Explain the principle briefly.

## Prerequisites

Tell the learner what must already exist before following this video.

## Step-by-Step Demonstration

For each step provide:

### SAY
What the instructor should explain.

### SHOW
Exactly what should appear on screen.

### DO
Exact actions/clicks/commands.

### AI PROMPT
The prompt the instructor should paste into ChatGPT/Copilot to produce or
assist with that stage.

### VERIFY
What the instructor should check before moving on.

The video should show the AI-assisted workflow rather than pretending all
code was handwritten.

The instructor should visibly:
1. explain the problem,
2. ask AI for a bounded solution,
3. inspect the output,
4. apply the output,
5. compile/run,
6. identify problems if any,
7. ask AI for debugging help when appropriate,
8. verify the corrected result.

## Engineering Discussion

At important architectural moments, include a short section beginning with:

"Here is an engineering discussion."

Explain:
- why this approach was chosen,
- what alternative would be tempting,
- and why the chosen architecture is more reusable.

## Common Mistakes

Use the style:

"Here are some common mistakes."

Then explain realistic errors and corrections.

Prefer actual milestone mistakes.

## Verification Checkpoint

Use spoken wording like:

"Now let's verify our result."

Then show:
- compiler result,
- Unreal result,
- Output Log,
- settings,
- behavior,
- optional packaged build.

## AI Review

Include a short spoken section:

"Let's review what AI actually did here."

Explain:
- what AI generated,
- what humans changed,
- what failed,
- how output was verified.

## Engineering Pause

Near the end, include a question in the style:

"Here is an engineering pause. Consider this question..."

Ask a generalizable question about the architecture.

Example:

"If this system only works for the first map we tested, did we actually build
a reusable system—or just automate one hardcoded case?"

Then answer it briefly.

## Completion Criteria

Finish with:

"This milestone is complete when..."

Give a concise checklist.

## Next Milestone

Explain what the architecture makes possible next, without implementing it.

---

# PHASE 5 — Make the Script Generalizable

This is critical.

The final script must distinguish between:

## Project-Specific Example

"This GrantMoney implementation uses TileGen and a 64x64 map."

and:

## Reusable Engineering Method

"The broader pattern is: generated data → stable interface → Unreal runtime
system → verified output."

Do not teach learners that:
- their class must be named exactly like ours,
- their map must be 64x64 unless required by the method,
- their material must look like ours,
- their game must be a survivor game,
- or their generator must be TileGen.

Teach them:
- how to inspect generated output,
- define stable interfaces,
- ask AI for bounded code,
- verify AI assumptions,
- structure Unreal systems,
- debug failures,
- test from a clean baseline,
- and preserve reproducibility.

Use GrantMoney as the worked example.

---

# PHASE 6 — Produce Three Final Outputs

At the end, provide:

## OUTPUT A — Milestone Change Report

Short, practical, Git-oriented.

## OUTPUT B — Developer Implementation Guide

Detailed Markdown field manual.

Suggested filename:

docs/course/[module]/[sprint]/[milestone-name].md

## OUTPUT C — Instructor Recording Script

Detailed spoken script with:
- SAY
- SHOW
- DO
- AI PROMPT
- VERIFY
- Engineering Discussion
- Common Mistakes
- Verification
- Engineering Pause

Suggested filename:

docs/course/[module]/[sprint]/scripts/[milestone-name]-script.md

Also suggest screenshot filenames that should be captured during the
implementation.

---

# FINAL QUALITY RULES

Before finishing, verify that:

- Documentation matches the actual code.
- File names come from the repository, not assumptions.
- AI prompts are reusable.
- No unperformed tests are described as successful.
- Common mistakes are grounded in real or credible observed behavior.
- The course teaches a method, not just GrantMoney.
- The instructor script follows the existing sprint transcript style.
- Important engineering reasoning is included.
- Exact steps are reproducible.
- The changelog clearly identifies every touched file.
- Git staging commands include only relevant milestone files.
- Known limitations are explicit.
- The next milestone is not accidentally implemented.# Post-Milestone Documentation and Course Packaging Task

You are working on an AI-native game development project whose purpose is not
only to produce a working game, but to create a reproducible instructional
process that another developer can follow to build a similar result in their
own game.

The milestone that was just completed is:

[MILESTONE NAME]

Examples:
- Procedural Map Boundary Generator
- Dynamic Multi-Map Registration
- TileGen World Renderer
- Procedural Environment Population
- Third-Person Camera Integration
- Player Character Movement
- Enemy Spawning System
- Collectible / Reward System
- Save Game System
- AI Asset Import Pipeline

The intended learner is a software engineer who may know programming but may
not know this project, Unreal Engine, the exact tool, or this specific game.

The instructional goal is NOT:
"Copy GrantMoney exactly."

The instructional goal IS:
"Show a repeatable AI-assisted engineering process that can be adapted to
other games."

This project uses AI as an engineering collaborator. Every result must clearly
separate:
- what AI proposed,
- what the engineer decided,
- what had to be corrected,
- what was actually tested,
- and what remains unverified.

Do not invent tests, files, prompts, bugs, or results that did not occur.

---

# PHASE 1 — Inspect the Current State

Before writing documentation, inspect the repository and determine what
actually changed during this milestone.

Do not assume filenames from this prompt.

Inspect:
- git status
- git diff
- git diff --stat
- relevant C++ source/header files
- Unreal configuration/assets when visible to you
- existing task documentation
- related architecture files
- any AI-generated code produced for this milestone
- any test/log evidence available

Determine:

1. What the system did BEFORE this milestone.
2. What capability exists AFTER this milestone.
3. Which files were created.
4. Which files were modified.
5. Which files were intentionally NOT changed.
6. Whether generated files or binary Unreal assets were touched.
7. Whether any files changed only because of formatting, line endings,
   generated state, or unrelated editor activity.
8. What tests were actually run.
9. What tests still need human verification.
10. What problems were encountered and how they were corrected.

Do not modify unrelated files during this task.

---

# PHASE 2 — Produce a Milestone Change Report

Create a concise changelog with the following sections.

## Milestone Summary

Explain in plain language:
- what was added,
- what problem it solves,
- and why it matters architecturally.

## Files Created

Use a table:

| File | Purpose |
|------|---------|
| path/to/file | What the new file does |

## Files Modified

Use a table:

| File | What Changed | Why |
|------|--------------|-----|
| path/to/file | Specific change | Reason |

## Files Not Changed

Explicitly call out important systems that were intentionally preserved.

Examples:
- TileGen generated arrays
- MapArrays interface
- PlayGame.umap
- M_Tile
- camera settings
- imported textures
- packaging config

This section is important because the developer needs to know what they
should and should NOT stage in Git.

## Git Staging Recommendation

Give exact `git add` commands for only the relevant changed files.

Example format:

git add Game/Source/GrantMoney/Public/SomeFile.h
git add Game/Source/GrantMoney/Private/SomeFile.cpp
git add docs/course/...

Also include:

git status
git diff --cached --stat
git diff --cached

Recommend a commit message in this format:

GMS-XX: short milestone description

Do not commit, push, merge, or stage automatically.

---

# PHASE 3 — Write the Developer Documentation

Create a detailed Markdown implementation guide for this milestone.

The guide should be written so that another developer could reproduce the
same RESULT, even if:
- their game has a different name,
- their art is different,
- their map is different,
- their directory structure differs slightly,
- their level names differ,
- or they use the same architectural idea for a different genre.

Do not make the guide unnecessarily GrantMoney-specific.

Use GrantMoney as the implementation example, but explain the general
principle behind every project-specific action.

Use this structure:

# [Milestone Name]

## 1. Objective

Explain what the developer will have at the end.

## 2. Why This System Exists

Explain the engineering problem being solved.

Discuss:
- why the previous state was insufficient,
- why this architecture was chosen,
- and what alternatives were rejected or deferred.

## 3. Prerequisites

List everything that must already exist.

Examples:
- required Unreal version
- existing C++ project
- prior system/interface
- required assets
- required plugins
- required map/data
- successful previous milestone

## 4. Architecture Overview

Explain the data/control flow.

Include an ASCII diagram when useful.

Example:

Generator Output
    ↓
Stable Data Interface
    ↓
Runtime System
    ↓
Unreal Components
    ↓
Visible / Playable Result

Explain responsibility boundaries.

## 5. AI-Assisted Planning

Include the actual or reconstructed prompt that should be given to AI to
produce the implementation.

The prompt must:
- tell AI to inspect existing code first,
- preserve working systems,
- avoid inventing filenames/symbols,
- use actual repository state,
- keep the scope narrow,
- explain test criteria,
- and not claim unperformed tests succeeded.

Label this clearly:

### AI Prompt

Then explain:

### Expected AI Output

What type of answer/code should the engineer expect?

### Human Review

What assumptions should the engineer verify before accepting the AI output?

## 6. Step-by-Step Implementation

Use numbered steps.

Each step should include:

- WHERE the action happens:
  - Unreal Editor
  - Visual Studio
  - VS Code
  - Git Bash
  - external generation tool
  - website/service
- WHAT the developer does
- WHY it is required
- WHAT they should expect to see

Avoid vague instructions such as:
"Set everything up."

Prefer:

1. Open Edit → Plugins.
2. Search for ...
3. Enable ...
4. Restart ...
5. Verify ...

When code is important, include only the relevant verified snippets.

Do not duplicate thousands of generated values.

## 7. AI Prompts by Stage

Break the implementation into logical stages.

For each meaningful stage provide a short reusable AI prompt.

Example:

### Prompt A — Inspect Existing Architecture
...

### Prompt B — Implement the Core System
...

### Prompt C — Add Validation
...

### Prompt D — Debug the Result
...

### Prompt E — Refactor After Verification
...

These prompts should teach the learner how to use AI productively instead
of merely giving them finished code.

## 8. Common Mistakes

Use actual problems from development first.

For each issue:

### Symptom
What the developer sees.

### Likely Cause
Why it happened.

### Fix
Exact correction.

### Lesson
General engineering lesson.

Do not invent fake bugs merely to make the section longer.

## 9. Verification

Give explicit acceptance tests.

Use a table:

| Test | Action | Expected Result |
|------|--------|-----------------|

Clearly distinguish:
- tests already performed,
- tests still pending,
- editor-only verification,
- packaged-build verification.

Never mark a test passed unless evidence exists.

## 10. Known Limitations

Explain what this milestone intentionally does NOT solve.

Examples:
- placeholder meshes
- no runtime switching
- no multiplayer support yet
- no navmesh integration yet
- simple collision
- limited visual polish
- no performance test yet

## 11. Extension Ideas

Explain how this architecture could be adapted to other games.

Examples:
- dungeon rooms
- racing-track boundaries
- RTS territory generation
- arena walls
- survival-biome population
- roguelike level generation

Keep this conceptual rather than implementing the next milestone.

## 12. Source-Control Checkpoint

Include:
- Jira ticket placeholder
- branch naming suggestion
- changed files
- staging commands
- commit message
- PR title suggestion
- verification checklist

---

# PHASE 4 — Write the Instructional Video Script

Create a separate recording script for an instructor.

The style should follow the structure used in the existing Programming Sprint
transcripts.

The script should teach the PROCESS and engineering reasoning, not merely
describe the finished source code.

Use this structure.

# VIDEO SCRIPT — [Milestone Name]

## Opening

Write natural spoken narration that explains:
- where the learner is starting,
- what result will be built,
- and why this system matters.

Do not assume they are reproducing GrantMoney exactly.

Frame GrantMoney as one example implementation of a reusable method.

Example philosophy:

"Today we are going to use generated map data to automatically construct
world boundaries. The exact map, art style, and game do not matter. The
important idea is separating generated data from the Unreal system that
consumes it."

## Acceptance Criteria

Give a spoken checklist:

"At the end of this part..."

Include observable results.

## Engineering Principle

Introduce one memorable principle related to the milestone.

Examples:
- Generated data should be adapted at the boundary.
- Automate repetitive work, but verify what the automation changes.
- Separate data generation from world presentation.
- Prove the smallest useful path before adding complexity.
- Prefer reusable interfaces over one-off hardcoded implementations.
- Build systems around stable contracts, not temporary filenames.

Explain the principle briefly.

## Prerequisites

Tell the learner what must already exist before following this video.

## Step-by-Step Demonstration

For each step provide:

### SAY
What the instructor should explain.

### SHOW
Exactly what should appear on screen.

### DO
Exact actions/clicks/commands.

### AI PROMPT
The prompt the instructor should paste into ChatGPT/Copilot to produce or
assist with that stage.

### VERIFY
What the instructor should check before moving on.

The video should show the AI-assisted workflow rather than pretending all
code was handwritten.

The instructor should visibly:
1. explain the problem,
2. ask AI for a bounded solution,
3. inspect the output,
4. apply the output,
5. compile/run,
6. identify problems if any,
7. ask AI for debugging help when appropriate,
8. verify the corrected result.

## Engineering Discussion

At important architectural moments, include a short section beginning with:

"Here is an engineering discussion."

Explain:
- why this approach was chosen,
- what alternative would be tempting,
- and why the chosen architecture is more reusable.

## Common Mistakes

Use the style:

"Here are some common mistakes."

Then explain realistic errors and corrections.

Prefer actual milestone mistakes.

## Verification Checkpoint

Use spoken wording like:

"Now let's verify our result."

Then show:
- compiler result,
- Unreal result,
- Output Log,
- settings,
- behavior,
- optional packaged build.

## AI Review

Include a short spoken section:

"Let's review what AI actually did here."

Explain:
- what AI generated,
- what humans changed,
- what failed,
- how output was verified.

## Engineering Pause

Near the end, include a question in the style:

"Here is an engineering pause. Consider this question..."

Ask a generalizable question about the architecture.

Example:

"If this system only works for the first map we tested, did we actually build
a reusable system—or just automate one hardcoded case?"

Then answer it briefly.

## Completion Criteria

Finish with:

"This milestone is complete when..."

Give a concise checklist.

## Next Milestone

Explain what the architecture makes possible next, without implementing it.

---

# PHASE 5 — Make the Script Generalizable

This is critical.

The final script must distinguish between:

## Project-Specific Example

"This GrantMoney implementation uses TileGen and a 64x64 map."

and:

## Reusable Engineering Method

"The broader pattern is: generated data → stable interface → Unreal runtime
system → verified output."

Do not teach learners that:
- their class must be named exactly like ours,
- their map must be 64x64 unless required by the method,
- their material must look like ours,
- their game must be a survivor game,
- or their generator must be TileGen.

Teach them:
- how to inspect generated output,
- define stable interfaces,
- ask AI for bounded code,
- verify AI assumptions,
- structure Unreal systems,
- debug failures,
- test from a clean baseline,
- and preserve reproducibility.

Use GrantMoney as the worked example.

---

# PHASE 6 — Produce Three Final Outputs

At the end, provide:

## OUTPUT A — Milestone Change Report

Short, practical, Git-oriented.

## OUTPUT B — Developer Implementation Guide

Detailed Markdown field manual.

Suggested filename:

docs/course/[module]/[sprint]/[milestone-name].md

## OUTPUT C — Instructor Recording Script

Detailed spoken script with:
- SAY
- SHOW
- DO
- AI PROMPT
- VERIFY
- Engineering Discussion
- Common Mistakes
- Verification
- Engineering Pause

Suggested filename:

docs/course/[module]/[sprint]/scripts/[milestone-name]-script.md

Also suggest screenshot filenames that should be captured during the
implementation.

---

# FINAL QUALITY RULES

Before finishing, verify that:

- Documentation matches the actual code.
- File names come from the repository, not assumptions.
- AI prompts are reusable.
- No unperformed tests are described as successful.
- Common mistakes are grounded in real or credible observed behavior.
- The course teaches a method, not just GrantMoney.
- The instructor script follows the existing sprint transcript style.
- Important engineering reasoning is included.
- Exact steps are reproducible.
- The changelog clearly identifies every touched file.
- Git staging commands include only relevant milestone files.
- Known limitations are explicit.
- The next milestone is not accidentally implemented.