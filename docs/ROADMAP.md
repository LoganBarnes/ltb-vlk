# Roadmap and Open Work

Where things stand after the November 2026 code review, and what's left to do.

- The original review report (private, shared from the owner's claude.ai account):
  <https://claude.ai/artifact/Rig2odvTo5Pw3fpGqU85uR>
- Everything below was checked against `main` at `ef56c1c` (25 Nov 2026).

## Current state

| Branch | State |
|:--|:--|
| `main` | All high-, medium- and low-severity review fixes, 66 tests, GitHub Actions CI (GCC 14 + Clang 18 on lavapipe), and `share-app` with platform-neutral `SharedImage`/`Channel` interfaces (Linux backends). |
| `windows-backend` | Windows backends for `Channel` and `SharedImage`. **Written on Linux, never built or run on Windows.** Linux still builds and passes all tests on this branch. |

## 1. Windows backend (`windows-backend` branch)

### What's there

| File | What it does |
|:--|:--|
| `src/ltb/ipc/windows/win_handle.cpp` | `WinHandle` owns an NT `HANDLE` (`CloseHandle` on destruction). It's `ipc::NativeHandle` on Windows. |
| `src/ltb/ipc/windows/channel.cpp` | `Channel` over message-mode named pipes. Handles are duplicated into the peer process with `DuplicateHandle` and their values sent in a header before the payload. Undelivered duplicates are closed again. Reads call `PeekNamedPipe` first so they never block. `ChannelListener` keeps one instance waiting with overlapped `ConnectNamedPipe`, so `accept()` doesn't block. The first instance uses `FILE_FLAG_FIRST_PIPE_INSTANCE`, so a second listener fails with a clear error. Address: `\\.\pipe\<name>-<session id>`. |
| `src/ltb/vlk/shared_image_win32.cpp` | `SharedImage` memory as NT handles (`VK_KHR_external_memory_win32`, `eOpaqueWin32`). Exports with `vkGetMemoryWin32HandleKHR`. Importing an NT handle doesn't give it to Vulkan, so the caller closes it (`WinHandle` does). |
| `src/ltb/vlk/shared_image.cpp` + `shared_image_backend.hpp` | Common `SharedImage` code for Linux and Windows. Each platform only implements 4 functions (handle type, extensions, import, export). |

CMake picks `src/ltb/ipc/<platform>/` automatically, `LTB_IPC_SUPPORTED` (in
`ltb/ipc/native_handle.hpp`) marks platforms with backends, and `share-app` is built on
Linux and Windows. The `Channel` and `SharedImage` tests are platform-neutral; on Windows
the `Channel` test sends an event handle and checks the received copy is the same event.

### To finish it

1. Build on Windows. **Start with `LTB_VLK_USE_STRICT_FLAGS=OFF`.** The library has likely
   never been built with MSVC, so unrelated code may fail with `/W4 /WX` first.
2. Run the unit tests (`ctest -L unit`), especially `ChannelTest.*`. They exercise the
   named-pipe backend for real, no GPU needed.
3. Run the GPU tests (`ctest -L gpu`), especially `SharedImageTest.*`.
4. Run `share-app --display` and a few `share-app --share` in other terminals.
5. Merge into `main`, then add a Windows job to CI (MSVC, install the Vulkan SDK, run the
   unit tests). GitHub's Windows runners have no GPU, so GPU tests will skip there unless a
   software driver such as Mesa's lavapipe for Windows is installed. Whether lavapipe on
   Windows supports `VK_KHR_external_memory_win32` hasn't been checked.

### Likely trouble spots

- `PeekNamedPipe` / `GetOverlappedResult` / `ConnectNamedPipe` details in `channel.cpp`.
  They follow the documentation but are the most likely to surprise.
- `shared_image_win32.cpp` defines `VK_USE_PLATFORM_WIN32_KHR` before its first include so
  the Vulkan headers declare the Win32 types. If it fails to compile, check include order.
- `DuplicateHandle` needs the peer opened with `PROCESS_DUP_HANDLE` (plus `SYNCHRONIZE`, to
  detect that it exited). That's fine for processes of the same user.
- `common.cpp` in `share-app`: only `SIGINT` (Ctrl+C) is delivered on Windows, not `SIGTERM`.

## 2. macOS backend (not started)

MoltenVK can't export memory as file descriptors, and macOS has no `SCM_RIGHTS`-style fd
passing between arbitrary processes that Vulkan memory could use. The plan:

- **`SharedImage`:** IOSurfaces. `create()` makes an `IOSurface` and imports it into Vulkan
  with `VK_EXT_metal_objects` (`VkImportMetalIOSurfaceInfoEXT`). `export_handle()` returns a
  Mach port (`IOSurfaceCreateMachPort`), and `import()` looks the surface up from the port
  (`IOSurfaceLookupFromMachPort`). This doesn't fit `shared_image_backend.hpp` (Vulkan
  doesn't export anything), so macOS implements `shared_image.hpp` directly.
- **`Channel`:** Mach messages (or XPC) carrying Mach port rights, with
  `NativeHandle` as a Mach port send-right wrapper.
- **Check first:** how far MoltenVK's `VK_EXT_external_memory_metal` support goes now. It may
  offer a simpler route.
- **Testing:** CI would need a macOS runner.

## 3. Open review items

### Build and config

- **Install/export** (`install()`, `INSTALL_INTERFACE` include dirs, a package config) so
  other projects can `find_package(LtbVlk)`. Deferred as a design decision.

### API and design

- **Frame lifecycle is easy to get wrong.** `VulkanCommandAndSync::increment_frame()` must be
  called by hand after every frame (the `objs` app forgot it before the review). Advance it
  automatically in `present_frame`/`end_frame`, or make a frame an RAII object that does it.
- **Boilerplate repeated across the apps:**
  - The uniform-descriptor write block is copy-pasted ~7 times. A library helper such as
    `write_buffer_descriptors( sets, binding, VulkanBuffer )` would replace them.
  - `update_camera_uniforms` is identical in the `api` and `objs` apps.
  - Per-frame "dirty" tracking (`camera_frames_updated_`, `compute_frames_updated_`) is
    hand-rolled in each app. A small `PerFrameUniform< T >` would cover it.
  - Staging-buffer uploads are hand-written in `api`/`objs`, though `vlk::copy_buffer`
    exists.
  - The `api` app stays as the deliberately low-level demo.
- **Memory-layout footgun.** `append_memory_size_n( layout, sizeof( T ), n )` silently means
  "no alignment", because `sizeof` converts to a `vk::MemoryRequirements` with alignment 0.
  All call sites pass the device's alignment now, but the trap remains. Also,
  `append_memory_size_n` clears the layout despite its name.
- **`VulkanPresentation::rebuild( { } )`** resets the swapchain settings to defaults instead of
  keeping the ones it was created with. It also compares the requested size against the
  clamped size it actually created.

### Smaller items (confirmed still open)

- **Swapchain:** `compositeAlpha` is hard-coded to `eOpaque`, and `image_usage` isn't
  checked against the surface's `supportedUsageFlags`.
- **`lines_2d.vert`:** add a comment on why its `mat3` push constant is a `glm::mat3x4` on
  the C++ side (std430 pads each column to 16 bytes).
- **`ShaderModuleSettings::name`** is a raw `char const*` that can easily dangle.
- **`Error::operator==`** ignores severity, and `operator!=` is redundant in C++20.
- **`Timer::duration_since_start`** should be `const`.
- **`GLFW_SAMPLES`** is set for windows, but has no effect with Vulkan.
- **`Camera2d`:** mouse-wheel zoom (`0.9^wheel`) is unclamped, so the width can underflow.
- **`raw-app`:**
  - Its `main` returns 0 when initialization fails.
  - It forces the EGL context API.
  - Its GL setup is commented out.
- **`headless-app`:** does no GPU work despite its name.
- **`json_settings.hpp` / `json_utils.hpp`** include `nlohmann/json.hpp`, but nlohmann_json
  isn't a dependency, so they can't be compiled in this repo. Add the dependency (and
  tests) or remove the headers.
- **clang-tidy:** 19 findings that predate the review, mostly
  `modernize-use-trailing-return-type` in `units.hpp`, plus naming findings on forward
  declarations of third-party types.

### Tests still worth adding

- **CPU only:**
  - Memory layout and alignment helpers.
  - `find_memory_type_index` with fake `PhysicalDeviceMemoryProperties`.
  - `single_loop_iteration` timing (step counts, `max_frame_time`, pause, `time_scale`).
- **GPU:**
  - A start/end frame loop under validation layers.
  - An offscreen `LinesPipeline2` render with pixel readback.

## 4. Known issues

- **NVIDIA device-lost flake.** On the RTX 4050 laptop GPU, `vkCreateDevice` occasionally
  returns `VK_ERROR_DEVICE_LOST`. Affected GPU tests skip rather than fail. It hits old
  tests as well as new ones, never happens on lavapipe, and looks like the discrete GPU
  waking from a power-saving state. Use `VK_DRIVER_FILES=.../lvp_icd.json` for reliable
  local runs.
- **Synchronization validation.** It couldn't be enabled locally: neither
  `VK_KHRONOS_VALIDATION_VALIDATE_SYNC` nor a `vk_layer_settings.txt` had an effect. The
  compute/graphics barrier in the particles apps is therefore justified by reading the code,
  not by a validation run. Worth retrying with `vkconfig`.
- **Two apps untested in some situations.** `share-app` and the swapchain "out of date
  without a resize" path haven't been exercised by minimizing a window (on X11, GLFW may not
  report 0×0 when minimized).
