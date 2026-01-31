# RTPhone Platform - Agent Guide

## Project Overview

RTPhone is a comprehensive real-time communication (RTC) platform developed by VoIP Objects (Sevana) that provides a complete software stack for building VoIP/SIP-based communication applications. It delivers production-ready voice communication capabilities with extensive codec support and cross-platform compatibility.

The project produces a static library (`librtphone.a`) that can be integrated into larger telephony and communication systems. It provides a JSON-based command interface for easy integration and control.

## Technology Stack

- **Language**: C++20
- **Build System**: CMake 3.20+
- **License**: Mozilla Public License Version 2.0 (see LICENSE_MPL.txt)
- **Primary Platforms**: Linux (x64, ARM/Raspberry Pi), Windows (32/64-bit), macOS, Android, iOS

## Project Structure

```
/home/anand/works/sevana/platform/rtphone/
├── src/                          # Main source code
│   ├── CMakeLists.txt            # Main CMake configuration
│   ├── engine/                   # Core engine modules
│   │   ├── agent/                # JSON-based command interface
│   │   ├── audio/                # Cross-platform audio I/O handling
│   │   ├── endpoint/             # SIP user agent implementation
│   │   ├── helper/               # Utility functions (networking, logging, threading)
│   │   ├── media/                # Audio codec management and processing
│   │   └── engine_config.h       # Compile-time configuration
│   └── libs/                     # Third-party libraries
│       ├── resiprocate/          # SIP stack (git submodule)
│       ├── libsrtp/              # SRTP library (git submodule)
│       ├── libraries/            # Prebuilt platform libraries (git submodule)
│       ├── ice/                  # ICE (Interactive Connectivity Establishment)
│       ├── jrtplib/              # RTP library
│       ├── opus/                 # Opus codec
│       ├── webrtc/               # WebRTC components
│       ├── libg729/              # G.729 codec
│       ├── libgsm/               # GSM codec
│       ├── gsmhr/                # GSM HR codec
│       ├── g722/                 # G.722 codec
│       ├── speexdsp/             # Speex DSP
│       ├── libevs/               # EVS codec (optional)
│       ├── opencore-amr/         # AMR codec (optional)
│       ├── oboe/                 # Android low-latency audio
│       └── fmt/                  # Format library
├── build_linux.py                # Linux build script
├── build_android.py              # Android build script
├── build_android.sh              # Android build script (shell)
├── run_ci.sh                     # CI build script
└── README.txt                    # Human-readable README
```

## Build Instructions

### Prerequisites

- CMake 3.20+
- C++20 compatible compiler (GCC, Clang, MSVC)
- Python 3 (for build scripts)
- Ninja (recommended, used by build scripts)
- OpenSSL 1.1+ development libraries
- Platform-specific audio libraries

### Linux Build

```bash
python3 build_linux.py
```

This creates a `build_linux/` directory and outputs `librtphone.a`.

### Android Build

```bash
# Ensure ANDROID_NDK_HOME environment variable is set
export ANDROID_NDK_HOME=/path/to/android-ndk
python3 build_android.py
```

Or use the shell script:
```bash
./build_android.sh
```

This creates a `build_android/` directory with ARM64 libraries by default.

### Manual CMake Build

```bash
mkdir build && cd build
cmake ../src -G Ninja
cmake --build . -j$(nproc)
```

### Build Options

The following CMake options are available (defined in `src/CMakeLists.txt`):

- `USE_AMR_CODEC` (ON/OFF): Include AMR-NB/AMR-WB codec support. Default: ON
- `USE_EVS_CODEC` (ON/OFF): Include EVS codec support. Default: ON
- `USE_MUSL` (ON/OFF): Build with MUSL library. Default: OFF

## Module Architecture

### 1. Agent Module (`src/engine/agent/`)

Provides a JSON-based command interface for controlling the engine.

**Key Classes:**
- `AgentImpl`: Main agent implementation, processes JSON commands
- `Agent_AudioManager`: Manages audio devices and streams

**Interface Pattern:**
Commands are sent as JSON strings and responses are returned as JSON.

### 2. Endpoint Module (`src/engine/endpoint/`)

SIP user agent implementation based on reSIProcate.

**Key Classes:**
- `UserAgent` (in EP_Engine.h): Main SIP stack wrapper, handles registration, sessions, presence
- `Account` (EP_Account): SIP account management
- `Session` (EP_Session): Call/session management
- `EP_AudioProvider`: Audio data provider for sessions
- `EP_DataProvider`: Generic data provider interface

**Key Concepts:**
- Implements resiprocate handlers: `ClientRegistrationHandler`, `InviteSessionHandler`, etc.
- Supports multiple transport types: UDP, TCP, TLS
- ICE integration for NAT traversal

### 3. Media Module (`src/engine/media/`)

Audio codec management, RTP/RTCP handling, and media processing.

**Key Classes:**
- `MT::AudioCodec`: Codec management
- `MT::AudioStream`: Audio streaming
- `MT::AudioReceiver`: RTP packet receiving
- `MT::CodecList`: Codec negotiation
- `MT::SrtpHelper`: SRTP encryption
- `MT::Dtmf`: DTMF tone handling
- `MT::AmrCodec`: AMR codec wrapper
- `MT::EvsCodec`: EVS codec wrapper

### 4. Audio Module (`src/engine/audio/`)

Cross-platform audio I/O abstraction.

**Key Classes:**
- `Audio::Interface`: Audio interface abstraction
- `Audio::DevicePair`: Input/output device pair
- `Audio::Resampler`: Audio resampling
- `Audio::Mixer`: Multi-channel audio mixing
- `Audio::CoreAudio`: macOS/iOS implementation
- `Audio::DirectSound`: Windows DirectSound implementation
- `Audio::AndroidOboe`: Android Oboe implementation
- `Audio::Null`: Null/no-op implementation for testing

### 5. Helper Module (`src/engine/helper/`)

Utility classes and platform abstractions.

**Key Classes:**
- `HL::Types`: Type definitions and BiMap template
- `HL::Sync`: Threading primitives (Mutex, Event, etc.)
- `HL::ByteBuffer`: Binary data buffer
- `HL::VariantMap`: Key-value configuration storage
- `HL::Rtp`: RTP packet utilities
- `HL::IuUP`: Iu User Plane protocol (3G)
- `HL::NetworkSocket`: Network socket abstraction
- `HL::ThreadPool`: Thread pool implementation

## Code Style Guidelines

### Naming Conventions

1. **Classes**: PascalCase with module prefix
   - `AgentImpl`, `UserAgent`, `MT::AudioCodec`, `HL::Sync`

2. **Files**: Prefix indicates module
   - `Agent_*.cpp/h` - Agent module
   - `EP_*.cpp/h` - Endpoint module  
   - `MT_*.cpp/h` - Media module
   - `Audio_*.cpp/h` - Audio module
   - `HL_*.cpp/h` - Helper module

3. **Member Variables**: Hungarian notation with `m` prefix
   - `mAgentMutex`, `mSessionMap`, `mShutdown`

4. **Type Aliases**: `P` prefix for smart pointers
   - `PAccount`, `PSession`, `PVariantMap`

### File Header Template

All source files must include the MPL license header:

```cpp
/* Copyright(C) 2007-YYYY VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
```

### Include Order

1. Corresponding header file (for .cpp files)
2. Project headers (using `"..."`)
3. Third-party library headers
4. Standard library headers
5. System headers

### Platform Abstraction

Use preprocessor defines for platform-specific code:
- `TARGET_WIN` - Windows
- `TARGET_LINUX` - Linux
- `TARGET_OSX` - macOS
- `TARGET_ANDROID` - Android
- `TARGET_MUSL` - MUSL libc

## Configuration

### Compile-Time Configuration (`src/engine/engine_config.h`)

Key configuration constants:

```cpp
// Audio settings
#define AUDIO_SAMPLE_WIDTH 16
#define AUDIO_CHANNELS 1
#define AUDIO_SAMPLERATE 48000
#define AUDIO_RESAMPLER_QUALITY 1

// SIP settings
#define UA_REGISTRATION_TIME 3600
#define UA_MEDIA_PORT_START  20000
#define UA_MEDIA_PORT_FINISH 30000

// Codec payload types
#define MT_AMRNB_PAYLOADTYPE 112
#define MT_AMRWB_PAYLOADTYPE 96
#define MT_EVS_PAYLOADTYPE 127
#define MT_OPUS_CODEC_PT 106
```

### Runtime Configuration

Configuration is passed via `VariantMap` objects to the UserAgent:

```cpp
enum
{
    CONFIG_IPV4 = 0,            // Use IP4
    CONFIG_IPV6,                // Use IP6
    CONFIG_USERNAME,            // Username
    CONFIG_DOMAIN,              // Domain
    CONFIG_PASSWORD,            // Password
    CONFIG_STUNSERVER_NAME,     // STUN server hostname
    CONFIG_STUNSERVER_PORT,     // STUN server port
    CONFIG_TRANSPORT,           // 0=all, 1=UDP, 2=TCP, 3=TLS
    // ... see EP_Engine.h for full list
};
```

## Dependencies

### Git Submodules

The project uses Git submodules for some dependencies:
- `src/libs/resiprocate` - SIP stack (sevana branch)
- `src/libs/libsrtp` - SRTP library
- `src/libs/libraries` - Prebuilt platform libraries

To initialize:
```bash
git submodule update --init --recursive
```

### Third-Party Libraries

Prebuilt libraries are provided in `src/libs/libraries/`:
- OpenSSL 1.1 (crypto, SSL)
- Opus codec
- Opencore AMR (NB/WB)
- Boost (headers)
- PortAudio
- libevent2

## Testing

There is no dedicated test suite in the main project. Testing is typically done through:

1. Integration with the final application
2. The `run_ci.sh` script for build verification
3. Unit tests in individual library submodules (e.g., oboe)

### CI Build

```bash
./run_ci.sh
```

This configures with CMake and builds with make (2 parallel jobs).

## Security Considerations

1. **OpenSSL Integration**: Uses OpenSSL 1.1+ for TLS and certificate handling
2. **SRTP Support**: Media encryption via libsrtp
3. **Certificate Management**: Root certificates can be added at runtime via `addRootCert()`
4. **AMR/EVS Patents**: The project does NOT include patent licenses for AMR and EVS codecs. Users must acquire these independently.

## Integration Guide

### Basic Usage Pattern

1. Create `AgentImpl` instance
2. Send JSON commands via `command()` method
3. Poll for events via `waitForData()` and `read()`
4. Process JSON event responses

### Example Commands

- `config` - Configure the engine
- `start` - Start the engine
- `createAccount` - Create SIP account
- `startAccount` - Register account
- `createSession` - Create call session
- `startSession` - Make/accept call
- `waitForEvent` - Poll for events

## Development Workflow

1. **Making Changes**: Edit files in appropriate `src/engine/<module>/` directory
2. **Building**: Use `build_linux.py` for quick Linux builds
3. **Testing**: Integrate with test application or use existing CI
4. **Commits**: Follow existing commit message style (visible in git log)

## Common Tasks

### Adding a New Codec

1. Create `MT_<Codec>Codec.cpp/h` in `src/engine/media/`
2. Derive from `MT::Codec` base class
3. Register in `MT_CodecList.cpp`
4. Add to `src/CMakeLists.txt` if external library needed

### Adding Platform Audio Support

1. Create `Audio_<Platform>.cpp/h` in `src/engine/audio/`
2. Implement `Audio::Interface` methods
3. Add platform detection in `src/CMakeLists.txt`
4. Include in build conditionally

### Modifying Build Configuration

- Edit `src/CMakeLists.txt` for main build changes
- Edit `src/libs/libraries/platform_libs.cmake` for platform library paths
- Edit `src/engine/engine_config.h` for compile-time constants

## Notes

- The codebase uses C++20 features - ensure compiler compatibility
- Thread safety: Use `std::recursive_mutex` (e.g., `mAgentMutex`) for thread-safe access
- Memory management: Uses smart pointers (`std::shared_ptr`) extensively
- The JSON library is embedded in `src/libs/json/`
