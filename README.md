# LTB VLK

[![CI](https://github.com/LoganBarnes/ltb-vlk/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/LoganBarnes/ltb-vlk/actions/workflows/ci.yml)

A C++ Vulkan Library. Work in progress. See [docs/ROADMAP.md](docs/ROADMAP.md) for open work.

## Requirements

* CMake 3.23+
* A C++23 compiler with `std::ranges::to` (GCC 14+ or Clang 17+)
* The [Vulkan SDK](https://vulkan.lunarg.com/sdk/home) (headers, loader, and `glslc`).
  The example apps use `VK_KHR_present_mode_fifo_latest_ready`, so use a recent SDK.
* Linux: X11 development libraries (the ImGui GLFW backend calls X11 directly)
* Optional: TBB (`libtbb-dev` / `brew install tbb`) for parallel STL algorithms with libstdc++
* Optional: OpenGL, only for the `raw` example

Other dependencies (GLFW, GLM, ImGui, spdlog, etc.) are downloaded by
[CPM](https://github.com/cpm-cmake/CPM.cmake). Set `CPM_SOURCE_CACHE` to share
downloads between build directories.

## Building

```bash
cmake -B build -DLTB_VLK_BUILD_EXAMPLES=ON -DLTB_VLK_BUILD_TESTS=ON
cmake --build build
```

| Option                     | Default | Description                                                   |
|:---------------------------|:--------|:--------------------------------------------------------------|
| `LTB_VLK_BUILD_EXAMPLES`   | `OFF`   | Build the example apps in `src/ltb/apps`                      |
| `LTB_VLK_BUILD_TESTS`      | `OFF`   | Build the GoogleTest suites in `tests`                        |
| `LTB_VLK_USE_STRICT_FLAGS` | `OFF`   | Treat warnings as errors (only for this project's own targets) |

Shaders in `res/shaders` are compiled to SPIR-V in `<build>/shaders` as part of the build.

## Tests

```bash
ctest --test-dir build -L unit   # CPU only
ctest --test-dir build -L gpu    # Needs a Vulkan device; skipped if none is found
```

GPU tests run on a headless device and are skipped if none is found. Without a GPU,
Mesa's lavapipe driver (`mesa-vulkan-drivers`) provides a software one. Set
`LTB_VLK_REQUIRE_GPU=1` to make a missing device fail the tests instead.

CI (`.github/workflows/ci.yml`) builds everything with strict flags using GCC and Clang,
then runs all tests on lavapipe with the validation layer enabled.

## Examples

| App              | Description                                                                 |
|:-----------------|:----------------------------------------------------------------------------|
| `api-app`        | Low-level demo built directly on the `ltb::vlk` wrappers                    |
| `objs-app`       | The same scene built on the higher-level `ltb::vlk::objs` helpers           |
| `particles-app`  | Compute-shader particle simulation                                          |
| `particles2-app` | Particle simulation with fixed-step compute updates                         |
| `headless-app`   | Runs the update loop with no window or GPU                                  |
| `share-app`      | Shares rendered output between processes (Linux, Windows)                  |
| `raw-app`        | GLFW/OpenGL scratch app (built only if OpenGL is found)                     |

## Sharing output between processes

`share-app` shows how separate processes can render on the GPU and share the results
without copying them through the CPU. Start one display, then any number of producers:

```bash
./build/src/ltb/apps/share-app --display          # Opens a window
./build/src/ltb/apps/share-app --share            # Run in as many terminals as you like
./build/src/ltb/apps/share-app --share --hue 0.3 --size 256 --fps 30
```

Each producer renders offscreen into images whose memory is exported as file descriptors
(`VK_KHR_external_memory_fd`). It sends those descriptors to the display over a Unix domain
socket (`SCM_RIGHTS`), and the display imports the memory and draws every producer as a tile.

* Producers double-buffer. A producer only renders into an image after the display has
  released it, and each hand-off uses a queue family ownership transfer through
  `VK_QUEUE_FAMILY_EXTERNAL`, so the display never reads a half-written frame.
* Memory can only be shared on the same GPU and driver. The display checks each producer's
  device and driver UUIDs and rejects mismatches with a message explaining why.
* `--socket <path>` picks the socket (default `$XDG_RUNTIME_DIR/ltb-vlk-share.sock`). Ctrl+C
  shuts either mode down cleanly.

The app only uses two platform-neutral library interfaces, so supporting another platform
means adding a backend for each, without changing the app:

| Interface | What it does | Linux backend | Windows (planned) | macOS (planned) |
|:--|:--|:--|:--|:--|
| `vlk::SharedImage` (`ltb/vlk/shared_image.hpp`) | Create, export and import images with shared memory | Opaque fds (`VK_KHR_external_memory_fd`) | NT handles (`VK_KHR_external_memory_win32`) | IOSurfaces (`VK_EXT_metal_objects`) |
| `ipc::Channel` (`ltb/ipc/channel.hpp`) | Send messages and native handles between processes | Unix socket + `SCM_RIGHTS` | Named pipe + `DuplicateHandle` | Mach messages / XPC |

Each header documents what a new backend has to implement. `share-app` is built and tested on
Linux and Windows; the macOS backends are only planned.

### Useful References

* [Diagrams](https://github.com/David-DiGioia/vulkan-diagrams#introduction)
* [Object Graph](https://gpuopen.com/learn/understanding-vulkan-objects/)
* [Bootstrap API](https://github.com/charles-lunarg/vk-bootstrap/blob/main/example/triangle.cpp)
* [Headless](https://github.com/SaschaWillems/Vulkan/blob/master/examples/computeheadless/computeheadless.cpp)

### Notes

| Thing                 | Description                                                                      |
|:----------------------|:---------------------------------------------------------------------------------|
| Render Pass           | Specifies the color/depth attachments used by a pipeline                         |
| Descriptor Set Layout | A collection of shader bindings (samplers/images, uniform/storage buffers, etc.) |
| Descriptor Set        | Actual descriptor to which the image/buffer/etc. can be bound                    |
| Pipeline Layout       | All descriptor layouts and push constant ranges used by the shader pipeline      |
| Pipeline              | The full specification and settings for a shader pipeline                        |

* Descriptor Set
  * Descriptor Set Layout
  
* Pipeline Layout
  * Descriptor Set Layouts
  * Push Constant Ranges

* Pipeline
  * Render Pass
  * Pipeline Layout
  * Shader Modules
  * Input Bindings/Attributes
  * Rasterization State
  * Multisample State
