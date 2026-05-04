# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

A more detailed agent reference (style guide, JSON command list, integration notes, full module class listings) is in `AGENTS.md`. Read that for deeper context; this file only captures what is needed to be productive quickly.

## What this repo produces

A single C++20 static library, `librtphone.a`, that implements a SIP/RTP softphone stack (codecs, media transport, SIP user agent, cross-platform audio I/O). It is consumed by other applications via a JSON command interface (`AgentImpl`). There is no executable target in `src/` — only the library.

## Build and run

The Python scripts wipe and recreate their build directory each invocation, so they are clean configure+build runs, not incremental.

```bash
python3 build_linux.py        # wipes build_linux/, configures with Ninja, outputs build_linux/librtphone.a
python3 build_android.py      # needs ANDROID_NDK_HOME and VCPKG_ROOT; arm64-v8a, API 24
python3 build_windows.py      # needs VCPKG_ROOT (defaults to C:\tools\vcpkg); VS 2022 x64
./run_ci.sh                   # Make-based CI build into ./build (used by Jenkins; pings Telegram on failure)
```

For incremental work, configure once and rebuild manually instead of re-running the wrapper:

```bash
mkdir -p build && cd build
cmake ../src -G Ninja -D OPUS_X86_MAY_HAVE_SSE4_1=ON
cmake --build . -j$(nproc)
```

CMake options (in `src/CMakeLists.txt`): `USE_AMR_CODEC` (default ON, force-OFF on Android), `USE_EVS_CODEC` (default ON), `USE_MUSL` (default OFF).

## Tests

There is no unit test suite for the library itself. The only first-party test is `test/rtp_decode/` — a standalone CMake project that links the library and exercises RTP decoding from a capture file. Build and run it via:

```bash
mkdir -p test/rtp_decode/build && cd test/rtp_decode/build
cmake .. && cmake --build . -j$(nproc)
./rtp_decode <args>
```

Note: `test/rtp_decode/CMakeLists.txt` does `add_subdirectory(../../src ...)`, so it rebuilds the whole library inside its own build tree. Don't expect to share artifacts with `build_linux/`.

## Submodules and external paths

The `src/libs/` directory mixes vendored sources with three git submodules: `resiprocate` (SIP stack, sevana branch), `libsrtp`, and `libraries` (prebuilt platform binaries — OpenSSL 1.1, Boost headers, etc.). After clone:

```bash
git submodule update --init --recursive
```

## Architecture: how a call flows through the modules

The five modules under `src/engine/` form a vertical stack. Understanding the flow matters more than the file list (which is in `AGENTS.md`):

1. **`agent/`** — Public API surface. Hosts apps create one `AgentImpl`, push JSON command strings into `command()`, and pull JSON event strings out via `waitForData()`/`read()`. This is the only thing external code should touch.
2. **`endpoint/`** — Wraps reSIProcate. `UserAgent` (in `EP_Engine.h`) owns the SIP transports, registrations, and session lifecycle; `EP_Account` and `EP_Session` are the SIP-side objects the agent manipulates. `EP_AudioProvider`/`EP_DataProvider` bridge SIP sessions to media streams.
3. **`media/`** (`MT::` namespace) — Codec registry (`MT_CodecList`), per-call audio pipeline (`MT_AudioStream`, `MT_AudioReceiver`), RTP send (`MT_NativeRtpSender`), SRTP (`MT_SrtpHelper`), DTMF (`MT_Dtmf`). Codec wrappers like `MT_AmrCodec`/`MT_EvsCodec` adapt vendored libraries in `src/libs/` to the `MT::Codec` base.
4. **`audio/`** (`Audio::` namespace) — Cross-platform device I/O. `Audio::Interface` is the abstraction; concrete implementations are picked at compile time: `Audio_DirectSound` (Windows), `Audio_CoreAudio` (macOS/iOS), `Audio_AndroidOboe` (Android, via `libs/oboe`), `Audio_Null` (testing). Also hosts mixing, resampling, WAV I/O, AEC integration.
5. **`helper/`** (`HL::` namespace) — Cross-cutting primitives used by every other module: `HL_Sync` (mutex/event), `HL_NetworkSocket`, `HL_Log`, `HL_VariantMap` (used as the runtime config bag passed to `UserAgent`), `HL_Rtp`, `HL_IuUP` (3G Iu-UP framing), `HL_ThreadPool`, `HL_ByteBuffer`.

ICE/STUN lives separately under `src/libs/ice/` (it is compiled directly into the library, not as a subproject) and is wired into the media path for NAT traversal.

Compile-time tunables — sample rate (48 kHz), buffer sizes, RTP/codec payload types, media port range — are all in `src/engine/engine_config.h`. Runtime configuration flows through `HL::VariantMap` keyed by the `CONFIG_*` enum in `EP_Engine.h`.

Per-stream call-quality metrics (RTT, jitter per RFC 3550 §A.8, packet-loss timeline, RFC 2833 DTMF events, network MOS) are collected in `MT::Statistics` / `MT::JitterStatistics` in `src/engine/media/MT_Statistics.{h,cpp}` and surfaced through the agent's JSON event stream.

## Conventions worth knowing before editing

- **File prefixes encode the module**: `Agent_*`, `EP_*`, `MT_*`, `Audio_*`, `HL_*`. Match this when adding files.
- **Members use `m` prefix** (`mAgentMutex`, `mSessionMap`); smart-pointer typedefs use `P` prefix (`PSession`, `PVariantMap`).
- **Platform code is gated by `TARGET_WIN` / `TARGET_LINUX` / `TARGET_OSX` / `TARGET_ANDROID` / `TARGET_MUSL`**, set in `src/CMakeLists.txt`. Don't sniff `_WIN32`/`__linux__` directly.
- **Every source file carries the MPL 2.0 header** (see top of any existing `.cpp`). New files need the same block.
- **Thread safety is via `std::recursive_mutex`** (e.g. `mAgentMutex` guards the agent's public surface). Memory uses `std::shared_ptr` extensively — prefer the existing `P*` typedefs over raw pointers.
- The codebase recently migrated to **C++20 and `std::chrono`**; avoid reintroducing older idioms or hand-rolled time math.

## Patent caveat

AMR-NB/AMR-WB and EVS sources are included but no patent licenses are bundled. If a change touches `MT_AmrCodec`/`MT_EvsCodec` or their build flags, keep in mind users are expected to license these codecs themselves — don't enable them by default in contexts where that hasn't been arranged.
