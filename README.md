# LTB VLK

[![CI](https://github.com/LoganBarnes/ltb-vlk/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/LoganBarnes/ltb-vlk/actions/workflows/ci.yml)

A C++ Vulkan Library. Work in progress.

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
| `raw-app`        | GLFW/OpenGL scratch app (built only if OpenGL is found)                     |

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
