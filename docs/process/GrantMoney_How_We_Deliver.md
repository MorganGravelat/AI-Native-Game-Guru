# Grant Money — How We Deliver Work

## Purpose

This document explains how Team A works on **Grant Money** from the moment a Jira ticket is selected until the work is reviewed, merged, documented, and ready to teach.

We are not only making the game work. We are also building a **reproducible course process**. Another person should eventually be able to follow our instructions, use the same prompts and files, and reproduce the same result.

Repository:

`https://github.com/MorganGravelat/AI-Native-Game-Guru.git`

Jira project key:

`GMS`

---

## Team Roles

| Team Member | Primary Role | Main Responsibility |
|---|---|---|
| Morgan | Technical Lead / Scrum Lead | Architecture, Jira/Scrum, technical decisions, review support |
| Austin | Gameplay Engineer | Player, movement, combat, enemies, survivor-game systems |
| Rohaan | Tools and Assets Engineer | AI asset generation, TileGen, import pipelines, models, textures, tooling |
| Kai | Course Production / Documentation | Sprint guides, scripts, screenshots, formatting, final documentation assembly |
| Julio | QA / Build / Release | Testing, packaged builds, build verification, release readiness |

These are primary ownership areas, not hard walls. Everyone should still understand and test the project.

---

# 1. The Main Rule

Every technical Jira ticket that teaches something a future learner would need to repeat should have **one task document**.

Write that document **while you work**, not several days later.

The task document should travel with the code or assets in the same branch and Pull Request.

```text
Jira Ticket
    ↓
Do the work
    ↓
Document it while doing it
    ↓
Save screenshots and AI prompts
    ↓
Test it
    ↓
Open Pull Request
    ↓
Teammate reviews the work and documentation
    ↓
Merge
    ↓
Kai assembles approved task docs into the Programming Sprint guide
```

---

# 2. What Needs a Task Document?

Create a task document when a learner would need to repeat the technical work.

Examples:

- C++ gameplay code
- Unreal systems
- TileGen setup/import
- AI-generated models
- Blender cleanup
- animation import
- materials
- UI implementation
- packaging
- build setup
- AI-assisted asset pipelines
- technical spikes/prototypes that affect a decision

A task document is usually **not** needed for routine management work such as:

- emails
- scheduling meetings
- Jira cleanup
- sponsor reminders
- routine Scrum administration

Simple rule:

> If a future learner would need to repeat what you did, document it.

---

# 3. Recommended Repository Structure

```text
AI-Native-Game-Guru/
│
├── Game/
│   ├── Config/
│   ├── Content/
│   ├── Source/
│   ├── Plugins/
│   └── GrantMoney.uproject
│
├── Prototypes/
│   └── <Developer>-<Topic>/
│       ├── README.md
│       ├── images/
│       └── useful prototype source files
│
├── docs/
│   ├── process/
│   │   ├── HOW_WE_DELIVER.md
│   │   └── TECH_STACK.md
│   │
│   ├── course/
│   │   ├── _template/
│   │   │   └── task-doc.md
│   │   ├── Module1-BuildingAWorld/
│   │   ├── Module2-BringingTheWorldToLife/
│   │   ├── Module3-CreatingChallengeAndReward/
│   │   └── Module4-FromPrototypeToProduct/
│   │
│   ├── ai/
│   │   └── AIWorkflowLog.md
│   │
│   └── tools/
│       └── TileGen/
│           └── README.md
│
├── .gitignore
├── .gitattributes
└── README.md
```

### `Game/`

The integrated Unreal project.

### `Prototypes/`

Experimental work and spikes. Do not dump an entire generated Unreal project here.

### `docs/course/`

Programming Sprint documentation and module material.

### `docs/ai/`

A lightweight index of meaningful AI use.

### `docs/tools/TileGen/`

Put the TileGen end-user guide here.

---

# 4. Jira Workflow

Use this simple status flow:

```text
Backlog
    ↓
To Do
    ↓
In Progress
    ↓
In Review
    ↓
Testing
    ↓
Done
```

### Backlog
Future work that is not part of the current sprint.

### To Do
Selected for the current sprint but not started.

### In Progress
Someone is actively working on it.

### In Review
Work is ready for a teammate to review. A Pull Request should normally be open.

### Testing
Review is complete and the result is being verified.

### Done
The Pull Request is merged and acceptance criteria are satisfied.

Do not mark a ticket Done simply because code was written.

---

# 5. Jira Work Types

### Epic
A large project area.

Example:

`Module 1 — Building a World`

### Story
A meaningful result.

Example:

`Generated map data renders correctly in Unreal`

### Task
Supporting technical work.

Example:

`Package and verify the Sprint 1 build`

### Subtask
A smaller part of a Story or Task.

### Bug
Something that should work but does not.

Example:

`Packaged build launches without tile textures`

---

# 6. Starting a Ticket

Example ticket:

`GMS-36 — TileGen import workflow is reproducible`

Before you work:

1. Read the description and acceptance criteria.
2. Ask questions if the expected result is unclear.
3. Move Jira to **In Progress**.
4. Update your local repo.
5. Create a branch for the ticket.
6. Create the task document.
7. Document as you work.

---

# 7. Update Your Local Repo

Open Git Bash or a terminal in your local repo:

```bash
git checkout main
git pull
git lfs pull
git status
```

You want a clean working tree before starting.

---

# 8. Branch Naming

Use one branch per Jira ticket:

```text
GMS-<ticket>-<short-description>
```

Example:

```bash
git checkout -b GMS-36-tilegen-import
```

Other examples:

```text
GMS-41-map-renderer
GMS-52-player-movement
GMS-77-enemy-spawner
```

Do not do normal development directly on `main`.

---

# 9. Commit Naming

Start commit messages with the Jira ticket.

Examples:

```bash
git commit -m "GMS-36: add repeatable TileGen import workflow"
```

```bash
git commit -m "GMS-36: document texture verification steps"
```

This links Jira, Git history, Pull Requests, and documentation.

---

# 10. Sandbox Workflow

Developers may use their own Unreal sandbox for experiments.

Use a sandbox when:

- testing unfamiliar C++;
- testing AI-generated code;
- testing a plugin;
- trying an asset pipeline;
- doing a risky spike.

Workflow:

```text
Personal Sandbox
    ↓
Build and test
    ↓
Document what worked
    ↓
Copy useful source/assets into the repo branch
    ↓
Integrate/test in the shared Game project when required
```

Important:

> The sandbox is not the final product.

A ticket is not finished just because something worked in one person's private project.

---

# 11. Task Document Location

Copy:

```text
docs/course/_template/task-doc.md
```

For prototype work:

```text
Prototypes/Austin-PlayerMovement/README.md
```

For direct Programming Sprint material:

```text
docs/course/Module1-BuildingAWorld/Sprint1/TileGenImport.md
```

The exact folder can vary, but keep the team consistent.

---

# 12. What Goes in a Task Document?

Use this order.

## What I Built
Explain the result in 2–3 sentences. Add a final screenshot when useful.

## Why We Built It
Explain the problem, chosen approach, alternatives, and why decisions were made.

## Prerequisites
List what must already exist.

## Steps
Use numbered steps. One main action per step.

Bad:

> Configure everything and import it.

Better:

> 1. Open Edit → Plugins.
> 2. Search for Python Editor Script Plugin.
> 3. Enable it.
> 4. Restart Unreal.

## AI Use
Put AI details inside the step where AI was used.

Example:

```text
AI Used

Tool:
ChatGPT

Prompt:
"In Unreal Engine 5.8.1 C++, how can I expose a generated 64x64
tile array to a renderer without making the renderer depend on
generated filenames?"

Result:
Suggested a small map-data interface.

Decision:
Modified.

Why:
The first answer used names that did not match TileGen.

Human Verification:
Compiled it, loaded Level 1, and checked sample array values.
```

## Files Changed
List important files and explain what each one does.

## Verify It Works
Explain exactly what another person should click/run and what they should see.

## Common Mistakes
Record real errors and how you fixed them.

## Known Limitations
Say what is intentionally unfinished.

## Source-Control Checkpoint
Record Jira ticket, branch, PR, and relevant commit/tag.

---

# 13. Screenshot Rules

Take screenshots while working.

Use an `images/` folder next to the document:

```text
README.md
images/
    01-open-project.png
    02-enable-python-plugin.png
    03-tilegen-export.png
    04-run-import-script.png
    05-imported-tiles.png
```

Place each screenshot immediately under the step it demonstrates:

```markdown
![Imported tiles in Unreal](images/05-imported-tiles.png)
```

Rules:

- crop to what matters;
- keep important text readable;
- use PNG or JPG;
- use numbered descriptive filenames;
- never include passwords, API keys, or unrelated private information.

---

# 14. AI Logging

For meaningful AI use, record:

```text
Tool
Prompt
Result
Decision
Verification
```

Decision must be:

```text
Accepted
Modified
Rejected
```

Example:

```text
Tool:
GitHub Copilot

Purpose:
Generate the loop that reads a 64x64 TileGen map.

Result:
Generated an X/Y loop.

Decision:
Modified.

Reason:
The first version treated the array indexes in the wrong order.

Verification:
Compared sample coordinates with the TileGen source and confirmed
the corrected map matched the source layout.
```

Do not log every autocomplete suggestion.

Log AI use that materially affects:

- architecture;
- code;
- assets;
- debugging;
- documentation;
- testing;
- technical decisions.

Also add one short row to:

```text
docs/ai/AIWorkflowLog.md
```

that links to the detailed task document.

---

# 15. AI Does Not Replace Verification

AI is central to this project, but every output still needs a human check.

### Code
Read it, compile it, run it, understand it.

### Unreal instructions
Perform them in the actual Unreal version being used.

### Models
Inspect scale, topology, materials, pivot, rigging, collision, and performance.

### Textures
Inspect them in Unreal.

### Audio
Listen to the exported files.

### Documentation
Have another person follow it.

---

# 16. Git LFS

Unreal uses binary assets such as:

```text
.uasset
.umap
```

Git cannot merge these like normal text files.

Use Git LFS for legitimate large binary project assets.

Each developer should run once:

```bash
git lfs install
```

After cloning:

```bash
git lfs pull
```

The repository should contain a `.gitattributes` file that defines which types use LFS.

---

# 17. What Should Never Be Committed?

Do not commit generated Unreal folders:

```text
Binaries/
Intermediate/
Saved/
DerivedDataCache/
```

Do not commit:

- packaged builds;
- ZIPs of the project;
- temporary exports;
- crash dumps;
- passwords;
- API keys;
- local IDE caches;
- video files.

Important game assets such as `.uasset` and `.umap` **do belong in source control** when they are part of the project. Store them with Git LFS rather than ignoring them.

---

# 18. Commit Your Work

First check:

```bash
git status
```

Read what changed.

Do not blindly stage everything until you understand what Git is about to include.

Example:

```bash
git add Source/GrantMoney/Maps/
git add docs/course/Module1-BuildingAWorld/
git commit -m "GMS-36: add repeatable TileGen import workflow"
```

Commit whenever you reach a useful working checkpoint.

---

# 19. Push the Branch

First push:

```bash
git push -u origin GMS-36-tilegen-import
```

Later pushes:

```bash
git push
```

---

# 20. Pull Requests

On GitHub:

1. Open the repo.
2. Choose the branch.
3. Click **Compare & pull request**.
4. Use:
   - base: `main`
   - compare: your branch
5. Title the PR with the Jira ticket.

Example:

```text
GMS-36: Reproducible TileGen import workflow
```

The PR should explain:

- what changed;
- how to test it;
- the Jira ticket;
- task-document location;
- known limitations.

Move Jira to:

```text
In Review
```

---

# 21. Review Rule

Keep it simple:

> Every Pull Request requires at least one teammate who did not author the work.

The reviewer checks:

1. Does it satisfy the Jira ticket?
2. Can the code/assets be understood?
3. Does the task document match what was actually done?
4. Are important AI decisions and verification recorded?
5. Is anything clearly unsafe or missing?
6. Can the important result be reproduced or tested?

Morgan can help with architecture and use AI as a second review aid.

Kai focuses on documentation quality during sprint assembly.

Julio coordinates build/release verification.

---

# 22. Fixing Review Feedback

Keep using the same branch.

Example:

```bash
git add Source/GrantMoney/Maps/MapArrays.cpp
git add docs/course/Module1-BuildingAWorld/
git commit -m "GMS-36: fix map indexing and update verification"
git push
```

The existing Pull Request updates automatically.

---

# 23. Testing

After review, move Jira to:

```text
Testing
```

Run the test described in the task document.

Examples:

- compile;
- launch Unreal;
- press Play;
- inspect Output Log;
- run the Python import;
- inspect textures;
- package Windows;
- reproduce on another machine.

Verify every acceptance criterion.

---

# 24. Merge and Finish

When review, testing, and documentation pass:

1. merge the PR;
2. delete the branch when appropriate;
3. update your local main:

```bash
git checkout main
git pull
git lfs pull
```

4. move Jira to **Done**.

Done means **merged and verified**, not "finished typing code."

---

# 25. Dailybot

Every working day, answer the Dailybot check-in and mention the Jira ticket.

Example:

```text
Yesterday:
Worked on GMS-36. Reviewed the TileGen Python import script.

Today:
Run the import in Unreal and verify texture settings.

Blocked:
None.
```

If blocked, say exactly what is blocking the ticket.

---

# 26. Programming Sprint Delivery

Our Programming Sprints may be shorter than the original two-week planning cadence.

For the current plan, the first three Programming Sprints are expected to fit into roughly the first two weeks.

A sprint is complete when:

```text
[ ] Milestone works
[ ] Jira work is merged
[ ] Task documents are complete
[ ] Meaningful AI use is recorded
[ ] Screenshots are present
[ ] Another teammate reviewed the work
[ ] Verification was performed
[ ] Packaged build is tested when required
[ ] Kai can assemble the material into the Sprint guide
```

Documentation may be as much work as implementation. Plan time for it.

---

# 27. How Task Docs Become the Sprint Guide

Each developer documents their own ticket.

Kai does not rewrite everyone's technical work from scratch.

Kai:

1. collects approved task docs;
2. puts them in teaching order;
3. removes unnecessary duplication;
4. makes terminology consistent;
5. adds the sprint introduction;
6. adds acceptance criteria;
7. checks screenshots;
8. assembles the Markdown guide;
9. creates the PDF;
10. prepares scripts and recording material.

The original technical author is still responsible for technical correctness.

---

# 28. Sprint Output

The team prepares:

```text
Markdown source
PDF
screenshots
code/source files
AI records
scripts
recording material
packaged build when required
```

Recommended storage:

- Markdown/source: GitHub
- final PDF: Google Drive / OneDrive, optionally GitHub Release
- video files: Google Drive / YouTube, not normal Git history
- links to final hosted files may be kept in the repo

The team prepares the course material but does **not** upload it to the public course website.

---

# 29. Module-End Deliverables

Each module includes these four items.

## Studio Retrospective
What worked, what surprised us, what failed, and what should change.

## Engineering Review
The most important architecture/code decision and its tradeoffs.

## AI Workflow Review
Where AI helped, where it failed, what humans changed, and how output was verified.

## Lead Engineer's Insight / Maxim
One memorable engineering rule demonstrated by real project work.

Example:

> Automate repetitive work, but verify the contract between generated data and the system consuming it.

---

# 30. TileGen Example

Assume:

```text
GMS-36 — TileGen workflow is reproducible
```

The developer:

1. moves GMS-36 to In Progress;
2. creates `GMS-36-tilegen-import`;
3. follows the TileGen guide;
4. captures screenshots while generating/exporting/importing;
5. records meaningful ChatGPT/Copilot use;
6. verifies texture settings;
7. verifies generated C++ map data;
8. writes the task doc;
9. commits;
10. pushes;
11. opens PR;
12. moves Jira to In Review;
13. another teammate reviews/reproduces the important part;
14. fixes problems;
15. moves Jira to Testing;
16. verifies final behavior;
17. merges;
18. moves Jira to Done.

Put the detailed TileGen guide at:

```text
docs/tools/TileGen/README.md
```

Task docs should link to it instead of repeating the entire TileGen setup.

---

# 31. Pull Request Checklist

```text
## Jira
Ticket: GMS-___

## What changed
Explain the result.

## How to test
1.
2.
3.

## Documentation
- [ ] Task document included
- [ ] Screenshots included where useful
- [ ] AI use documented where required
- [ ] Important files explained
- [ ] Known limitations documented

## Verification
- [ ] Project compiles
- [ ] Feature works in Unreal
- [ ] Acceptance criteria pass
- [ ] Packaged build tested if required
- [ ] No generated Unreal junk committed
- [ ] No credentials committed
```

---

# 32. Definition of Done

A technical Jira item is Done when:

```text
[ ] Acceptance criteria are satisfied
[ ] Work is in Git
[ ] Task documentation is complete
[ ] Meaningful AI use is documented
[ ] Required screenshots are present
[ ] Another teammate reviewed it
[ ] Required testing passed
[ ] Pull Request is merged
[ ] Jira status is Done
```

For major milestones also confirm:

```text
[ ] Shared Unreal project still builds
[ ] Packaged Windows build works
[ ] Another developer can reproduce the documented result
```

---

# 33. Common Problems

### Repository not found
Check your GitHub account, invite, permissions, and repository URL.

### Push rejected
Do not force-push blindly. Update carefully and ask for help if there is a conflict.

### Huge file appears in Git
Stop before pushing. Check for generated folders, packaged builds, ZIPs, or videos.

### Legitimate Unreal binary is large
Use Git LFS instead of ignoring a real project asset.

### Password or API key was added
Do not push. If already pushed, tell Morgan immediately and revoke the credential.

### AI code compiles but is wrong
Record the failure, fix it, explain the correction, and keep it as useful course material.

---

# 34. Quick Daily Workflow

```text
1. Read Jira ticket
2. git checkout main
3. git pull
4. git lfs pull
5. create GMS branch
6. Jira → In Progress
7. create task doc
8. work in sandbox/shared project as appropriate
9. document steps + screenshots + AI use
10. test
11. git status
12. commit
13. push
14. open PR
15. Jira → In Review
16. teammate reviews
17. fix comments
18. Jira → Testing
19. verify acceptance criteria
20. merge
21. Jira → Done
22. complete Dailybot check-in
```
