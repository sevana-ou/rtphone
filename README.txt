# RTPhone Platform

RTPhone is a comprehensive real-time communication (RTC) platform that provides a complete software stack for building VoIP/SIP-based communication applications. Developed by VoIP Objects (Sevana), RTPhone delivers production-ready voice communication capabilities with extensive codec support and cross-platform compatibility.

## Overview

RTPhone serves as a static library (`librtphone.a`) that can be integrated into larger telephony and communication systems. It provides a JSON-based command interface for easy integration and control, making it suitable for building softphones, PBX systems, WebRTC gateways, and carrier-grade voice solutions.

## Key Features

### Audio Codec Support
RTPhone supports an extensive range of audio codecs:

**Standard Codecs:**
- G.711 (A-law/¼-law)
- G.722 (16kHz wideband)
- G.729
- GSM (Full Rate, Half Rate, Enhanced Full Rate)
- iLBC (20ms/30ms)
- ISAC (16kHz/32kHz)

**Advanced Codecs:**
- AMR-NB/AMR-WB (Adaptive Multi-Rate Narrowband/Wideband) - please be aware - there is no patents for AMR codecs usage included ! You should acquire them on your own.
- EVS (Enhanced Voice Services) - 3GPP's latest codec. Again - please be aware - there is no patents for EVS codec usage included ! You should acquire them on your own.
- Opus - Modern low-latency codec
- Speex (with acoustic echo cancellation)

**Codec Features:**
- Bandwidth-efficient and octet-aligned modes
- IuUP (Iu User Plane) protocol support for 3G networks
- Dynamic codec switching
- Packet loss concealment (PLC)
- Comfort noise generation (CNG)

### Network & Protocol Support

**SIP Features:**
- Full SIP 2.0 implementation via reSIProcate
- Multiple transport protocols (UDP, TCP, TLS)
- Registration, authentication, and session management
- SIP MESSAGE, presence, and REFER support

**Media Transport:**
- RTP/RTCP for media streaming
- SRTP for secure media
- ICE for NAT traversal with STUN/TURN support
- WebRTC integration components
- IPv4 and IPv6 support

### Cross-Platform Audio Support
- DirectSound/WMME (Windows)
- Core Audio (macOS/iOS)
- ALSA/PulseAudio (Linux)
- Oboe (Android) for low-latency audio
- PortAudio fallback support

### Audio Quality Features
- 48kHz sample rate support
- Acoustic Echo Cancellation (AEC)
- Audio resampling and format conversion
- Multi-channel audio mixing
- Perceptual Voice Quality Assessment (PVQA)

## Architecture

The platform is organized into several core modules:

- **Engine/Agent**: JSON-based command interface
- **Engine/Endpoint**: SIP user agent implementation
- **Engine/Media**: Audio codec management and processing
- **Engine/Audio**: Cross-platform audio I/O handling
- **Engine/Helper**: Utility functions (networking, logging, threading)

## Supported Platforms

- Linux (x64, ARM/Raspberry Pi)
- Windows (32/64-bit)
- macOS
- Android (with Oboe integration)
- iOS

## Building

RTPhone uses a CMake-based build system with cross-compilation support:

### Linux
```bash
python3 build_linux.py
```

### Android
```bash
python3 build_android.py
# or
./build_android.sh
```

### Dependencies
- CMake 3.10+
- OpenSSL 1.1+
- Boost libraries
- Platform-specific audio libraries

## Recent Updates

Recent development has focused on:
- AMR codec parsing and decoding improvements
- Octet-aligned mode fixes for AMR-WB
- RTP SSRC handling enhancements
- Build system optimizations
- Code modernization to C++20

## Use Cases

RTPhone is good for building:
- VoIP softphones and mobile applications
- PBX and telephony server systems
- RTP proxies
- Carrier-grade voice communication platforms
- 3GPP/IMS-compliant systems

## Security

RTPhone includes comprehensive security features:
- OpenSSL 1.1 integration for encryption
- TLS transport layer security
- SRTP media encryption
- Certificate management support

## Integration

The platform provides a developer-friendly interface with:
- Event-driven architecture
- Comprehensive logging system
- Modern C++20 codebase

For detailed integration instructions and API documentation, please refer to the source code and header files in the `/src/engine/` directory.

## License

Our source code is licensed under the MPL license. Naturally, any third-party components we use are subject to their respective licenses.
