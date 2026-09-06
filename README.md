## Aquamarine

> **Omarchy32 CPU fork:** the `cpu-backend` branch adds SHM allocation, nested SHM output and DRM dumb-buffer selection. See [fork divergence, upstream backlog, limitations and tracking issues](docs/divergence.md). It supports [Hyprland's CPU renderer](https://github.com/cederikdotcom/Hyprland/blob/pixman-renderer/docs/divergence.md); [Omarchy integration](https://github.com/cederikdotcom/omarchy32cpu/blob/main/docs/divergence.md) is tracked separately.

Aquamarine is a very light linux rendering backend library. It provides basic abstractions
for an application to render on a Wayland session (in a window) or a native DRM session.

It is agnostic of the rendering API (Vulkan/OpenGL) and designed to be lightweight, performant, and
minimal.

Aquamarine provides no bindings for other languages. It is C++-only.

## Stability

Aquamarine depends on the ABI stability of the stdlib implementation of your compiler. Sover bumps will be done only for aquamarine ABI breaks, not stdlib.

## Building

```sh
cmake --no-warn-unused-cli -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr -S . -B ./build
cmake --build ./build --config Release --target all -j`nproc 2>/dev/null || getconf _NPROCESSORS_CONF`
```

