# AGENTS.md

Simon Tatham's Portable Puzzle Collection — a C library of ~40 single-file
puzzle back ends plus platform front ends (GTK/Windows/macOS/Emscripten/
NestedVM/KaiOS). Built with CMake.

## Build

```sh
cmake .              # in-source configure is normal here
cmake --build .      # or: cmake --build . -j
```

Strict CI build (enables `-Werror` via `-DSTRICT=ON`) — run with **both** gcc
and clang before assuming a change is safe; the CI script (`Buildscr`) does
exactly this:

```sh
cmake -B test-gcc  -DSTRICT=ON .
cmake -B test-clang -DSTRICT=ON -DCMAKE_C_COMPILER=clang .
cmake --build test-gcc -j ; cmake --build test-clang -j
```

NDEBUG is deliberately stripped from Release/MinSizeRel/RelWithDebInfo (see
`cmake/setup.cmake`). **Do not re-add `-DNDEBUG`** — assertions are load-bearing
and cheap; keep them on in shipped binaries.

## Layout / how a puzzle is wired

- Each puzzle is one file `<name>.c` at the repo root, implementing the `game`
  struct from `puzzles.h`. The struct must be renamed from the `nullgame`
  template (macOS build breaks otherwise — see `CHECKLST.txt`).
- `CMakeLists.txt` registers puzzles via the `puzzle(NAME ...)` macro defined
  in `cmake/setup.cmake`. That macro:
  - emits an executable target,
  - records metadata (`displayname_*`, `description_*`, `objective_*`) used by
    the web/installer build,
- A `solver(NAME)` line emits a `<name>solver` CLI binary built from the same
  `.c` with `-DSTANDALONE_SOLVER`. Some solvers also need `latin.c` etc. —
  pass them as extra args: `solver(towers latin.c)`.
- Auxiliary tools come from `cliprogram(...)` / `guiprogram(...)`:
  `mineobfusc`, `patternpicture`, `galaxiespicture`, `galaxieseditor`,
  `pearlbench`, `grapheditor`, `polygon-test` (SDL, gated on
  `BUILD_SDL_PROGRAMS`).
- `nullgame` is built (to prove the build works) but is `official=FALSE` — it
  is not installed, not listed in `gamelist.txt`, and not rolled into combined
  binaries.
- Shared core lives in the `core` / `common` OBJECT libraries (`midend.c`,
  `drawing.c`, `random.c`, `grid.c`, `latin.c`, `matching.c`, `tree234.c`,
  `findloop.c`, `loopgen.c`, `hat.c`, `spectre.c`, `penrose*.c`, etc.).
  Front ends per platform are selected in `cmake/platforms/<name>.cmake`.
- On **Windows**, a puzzle's exe name comes from `WINDOWS_EXE_NAME` (e.g. `net`
  → `netgame.exe` to avoid clashing with Windows' own `net`). Don't assume the
  binary name equals the source name there.

## Unfinished puzzles

`unfinished/` holds games that compile but aren't shipped. To promote one to
official for a build:

```sh
cmake -DPUZZLES_ENABLE_UNFINISHED=group .
# multiple: -DPUZZLES_ENABLE_UNFINISHED="group;path"
```

`unfinished/` has its own `CMakeLists.txt` and uses
`export_variables_to_parent_scope()` to feed names back to the top level.

## "Tests"

There is no unit test framework. Verification is:

1. **Strict build with gcc and clang** (`-DSTRICT=ON`) — the primary signal.
2. **Benchmark** — run from the build dir (needs `gamelist.txt`, written by
   `build_extras()` at configure end):
   ```sh
   ./benchmark.sh              # all presets of every game
   ./benchmark.sh solo/cube    # or a subset
   ```
   Each game self-tests via `--test-solve --time-generation --generate 100
   <preset>`. Failures print to stderr.
3. **`fuzzpuzz`** — built only when `build_cli_programs` is on; uses libFuzzer
   when `-DWITH_LIBFUZZER=ON` and `HAVE_HF_ITER` is detected. Corpus in
   `fuzzpuzz.dict`. This is the closest thing to property tests.
4. **Per-game self-checks** in `#ifdef COMBINED` / standalone helper paths
   inside each `.c`.

When you add or modify puzzle logic, run at minimum: strict gcc + strict clang
build, then `./benchmark.sh <that_puzzle>`.

## Docs

Manuals and HACKING are written in Halibut (`puzzles.but`, `devel.but`) and
generated via `halibut`. The Makefile.doc is binary-marshalled; rely on
`Buildscr`'s invocations instead:
```sh
halibut --text=HACKING devel.but
halibut --winhelp=puzzles.hlp --text=puzzles.txt puzzles.but
```

## Adding a puzzle (summary of `CHECKLST.txt`)

- Write `<name>.c` (start from `nullgame.c`), rename the `game` struct.
- Add a `puzzle(NAME DISPLAYNAME ... DESCRIPTION ... OBJECTIVE ...)` block +
  optional `solver(NAME)`.
- Update `LICENCE` and `puzzles.but` copyright if by a new author; add a docs
  section in `puzzles.but` including a Windows help topic string referenced by
  the game struct's help-topic field.
- Set `REQUIRE_RBUTTON` / `REQUIRE_NUMPAD` flags on the `game` struct if the
  puzzle needs them.
- Add the Unix binary name(s) (and any auxiliary binary names) to `.gitignore`.
- Add `html/<name>.html` instructions fragment for the webified puzzle.
- For the site icon: drop a save file in `icons/`, optionally add
  `<name>_redo` / `<name>_crop` in `icons/icons.cmake`.

## Things easy to miss

- The CI/release pipeline is `Buildscr` (a `bob` script), not plain shell. It
  builds icons, runs strict gcc+clang, then cross-builds Windows (clangcl
  win64/win32), macOS, NestedVM, Emscripten, KaiOS, packages installers
  (WiX), signs, and uploads. Don't try to reproduce it by hand — use the
  individual `cmake`/`make` incantations above for local work.
- In-source `cmake .` is expected; out-of-source works but `Buildscr` mixes
  styles. Stick to in-source unless you have a reason.
- `BUILD_SDL_PROGRAMS` defaults **OFF**; `polygon-test` and SDL-using
  `cliprogram`s won't build without it.
- `USE_DRAW_POLYGON_FALLBACK` forces a software polygon rasterizer — useful
  when debugging rendering on a platform without native accelerated polygons.
- `PUZZLES_ROOT_DIR` is remapped in `__FILE__` via `-fmacro-prefix-map` so
  assertion messages don't leak local build paths; clang-cl uses
  `-Xclang -fmacro-prefix-map`. Don't strip this — it keeps release binaries
  deterministic and path-free.

## Reference

- Architecture / porting guide: `puzzles.h` (game interface), `devel.but`
  (run through `halibut --text=HACKING devel.but` to read it).
- New-puzzle checklist: `CHECKLST.txt`.
- Per-platform wiring: `cmake/platforms/*.cmake`.
- Release pipeline: `Buildscr`.

`opencode.json` does not exist in this repo; this file is the only
agent-facing instruction source.