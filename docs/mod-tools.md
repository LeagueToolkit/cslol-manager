## mod-tools CLI

A small CLI for working with CSLOL-style mods and WADs: importing/exporting, copying/optimizing, building overlays, and running the live overlay patcher.

Unless otherwise noted, paths may be absolute or relative. Commands log progress and exit with non‑zero on error.

### General usage

```bash
mod-tools <command> [args] [flags]
```

Commands:
- `addwad`, `copy`, `export`, `import`, `mkoverlay`, `runoverlay`

Common flags:
- `--game:<path>`: Path to the game folder. Enables rebasing against the game WADs and filtering.
- `--noTFT`: Exclude Teamfight Tactics assets when indexing game WADs (skips `map21` and `map22`).

Notes on formats:
- “WAD source” can be a `.wad`/`.wad.client` file or a directory that looks like a unpacked WAD (e.g., contains `data`, `data2`, `levels`, `assets`, or `OBSIDIAN_PACKED_MAPPING.txt`).
- “Mod folder” must contain `META/info.json` and a `WAD` directory after optimization.

---

## Commands

### addwad

Add a WAD (file or directory) into a mod folder, optionally rebasing against the game to strip unmodified entries and align file naming.

```bash
mod-tools addwad <src_wad> <dst_mod_dir> [--game:<path>] [--noTFT] [--removeUNK]
```

- **args**:
  - `<src_wad>`: WAD file or directory to add.
  - `<dst_mod_dir>`: Target mod folder (must contain `META/info.json`). WAD will be written under `WAD/`.
- **flags**:
  - `--game:<path>`: Rebases against game WADs; output filename is aligned to the base mount. Also strips unmodified entries.
  - `--noTFT`: While indexing the game, exclude TFT assets (`map21`, `map22`).
  - `--removeUNK`: When rebasing, also drop entries not present in the base (unknown to the game).
- **behavior**:
  - Without `--game`, the WAD is added and, if needed, renamed to end with `.wad.client`.
  - With `--game`, the tool finds the base mount, optionally removes unknown entries, and always removes unmodified entries before writing to `WAD/`.

Examples:
```bash
mod-tools addwad ./my-changes.wad ./MyMod --game:/games/LoL --noTFT --removeUNK
mod-tools addwad ./unpacked-wad/ ./MyMod
```

---

### copy

Optimize and copy a mod folder, resolving internal conflicts and optionally rebasing against the game. Useful for in‑place optimization.

```bash
mod-tools copy <src_mod_dir> <dst_mod_dir> [--game:<path>] [--noTFT]
```

- **args**:
  - `<src_mod_dir>`: Source mod folder (must contain `META/info.json`).
  - `<dst_mod_dir>`: Destination mod folder. If it exists, it will be replaced.
- **flags**:
  - `--game:<path>`: Rebase mod WADs against the game to reduce deltas.
  - `--noTFT`: Exclude TFT assets when indexing the game.
- **behavior**:
  - Resolves conflicts inside the mod, writes normalized WADs into `dst_mod_dir`, and copies `META/*`.
  - If `src_mod_dir == dst_mod_dir`, performs a safe in‑place optimization using a temporary directory.

Examples:
```bash
mod-tools copy ./MyMod ./MyMod.optimized --game:/games/LoL
mod-tools copy ./MyMod ./MyMod --noTFT
```

---

### export

Export a mod folder to a single archive after optimizing it.

```bash
mod-tools export <src_mod_dir> <dst_archive> [--game:<path>] [--noTFT]
```

- **args**:
  - `<src_mod_dir>`: Source mod folder (must contain `META/info.json`).
  - `<dst_archive>`: Output archive path (e.g., `MyMod.zip` or `MyMod.fantome`).
- **flags**:
  - `--game:<path>`: Optimize using rebase against the game before zipping.
  - `--noTFT`: Exclude TFT assets when indexing the game.
- **behavior**:
  - Internally runs the same optimization as `copy`, then zips the result to `<dst_archive>`.

Examples:
```bash
mod-tools export ./MyMod ./MyMod.zip --game:/games/LoL
```

---

### import

Import various mod inputs into a standardized mod folder, optimizing as needed.

```bash
mod-tools import <src> <dst_mod_dir> [--game:<path>] [--noTFT]
```

- **src forms**:
  - WAD file or directory (see “Notes on formats”).
  - `.zip` or `.fantome` archive.
  - Existing mod folder containing `META/info.json`.
- **flags**:
  - `--game:<path>`: When applicable, rebase/optimize against the game.
  - `--noTFT`: Exclude TFT assets when indexing the game.
- **behavior**:
  - WAD source: creates a minimal `META/info.json` and adds the WAD (with optional rebasing), then writes to `dst_mod_dir`.
  - Zip/Fantome: unzips, optimizes like `copy`, then writes to `dst_mod_dir`.
  - Existing mod: optimizes like `copy` to `dst_mod_dir`.

Examples:
```bash
mod-tools import ./SomeMod.fantome ./SomeMod --game:/games/LoL
mod-tools import ./unpacked-wad ./ImportedFromWad
```

---

### mkoverlay

Build an overlay from multiple mods against the game’s WADs, resolving conflicts deterministically.

```bash
mod-tools mkoverlay <mods_dir> <overlay_dir> --game:<path> --mods:<name1>/<name2>/... [--noTFT] [--ignoreConflict]
```

- **args**:
  - `<mods_dir>`: Parent directory containing the candidate mods as subfolders.
  - `<overlay_dir>`: Output directory where overlay WADs are written.
- **required flags**:
  - `--game:<path>`: Game folder to build against.
  - `--mods:<name1>/<name2>/...`: Slash‑separated list of subfolder names within `<mods_dir>` to include, in apply order.
- **optional flags**:
  - `--noTFT`: Exclude TFT assets when indexing the game.
  - `--ignoreConflict`: Don’t error on conflicts; later mods in the list win.
- **behavior**:
  - Removes overlay‑unsafe entries (e.g., `*.SubChunkTOC`) to avoid collisions.
  - Resolves conflicts within each mod and across the list in order, then writes the merged overlay to `<overlay_dir>` and cleans up stray WADs.

Examples:
```bash
mod-tools mkoverlay ./Mods ./Overlay --game:/games/LoL --mods:MyModA/MyModB --ignoreConflict
```

---

### runoverlay

Run the overlay with the live patcher and stream status to stdout. Exit by pressing Enter when idle.

```bash
mod-tools runoverlay <overlay_dir> <config_file> [--game:<path>] [--opts:<opt1>/<opt2>/...]
```

- **args**:
  - `<overlay_dir>`: Overlay directory produced by `mkoverlay`.
  - `<config_file>`: Config file consumed by the patcher (e.g., `.ini`).
- **flags**:
  - `--game:<path>`: Optional game folder if required by the patcher context.
  - `--opts:<...>`: Slash‑separated runtime options passed through to the patcher. Known values include `none` and `configless` (others may be supported by the patcher).
- **behavior**:
  - Prints lines like `Status: ...`, `Config: ...`, and `[DLL] ...` as the patcher progresses.
  - While patching or awaiting save, exit is locked; when idle, pressing Enter exits the process cleanly.

Examples:
```bash
mod-tools runoverlay ./Overlay ./overlay.ini --game:/games/LoL --opts:configless
```

---

## Exit codes and logging

- On success: prints `Done!` and exits with code 0.
- On error: prints a backtrace and the error message to stderr, exits non‑zero.

## Implementation details (for reference)

- TFT filtering (`--noTFT`) excludes maps named `map21` and `map22` when indexing the game, reducing install size for SR‑only mods.
- `addwad` with `--game` always strips unmodified entries; `--removeUNK` also strips entries not present in the base mount.
- `import` detects WAD sources by filename extension or directory structure (`data`/`data2`/`levels`/`assets`/`OBSIDIAN_PACKED_MAPPING.txt`).


