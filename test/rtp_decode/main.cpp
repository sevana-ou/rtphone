/* Copyright(C) 2007-2026 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// rtp_decode — read an rtpdump file, decode RTP with a given codec, write WAV.
//
// Usage:
//   rtp_decode <input.rtp> <output.wav> --codec <name> [--pt <N>] [--rate <N>] [--channels <N>]

#include "helper/HL_Rtp.h"
#include "media/MT_CodecList.h"
#include "media/MT_Codec.h"
#include "audio/Audio_WavFile.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <stdexcept>

// ---------------------------------------------------------------------------
// CLI helpers
// ---------------------------------------------------------------------------

static void usage(const char* progname)
{
    fprintf(stderr,
        "Usage: %s <input.rtp> <output.wav> --codec <name> [--pt <N>] [--rate <N>] [--channels <N>]\n"
        "\n"
        "Codecs: pcmu pcma g722 g729 opus gsm gsmhr gsmefr\n"
        "        amrnb amrwb amrnb-bwe amrwb-bwe evs ilbc20 ilbc30 isac16 isac32\n"
        "\n"
        "Options:\n"
        "  --codec <name>    Codec name (required)\n"
        "  --pt <N>          Override RTP payload type\n"
        "  --rate <N>        Sample rate hint for Opus (default 48000)\n"
        "  --channels <N>    Channel count hint for Opus (default 2)\n",
        progname);
}

static const char* getOption(int argc, char* argv[], const char* name)
{
    for (int i = 1; i < argc - 1; ++i) {
        if (strcmp(argv[i], name) == 0)
            return argv[i + 1];
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// Default payload types for codecs without a fixed standard PT
// ---------------------------------------------------------------------------
struct CodecDefaults
{
    const char* name;
    int defaultPt;      // -1 = must be specified via --pt
    bool needsPt;       // true if --pt is required when no default exists
};

static const CodecDefaults kCodecTable[] = {
    { "pcmu",   0,   false },
    { "pcma",   8,   false },
    { "g722",   9,   false },
    { "g729",   18,  false },
    { "gsm",    3,   false },
    { "opus",   106, false },
    { "amrnb",     -1,  true  },
    { "amrwb",     -1,  true  },
    { "amrnb-bwe", -1,  true  },
    { "amrwb-bwe", -1,  true  },
    { "gsmhr",  -1,  true  },
    { "gsmefr", 126, false },
    { "evs",    127, false },
    { "ilbc20", -1,  true  },
    { "ilbc30", -1,  true  },
    { "isac16", -1,  true  },
    { "isac32", -1,  true  },
};

static const CodecDefaults* findCodecDefaults(const std::string& name)
{
    for (auto& c : kCodecTable)
        if (name == c.name)
            return &c;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Build CodecList::Settings for the requested codec
// ---------------------------------------------------------------------------
static MT::CodecList::Settings buildSettings(const std::string& codecName, int pt,
                                              int opusRate, int opusChannels)
{
    MT::CodecList::Settings s;

    if (codecName == "opus") {
        s.mOpusSpec.push_back(MT::CodecList::Settings::OpusSpec(pt, opusRate, opusChannels));
    } else if (codecName == "gsm") {
        s.mGsmFrPayloadType = pt;
    } else if (codecName == "gsmhr") {
        s.mGsmHrPayloadType = pt;
    } else if (codecName == "gsmefr") {
        s.mGsmEfrPayloadType = pt;
    } else if (codecName == "amrnb") {
        s.mAmrNbOctetPayloadType.insert(pt);
    } else if (codecName == "amrwb") {
        s.mAmrWbOctetPayloadType.insert(pt);
    } else if (codecName == "amrnb-bwe") {
        s.mAmrNbPayloadType.insert(pt);
    } else if (codecName == "amrwb-bwe") {
        s.mAmrWbPayloadType.insert(pt);
    } else if (codecName == "evs") {
        MT::CodecList::Settings::EvsSpec ev;
        ev.mPayloadType = pt;
        s.mEvsSpec.push_back(ev);
    } else if (codecName == "ilbc20") {
        s.mIlbc20PayloadType = pt;
    } else if (codecName == "ilbc30") {
        s.mIlbc30PayloadType = pt;
    } else if (codecName == "isac16") {
        s.mIsac16KPayloadType = pt;
    } else if (codecName == "isac32") {
        s.mIsac32KPayloadType = pt;
    }
    // pcmu, pcma, g722, g729 — fixed PT, auto-registered by CodecList::init()

    return s;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    if (argc < 4) {
        usage(argv[0]);
        return 1;
    }

    const char* inputPath  = argv[1];
    const char* outputPath = argv[2];

    const char* codecArg = getOption(argc, argv, "--codec");
    if (!codecArg) {
        fprintf(stderr, "Error: --codec is required\n\n");
        usage(argv[0]);
        return 1;
    }
    std::string codecName = codecArg;

    const auto* defaults = findCodecDefaults(codecName);
    if (!defaults) {
        fprintf(stderr, "Error: unknown codec '%s'\n\n", codecArg);
        usage(argv[0]);
        return 1;
    }

    // Resolve payload type
    int pt = defaults->defaultPt;
    const char* ptArg = getOption(argc, argv, "--pt");
    if (ptArg) {
        pt = atoi(ptArg);
    } else if (defaults->needsPt) {
        fprintf(stderr, "Error: --pt is required for codec '%s'\n\n", codecArg);
        usage(argv[0]);
        return 1;
    }

    int opusRate     = 48000;
    int opusChannels = 2;
    const char* rateArg = getOption(argc, argv, "--rate");
    if (rateArg)
        opusRate = atoi(rateArg);
    const char* chArg = getOption(argc, argv, "--channels");
    if (chArg)
        opusChannels = atoi(chArg);

    // -----------------------------------------------------------------------
    // 1. Load rtpdump
    // -----------------------------------------------------------------------
    RtpDump dump(inputPath);
    try {
        dump.load();
    } catch (const std::exception& e) {
        fprintf(stderr, "Error loading rtpdump '%s': %s\n", inputPath, e.what());
        return 1;
    }

    if (dump.count() == 0) {
        fprintf(stderr, "No packets in '%s'\n", inputPath);
        return 1;
    }
    fprintf(stderr, "Loaded %zu packets from '%s'\n", dump.count(), inputPath);

    // -----------------------------------------------------------------------
    // 2. Create codec
    // -----------------------------------------------------------------------
    auto settings = buildSettings(codecName, pt, opusRate, opusChannels);
    MT::CodecList codecList(settings);
    MT::PCodec codec = codecList.createCodecByPayloadType(pt);
    if (!codec) {
        fprintf(stderr, "Error: could not create codec for payload type %d\n", pt);
        return 1;
    }

    auto codecInfo = codec->info();
    fprintf(stderr, "Codec: %s  samplerate=%d  channels=%d  pcmLength=%d  frameTime=%dms\n",
            codecInfo.mName.c_str(), codecInfo.mSamplerate, codecInfo.mChannels,
            codecInfo.mPcmLength, codecInfo.mFrameTime);

    // -----------------------------------------------------------------------
    // 3. Open WAV writer
    // -----------------------------------------------------------------------
    Audio::WavFileWriter writer;
    if (!writer.open(outputPath, codecInfo.mSamplerate, codecInfo.mChannels)) {
        fprintf(stderr, "Error: could not open WAV file '%s' for writing\n", outputPath);
        return 1;
    }

    // -----------------------------------------------------------------------
    // 4. Decode loop
    // -----------------------------------------------------------------------
    std::vector<uint8_t> pcmBuffer(65536);
    size_t totalDecodedBytes = 0;
    size_t packetsDecoded = 0;
    size_t packetsSkipped = 0;

    for (size_t i = 0; i < dump.count(); ++i) {
        const auto& rawData = dump.rawDataAt(i);

        // Verify it's actually RTP
        if (!RtpHelper::isRtp(rawData.data(), rawData.size())) {
            ++packetsSkipped;
            continue;
        }

        // Parse RTP to get payload
        jrtplib::RTPPacket& rtpPacket = dump.packetAt(i);

        // Check payload type matches what we expect
        int pktPt = rtpPacket.GetPayloadType();
        if (pktPt != pt) {
            ++packetsSkipped;
            continue;
        }

        uint8_t* payloadData = rtpPacket.GetPayloadData();
        size_t   payloadLen  = rtpPacket.GetPayloadLength();

        if (!payloadData || payloadLen == 0) {
            ++packetsSkipped;
            continue;
        }

        std::span<const uint8_t> input(payloadData, payloadLen);
        std::span<uint8_t> output(pcmBuffer.data(), pcmBuffer.size());

        try {
            auto result = codec->decode(input, output);
            if (result.mDecoded > 0) {
                writer.write(pcmBuffer.data(), result.mDecoded);
                totalDecodedBytes += result.mDecoded;
                ++packetsDecoded;
            }
        } catch (const std::exception& e) {
            fprintf(stderr, "Warning: decode error at packet %zu: %s\n", i, e.what());
            ++packetsSkipped;
        }
    }

    // -----------------------------------------------------------------------
    // 5. Close WAV and print summary
    // -----------------------------------------------------------------------
    writer.close();

    size_t totalSamples = totalDecodedBytes / (sizeof(int16_t) * codecInfo.mChannels);
    double durationSec  = (codecInfo.mSamplerate > 0)
                          ? static_cast<double>(totalSamples) / codecInfo.mSamplerate
                          : 0.0;

    fprintf(stderr, "\nDone.\n");
    fprintf(stderr, "  Packets decoded: %zu\n", packetsDecoded);
    fprintf(stderr, "  Packets skipped: %zu\n", packetsSkipped);
    fprintf(stderr, "  Decoded PCM:     %zu bytes\n", totalDecodedBytes);
    fprintf(stderr, "  Duration:        %.3f seconds\n", durationSec);
    fprintf(stderr, "  Output:          %s\n", outputPath);

    return 0;
}
