#!/usr/bin/env python3
"""Claude Code Stop hook: keeps Claude from finishing while C++ work is untested.

Wired up in .claude/settings.json. Runs whenever Claude tries to end its turn:
  * No uncommitted changes under Game/Source        -> allow (no cost).
  * Same source state already passed tests          -> allow (no rebuild).
  * A new .cpp has no matching test file            -> block, tell Claude which.
  * Otherwise build + run GrantMoney.* tests        -> allow on pass, block on fail.

"Block" = exit code 2 with the reason on stderr; Claude Code feeds that back
to Claude, which must fix the problem before stopping.

A new Foo.cpp counts as tested when a FooTests.cpp, FooTest.cpp or FooSpec.cpp
exists anywhere under Game/Source, or when Foo.cpp contains a line
"// NO-TEST: <reason>" (for code with no testable logic).

To avoid endless loops, the same unchanged state is blocked at most
MAX_BLOCKS times; after that the hook allows the stop and warns the user.
"""
import hashlib
import json
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import run_tests  # noqa: E402

REPO_ROOT = run_tests.REPO_ROOT
SOURCE_DIR = "Game/Source"
STATE_DIR = REPO_ROOT / "Game" / "Saved" / "ClaudeTestGate"  # Saved/ is gitignored
MAX_BLOCKS = 3
SOURCE_SUFFIXES = {".h", ".hpp", ".cpp", ".inl", ".cs"}
TEST_SUFFIXES = ("Tests", "Test", "Spec")
EXEMPT_FILES = {"GrantMoney.cpp"}  # module boilerplate
NO_TEST_MARKER = "NO-TEST:"


def git(*args):
    return subprocess.run(["git", *args], cwd=REPO_ROOT, capture_output=True).stdout


def changed_source_files():
    """[(status, path)] for uncommitted changes under Game/Source."""
    raw = git("status", "--porcelain", "-z", "--untracked-files=all", "--", SOURCE_DIR)
    entries = raw.decode("utf-8", "replace").split("\0")
    changes, i = [], 0
    while i < len(entries):
        entry = entries[i]
        i += 1
        if len(entry) < 4:
            continue
        status, path = entry[:2], entry[3:]
        if status[0] in "RC":  # rename/copy: next entry is the old path
            i += 1
        if Path(path).suffix.lower() in SOURCE_SUFFIXES:
            changes.append((status, path))
    return changes


def missing_tests(changes):
    test_stems = {p.stem for p in (REPO_ROOT / SOURCE_DIR).rglob("*.cpp")}
    missing = []
    for status, path in changes:
        p = Path(path)
        is_new = status == "??" or "A" in status
        if not is_new or p.suffix.lower() != ".cpp" or p.name in EXEMPT_FILES:
            continue
        if p.stem.endswith(TEST_SUFFIXES):
            continue
        try:
            text = (REPO_ROOT / p).read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue  # deleted after being staged
        if NO_TEST_MARKER in text:
            continue
        if not any(f"{p.stem}{suffix}" in test_stems for suffix in TEST_SUFFIXES):
            missing.append(path)
    return missing


def source_state_hash(changes):
    digest = hashlib.sha256(git("diff", "HEAD", "--binary", "--", SOURCE_DIR))
    for status, path in sorted(changes, key=lambda c: c[1]):
        if status == "??":
            digest.update(path.encode())
            try:
                digest.update((REPO_ROOT / path).read_bytes())
            except OSError:
                pass
    return digest.hexdigest()


def _read(name):
    try:
        return (STATE_DIR / name).read_text().strip()
    except OSError:
        return ""


def _write(name, value):
    try:
        STATE_DIR.mkdir(parents=True, exist_ok=True)
        (STATE_DIR / name).write_text(value)
    except OSError:
        pass


def block(state, reason):
    record = _read("blocks").split()
    count = int(record[1]) + 1 if len(record) == 2 and record[0] == state else 1
    _write("blocks", f"{state} {count}")
    if count > MAX_BLOCKS:
        print(json.dumps({"systemMessage":
            f"Test gate gave up after {MAX_BLOCKS} attempts; the C++ changes are NOT verified.\n{reason}"}))
        return 0
    sys.stderr.write(
        "SCRUM test gate: you cannot finish yet.\n"
        f"{reason}\n"
        "Fix this, or if it cannot be fixed here, tell the user exactly what is untested "
        "and why. Never claim tests passed unless this gate reported a pass.\n")
    return 2


def main():
    try:
        hook_input = json.loads(sys.stdin.read() or "{}")
    except ValueError:
        hook_input = {}

    changes = changed_source_files()
    if not changes:
        return 0

    state = source_state_hash(changes)
    if _read("last_pass") == state:
        return 0

    missing = missing_tests(changes)
    if missing:
        names = "\n".join(f"  - {m}" for m in missing)
        return block(state, "These new source files have no matching test file "
                            "(<Name>Tests.cpp, see CLAUDE.md 'Testing Rules'):\n" + names)

    code, message = run_tests.run_all()
    if code == 0:
        _write("last_pass", state)
        print(json.dumps({"systemMessage": "SCRUM test gate: " + message.splitlines()[0]}))
        return 0
    if code == 3 and hook_input.get("stop_hook_active"):
        # Machine can't run tests; Claude was already told once. Don't loop.
        print(json.dumps({"systemMessage": message + "\nC++ changes were NOT tested on this machine."}))
        return 0
    return block(state, message)


if __name__ == "__main__":
    sys.exit(main())
