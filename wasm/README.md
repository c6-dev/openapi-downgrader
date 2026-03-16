# openapi-downgrader — WebAssembly / Browser Build

This directory contains everything needed to build and run the OpenAPI 3.0 → Swagger 2.0 converter **entirely in the browser** using WebAssembly (Emscripten).

## Prerequisites

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) (`emcc`/`em++` on your PATH)
- CMake >= 3.15
- Git (used by CMake `FetchContent` to download yaml-cpp)
- Python 3 (for the local dev server)

### Install Emscripten SDK

```bash
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh   # add to your shell profile for persistence
```

## Build

Run the following commands from the **repository root**:

```bash
mkdir build-wasm
cd build-wasm
emcmake cmake ../wasm
cmake --build . -- -j$(nproc)
```

This will produce `downgrader.js` and `downgrader.wasm` in the `build-wasm/` directory.

After a successful build, copy (or symlink) those two files next to `wasm/index.html`:

```bash
cp build-wasm/downgrader.js  wasm/
cp build-wasm/downgrader.wasm wasm/
```

## Serve locally

Browsers require a proper HTTP server to load `.wasm` files (the `file://` protocol does not work):

```bash
cd wasm
python3 -m http.server 8080
```

Then open <http://localhost:8080> in your browser.

## GitHub Pages hosting

1. After building, commit `wasm/index.html`, `wasm/downgrader.js`, and `wasm/downgrader.wasm` to your repository.
2. Enable **GitHub Pages** in the repository settings, pointing at the branch/folder that contains the `wasm/` directory (or move the files to `docs/`).
3. GitHub Pages serves static files with the correct MIME types for `.wasm`, so no extra configuration is needed.

## Notes

- yaml-cpp is fetched automatically by CMake `FetchContent` during configure — no manual download required.
- The prebuilt `.lib` files in `lib/` are Windows-only and are **not** used by the Wasm build.
- The conversion runs 100 % client-side; no data is sent to any server.
