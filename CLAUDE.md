# soft_render — C++ Software Renderer

## Build

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Produces: `build/demo` and `build/viewer` (if SDL2 found).

## Run

```bash
./build/demo                    # sphere, writes frame_0000.ppm … frame_0059.ppm
./build/demo path/to/mesh.obj   # custom OBJ mesh
./build/viewer                  # real-time SDL2 window (requires SDL2)
./build/viewer path/to/mesh.obj
```

## Architecture

```
include/soft_render/
  math/       — Vec3, Vec4, Mat4 (SSE4.1 on x86, NEON on ARM)
  core/       — Framebuffer, Texture
  pipeline/   — Vertex, VertexProcessor, Rasterizer, FragmentShader
  render/     — Renderer (high-level draw API)
src/          — Implementations
examples/
  demo/       — Offline renderer → PPM frames
  viewer/     — Real-time SDL2 interactive viewer
```

## Gotchas

- **SDL2 path is hardcoded** in `CMakeLists.txt`: `set(SDL2_PATH "/usr/local/Cellar/sdl2/2.32.6" ...)`. Update if SDL2 version changes (`ls /usr/local/Cellar/sdl2/`).
- **SIMD is auto-detected** at cmake time. x86_64 gets `-msse4.1 -DUSE_SSE4_1`; ARM gets `-DUSE_NEON`. Do not mix object files compiled with different flags.
- `Mat4::lookAt` uses right-handed convention (OpenGL-style). Forward is `-Z`.
- The demo writes PPM files to **cwd** — run from `build/` or redirect output.
