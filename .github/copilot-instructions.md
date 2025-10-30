## Purpose

Short, actionable guidance for AI coding agents working on this repository. Focus on the real codebase (C++/JUCE + tracktion modules) and the developer workflows required to build, test and change it.

## Big picture (what to know first)

- This is a native C++ audio / UI application using JUCE and the bundled `tracktion_engine` module. Key app logic lives under `Source/` (e.g. `Main.cpp`, `MainComponent.*`, `DatabaseManager.*`, `FileScanner.*`, exporters like `RekordboxExporter.*`).
- The app is compiled with CMake (top-level `CMakeLists.txt`) and includes the `JUCE/` and `tracktion_engine/` directories as in-tree dependencies. Expect multi-platform CMake usage; Windows developers typically generate a Visual Studio solution.
- Runtime data flow (common pattern): FileScanner -> DatabaseManager (persistence) -> UI components (LibraryTableComponent, PlaylistTreeComponent) -> Exporters (Rekordbox/Serato/Traktor) for external formats.

## Key files & directories (jump-to examples)

- `CMakeLists.txt` — top-level build orchestration. Use this to find build targets and test integration.
- `Source/Main.cpp`, `Source/MainComponent.*` — application entry and top-level UI wiring.
- `Source/DatabaseManager.*`, `Source/FileScanner.*` — database and library ingestion logic.
- `Source/RekordboxExporter.*`, `Source/SeratoExporter.*`, `Source/TraktorExporter.*` — format-specific export logic; good examples of file I/O and serialization patterns.
- `Source/Test*.*` (e.g. `TestRekordboxExport.cpp`) — small unit/integration style checks you can run to validate export behavior.
- `JUCE/` and `tracktion_engine/` — vendored engine and framework code. Avoid editing these unless necessary.

## Developer workflows (quick commands)

Prefer PowerShell on Windows (repository includes helper scripts). Typical dev flow:

```powershell
.\check-prerequisites.ps1      # verify required SDKs/tools
.\install.ps1                 # optional setup tasks (dependencies, hooks)
mkdir build; cmake -S . -B build -G "Visual Studio 17 2022"  # generate
cmake --build build --config Debug                            # build
cmake --build build --config Release                          # build release
cmake --build build --target RUN_TESTS --config Debug         # if CTest/targets are defined
``` 

Notes: the repo provides `*.ps1` and `*.bat` helpers; check them before inventing alternate steps. If you need a different generator (Ninja, VS 2019, etc.), match local toolchain.

## Tests & quick validation

- Tests are lightweight C++ files under `Source/` with names starting `Test*`. They may be wired into CTest or a custom runner via CMake. After building, run CTest from the build directory when available.
- For behavioral changes (export formats, DB migrations), run the corresponding `Test*` file (e.g. `TestRekordboxExport`) or the app in a Debug build to exercise UI flows.

## Project-specific patterns & conventions

- Exporters: each external format has its own exporter class in `Source/*Exporter.*` that reads from the centralized `DatabaseManager` and writes files. Follow those patterns when adding new formats.
- Single-source-of-truth: `DatabaseManager` centralizes persistence; components request data through it rather than reaching into storage directly.
- UI components follow JUCE idioms: use callbacks/message passing and are located under `Source/*Component.*` (e.g., `LibraryTableComponent.*`, `WaveformComponent.*`). Match existing event-dispatch patterns.
- Tests under `Source/` are small and example-driven — mimic their style for new verification code.

## Integration points & external deps

- JUCE and `tracktion_engine` are the major vendored frameworks; treat them as stable libraries.
- The app likely uses a local embedded DB (see `DatabaseManager.*`) — any schema changes should include migration notes and tests.

## When editing code (practical checklist)

1. Locate related UI/logic in `Source/` (see exporter/db/ui files above).
2. Build locally (Debug) and run the minimal test or the app to exercise changes.
3. Add or update `Test*` files where applicable (small, reproducible checks). Keep tests independent.
4. Avoid editing `JUCE/` and `tracktion_engine/` unless necessary; prefer adapter code in `Source/`.

## What to avoid / gotchas

- Don't assume Java/Maven conventions — this repo is native C++/CMake. The old top-of-file content (Maven/Java) was outdated.
- Be mindful of platform-specific paths and generators in CMake; prefer using the shipped scripts when they exist.

## If something is missing

- If a build/test step is unclear or failing, open `CMakeLists.txt` and `check-prerequisites.ps1` first. If project-specific tooling is needed, add short helper scripts instead of changing global dev docs.

---

If anything above is unclear or you want more examples (small code snippets showing DatabaseManager calls or exporter usage), tell me which area to expand and I will add 1-2 concrete examples from `Source/`.
