# Reference ports

These trees are examples of how an N64 game can be split from the machine. They are not linked into `pm_3ds.3dsx` and they are not built.

| Path | What to copy from | What to leave |
| --- | --- | --- |
| `reference/paperboat` | `src/port/` (GBI middleware, NuSys overrides, `os`, audio, `gu.c`). Branch `develop`. | `external/libultraship`, `external/torch`, and the DX11/GL/Metal backends. |
| `reference/sm64-3ds` | `src/pc/gfx/gfx_3ds.c`, `gfx_citro3d.c`. | Its process model and anything that is not the PICA200 path. |
| `reference/soh-3ds` | `platform/3ds/source/gfx_citro3d.cpp`, `gbi_middleware_3ds.cpp`, `audio_ndsp_3ds.cpp`, `input_3ds.cpp`. | The N3DS-only extended-memory process. This port stays on one game thread on core 0. |

Paper Mario keeps emitting F3DEX2 display lists. Only NuSys, libultra OS, and the RSP/RDP tasks are replaced, in `3ds/src/`.
