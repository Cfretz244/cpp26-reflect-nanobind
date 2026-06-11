#!/usr/bin/env python3
"""Gate 6b: API-surface differential between the two binder backends.

The constexpr-built module (binding/build/) and the emit-built module
(binding/build-emit/) share one module name, so they can never coexist in one
interpreter: `dump` mode imports ONE module under its lane's environment and
prints a JSON description of its recursive surface; `compare` mode (what
run_gates.py calls) spawns two dump subprocesses, deep-diffs the JSON, and
exits nonzero on any mismatch.

The description captures what the binder synthesizes: classes/enums and their
attribute kinds, property read/write-ness, base wiring (__bases__ names), enum
member tables, and the __doc__ of every callable -- nanobind renders the full
overload signatures into docstrings, so a type-level divergence that still
compiled (a mis-spelled parameter type, a missing overload) shows up here.

usage:
  surface_diff.py dump <build_dir> <module_name> [--toolchain-dyld]
  surface_diff.py compare <run_dir> <module_name> [--ignore REGEX ...]
"""
import json
import os
import pathlib
import re
import subprocess
import sys

SYNTH_DUNDERS = {
    "__init__", "__eq__", "__ne__", "__lt__", "__gt__", "__le__", "__ge__",
    "__add__", "__radd__", "__sub__", "__rsub__", "__mul__", "__rmul__",
    "__truediv__", "__rtruediv__", "__mod__", "__rmod__", "__xor__",
    "__rxor__", "__and__", "__rand__", "__or__", "__ror__", "__lshift__",
    "__rlshift__", "__rshift__", "__rrshift__", "__iadd__", "__isub__",
    "__imul__", "__itruediv__", "__imod__", "__ixor__", "__iand__",
    "__ior__", "__ilshift__", "__irshift__", "__neg__", "__pos__",
    "__invert__", "__call__", "__getitem__", "__bool__", "__int__",
    "__float__", "__str__", "__hash__", "__iter__",
}


def _interesting(name):
    if name.startswith("__") and name.endswith("__"):
        return name in SYNTH_DUNDERS
    return True


def _describe_attr(attr):
    info = {"kind": type(attr).__name__}
    doc = getattr(attr, "__doc__", None)
    if callable(attr) or info["kind"] == "property":
        info["doc"] = doc
    if info["kind"] == "property":
        info["writable"] = attr.fset is not None
    if isinstance(attr, (int, float, str, bool)):
        info["value"] = attr
    try:
        info["int"] = int(attr)  # nanobind enum members
    except (TypeError, ValueError):
        pass
    return info


def _describe_class(name, cls, seen):
    if name in seen:
        return {"recursive": True}
    seen = seen | {name}
    desc = {
        "bases": [b.__name__ for b in cls.__bases__],
        "doc": cls.__doc__,
        "attrs": {},
    }
    for aname, attr in sorted(cls.__dict__.items()):
        if not _interesting(aname) or aname == "__doc__":
            continue
        if isinstance(attr, type):
            desc["attrs"][aname] = _describe_class(aname, attr, seen)
        else:
            desc["attrs"][aname] = _describe_attr(attr)
    return desc


def describe_module(mod):
    out = {}
    for name in sorted(dir(mod)):
        if name.startswith("__"):
            continue
        attr = getattr(mod, name)
        if isinstance(attr, type):
            out[name] = _describe_class(name, attr, frozenset())
        else:
            out[name] = _describe_attr(attr)
    return out


def dump(build_dir, module_name):
    sys.path.insert(0, build_dir)
    mod = __import__(module_name)
    json.dump(describe_module(mod), sys.stdout, indent=1, sort_keys=True)
    print()


def _diff(a, b, path, out, ignore):
    if any(rx.search(path) for rx in ignore):
        return
    if isinstance(a, dict) and isinstance(b, dict):
        for k in sorted(set(a) | set(b)):
            p = f"{path}.{k}"
            if any(rx.search(p) for rx in ignore):
                continue
            if k not in a:
                out.append(f"{p}: only in emit")
            elif k not in b:
                out.append(f"{p}: only in constexpr")
            else:
                _diff(a[k], b[k], p, out, ignore)
    elif a != b:
        out.append(f"{path}: constexpr={a!r} emit={b!r}")


def compare(run_dir, module_name, ignore_patterns):
    run = pathlib.Path(run_dir).resolve()
    repo = pathlib.Path(__file__).resolve().parent.parent.parent
    backend = os.environ.get("CORPUS_TOOLCHAIN") or (
        "gcc16" if sys.platform.startswith("linux") else "clang-p2996")
    is_gcc = backend == "gcc16"
    suffix = "-gcc16" if is_gcc else ""
    venv_py = (repo / "gcc16-proveout" / "venv" / "bin" / "python") if is_gcc \
        else (repo / ".venv" / "bin" / "python")
    me = pathlib.Path(__file__).resolve()

    def dump_lane(build_dir, dyld):
        env = dict(os.environ)
        env.pop("DYLD_LIBRARY_PATH", None)
        if dyld and not is_gcc:
            env["DYLD_LIBRARY_PATH"] = str(repo / "toolchain" / "lib")
        p = subprocess.run([str(venv_py), str(me), "dump", str(build_dir),
                            module_name], capture_output=True, text=True,
                           env=env)
        if p.returncode != 0:
            print(f"surface dump failed for {build_dir}:\n{p.stderr}",
                  file=sys.stderr)
            sys.exit(3)
        return json.loads(p.stdout)

    # clang backend: the constexpr lane needs the toolchain libc++; the emit
    # lane must NOT see it (leaf-name dyld override would hijack the system
    # libc++). gcc16 backend: one runtime, no env shaping.
    cx = dump_lane(run / "binding" / ("build" + suffix), dyld=True)
    em = dump_lane(run / "binding" / ("build-emit" + suffix), dyld=False)

    art = run / "tests" / ("build" + suffix)
    art.mkdir(parents=True, exist_ok=True)
    (art / "surface_constexpr.json").write_text(
        json.dumps(cx, indent=1, sort_keys=True) + "\n")
    (art / "surface_emit.json").write_text(
        json.dumps(em, indent=1, sort_keys=True) + "\n")

    ignore = [re.compile(p) for p in ignore_patterns]
    mismatches = []
    _diff(cx, em, "", mismatches, ignore)
    for m in mismatches:
        print(m)
    print(f"surface_diff: {len(mismatches)} mismatch(es), "
          f"{len(cx)} top-level object(s) compared")
    return 1 if mismatches else 0


def main(argv):
    if len(argv) >= 4 and argv[1] == "dump":
        dump(argv[2], argv[3])
        return 0
    if len(argv) >= 4 and argv[1] == "compare":
        ignores = []
        rest = argv[4:]
        while rest:
            if rest[0] == "--ignore" and len(rest) > 1:
                ignores.append(rest[1])
                rest = rest[2:]
            else:
                rest = rest[1:]
        return compare(argv[2], argv[3], ignores)
    print(__doc__, file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
