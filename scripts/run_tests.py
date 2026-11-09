#!/usr/bin/env python3
"""Build GrantMoneyEditor and run the GrantMoney Automation tests headlessly.

Portable across teammates' machines: nothing here is tied to one computer.
The engine is found from, in order:
  1. the UE_ROOT environment variable (folder that contains "Engine/"),
  2. the .uproject's EngineAssociation looked up in the Windows registry
     (the same lookup Unreal uses when you double-click a .uproject),
  3. the Epic launcher's LauncherInstalled.dat,
  4. the default launcher folder (C:/Program Files/Epic Games/UE_<version>).

Usage:
  python scripts/run_tests.py                     # build + run all GrantMoney.* tests
  python scripts/run_tests.py --filter GrantMoney.Environment
  python scripts/run_tests.py --skip-build

Exit codes: 0 all passed | 1 tests failed or none ran | 2 build failed | 3 setup problem
"""
import argparse
import json
import os
import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
GAME_DIR = REPO_ROOT / "Game"
EDITOR_TARGET = "GrantMoneyEditor"
DEFAULT_FILTER = "GrantMoney"
TEST_TIMEOUT_SECONDS = 1500


class SetupError(Exception):
    """The machine is missing something needed to build or run tests."""


def find_uproject():
    found = sorted(GAME_DIR.glob("*.uproject"))
    if not found:
        raise SetupError(f"No .uproject found in {GAME_DIR}")
    return found[0]


def _valid_engine(root):
    return root is not None and (Path(root) / "Engine").is_dir()


def _from_registry(association):
    try:
        import winreg
    except ImportError:
        return None
    lookups = [
        # Launcher install, e.g. EngineAssociation "5.8"
        (winreg.HKEY_LOCAL_MACHINE, rf"SOFTWARE\EpicGames\Unreal Engine\{association}", "InstalledDirectory"),
        # Source build registered by UnrealVersionSelector, EngineAssociation "{GUID}"
        (winreg.HKEY_CURRENT_USER, r"Software\Epic Games\Unreal Engine\Builds", association),
    ]
    for hive, key, value in lookups:
        for view in (winreg.KEY_WOW64_64KEY, winreg.KEY_WOW64_32KEY):
            try:
                with winreg.OpenKey(hive, key, 0, winreg.KEY_READ | view) as handle:
                    path, _ = winreg.QueryValueEx(handle, value)
                    if _valid_engine(path):
                        return Path(path)
            except OSError:
                continue
    return None


def _from_launcher_manifest(association):
    program_data = os.environ.get("PROGRAMDATA", r"C:\ProgramData")
    manifest = Path(program_data) / "Epic" / "UnrealEngineLauncher" / "LauncherInstalled.dat"
    try:
        data = json.loads(manifest.read_text(encoding="utf-8-sig"))
    except (OSError, ValueError):
        return None
    for item in data.get("InstallationList", []):
        if item.get("AppName") == f"UE_{association}" and _valid_engine(item.get("InstallLocation")):
            return Path(item["InstallLocation"])
    return None


def find_engine(uproject):
    env_root = os.environ.get("UE_ROOT")
    if env_root:
        if _valid_engine(env_root):
            return Path(env_root)
        raise SetupError(f"UE_ROOT is set to '{env_root}', but that folder has no Engine/ subfolder.")

    association = json.loads(uproject.read_text(encoding="utf-8-sig")).get("EngineAssociation", "")
    for finder in (_from_registry, _from_launcher_manifest):
        root = finder(association)
        if root:
            return root
    default = Path(r"C:\Program Files\Epic Games") / f"UE_{association}"
    if _valid_engine(default):
        return default

    raise SetupError(
        f"Could not find Unreal Engine '{association}' on this machine. "
        f"Set the UE_ROOT environment variable to your engine folder "
        f"(the one containing Engine/, e.g. C:\\Program Files\\Epic Games\\UE_{association})."
    )


def _tail(text, lines=40):
    return "\n".join(text.splitlines()[-lines:])


def build_editor(engine, uproject):
    """Returns (ok, message)."""
    build_bat = engine / "Engine" / "Build" / "BatchFiles" / "Build.bat"
    if not build_bat.is_file():
        raise SetupError(f"Build script not found: {build_bat} (building is Windows-only here).")
    cmd = [str(build_bat), EDITOR_TARGET, "Win64", "Development",
           f"-Project={uproject}", "-WaitMutex", "-NoHotReloadFromIDE"]
    result = subprocess.run(cmd, capture_output=True, text=True, errors="replace")
    output = (result.stdout or "") + (result.stderr or "")
    if result.returncode == 0:
        return True, "Build succeeded."
    hint = ""
    if "Live Coding" in output:
        hint = "\nHint: Live Coding is active. Close the Unreal Editor (or disable Live Coding) and run again."
    return False, f"Build of {EDITOR_TARGET} failed:\n{_tail(output)}{hint}"


def _results_from_report(report_dir):
    index = report_dir / "index.json"
    if not index.is_file():
        return None
    try:
        data = json.loads(index.read_text(encoding="utf-8-sig"))
    except ValueError:
        return None
    results = []
    for test in data.get("tests", []):
        errors = [
            entry.get("event", {}).get("message", "")
            for entry in test.get("entries", [])
            if entry.get("event", {}).get("type") == "Error"
        ]
        results.append({
            "name": test.get("fullTestPath") or test.get("testDisplayName", "?"),
            "state": test.get("state", "Unknown"),
            "errors": errors,
        })
    return results


_LOG_RESULT = re.compile(r"Test Completed\. Result=\{(\w+)\}\s+Name=\{([^}]*)\}\s+Path=\{([^}]*)\}")


def _results_from_log(log_path):
    try:
        text = log_path.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return []
    return [{"name": path, "state": state, "errors": []} for state, _name, path in _LOG_RESULT.findall(text)]


def run_tests(engine, uproject, test_filter):
    """Returns (ok, message)."""
    editor_cmd = engine / "Engine" / "Binaries" / "Win64" / "UnrealEditor-Cmd.exe"
    if not editor_cmd.is_file():
        raise SetupError(f"UnrealEditor-Cmd.exe not found at {editor_cmd}")

    report_dir = GAME_DIR / "Saved" / "Automation" / "ClaudeReport"
    log_path = GAME_DIR / "Saved" / "Logs" / "ClaudeAutomation.log"
    report_dir.mkdir(parents=True, exist_ok=True)
    log_path.parent.mkdir(parents=True, exist_ok=True)
    stale = report_dir / "index.json"
    if stale.exists():
        try:
            stale.unlink()
        except OSError:
            pass

    cmd = [str(editor_cmd), str(uproject),
           f"-ExecCmds=Automation RunTests {test_filter}",
           "-TestExit=Automation Test Queue Empty",
           f"-ReportExportPath={report_dir}",
           f"-abslog={log_path}",
           "-unattended", "-nullrhi", "-nosplash", "-nosound", "-nopause"]
    try:
        subprocess.run(cmd, capture_output=True, text=True, errors="replace", timeout=TEST_TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired:
        return False, f"Automation tests timed out after {TEST_TIMEOUT_SECONDS}s. Log: {log_path}"

    results = _results_from_report(report_dir)
    if results is None:
        results = _results_from_log(log_path)
    if not results:
        return False, (f"No Automation tests matching '{test_filter}' ran. "
                       f"Check the editor started correctly. Log: {log_path}")

    failed = [r for r in results if r["state"] not in ("Success", "Passed")]
    lines = [f"{len(results) - len(failed)}/{len(results)} tests passed (filter '{test_filter}')."]
    for r in failed:
        lines.append(f"  FAILED [{r['state']}] {r['name']}")
        lines.extend(f"      {msg}" for msg in r["errors"][:5])
    lines.append(f"Log: {log_path}")
    return not failed, "\n".join(lines)


def run_all(test_filter=DEFAULT_FILTER, skip_build=False):
    """Returns (exit_code, message). Never raises for setup problems."""
    try:
        uproject = find_uproject()
        engine = find_engine(uproject)
        if not skip_build:
            ok, message = build_editor(engine, uproject)
            if not ok:
                return 2, message
        ok, message = run_tests(engine, uproject, test_filter)
        return (0 if ok else 1), message
    except SetupError as err:
        return 3, f"Test setup problem: {err}"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--filter", default=DEFAULT_FILTER, help="Automation test name prefix (default: GrantMoney)")
    parser.add_argument("--skip-build", action="store_true", help="Run tests without rebuilding the editor target")
    args = parser.parse_args()
    code, message = run_all(args.filter, args.skip_build)
    print(message)
    return code


if __name__ == "__main__":
    sys.exit(main())
