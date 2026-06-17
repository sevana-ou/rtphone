#ifndef __MT_EVS_CODEC_H
#define __MT_EVS_CODEC_H

#include "../engine_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "MT_Codec.h"

#include "libevs/lib_com/prot.h"

namespace MT {

class EVSCodec : public Codec
{
public:
    // Emulated SDP config/media type parameters
    struct StreamParameters
    {
        bool mime = false;              /* encoding */
        bool fh_only = false;           /* not use*/
        int CRMByte = -1/*CMR_OFF*/;    /* not use*/
        int br = 0;                     /* not use*/
        int bw = NB;                    /* bandwidth */
        int ptime = 20;                 /* ptime */
        int ptype = -1;                 /* payload type */
    };

public:
    class EVSFactory : public Factory
    {
    protected:
        StreamParameters _sp;

    public:
        EVSFactory(StreamParameters& sp);
        const char* name() { return MT_EVS_CODECNAME; }
        int samplerate();
        int payloadType();
        PCodec create();
        int channels() { return 1; } //without support stereo audio transmition
    };

    EVSCodec();
    EVSCodec(const StreamParameters& sp);
    ~EVSCodec() override;

    Info info() override;

    EncodeResult encode(std::span<const uint8_t> input, std::span<uint8_t> output) override;
    DecodeResult decode(std::span<const uint8_t> input, std::span<uint8_t> output) override;
    size_t       plc(int lostFrames, std::span<uint8_t> output) override;

private:
    evs::Decoder_State* st_dec = nullptr;
    StreamParameters sp;

    // Output sample rate, derived from the negotiated bandwidth (sp.bw) at
    // construction. Cached so info()/samplerate()/pcmLength() work for network-MOS
    // metadata without allocating the (large) EVS decoder state - see ensureDecoder.
    int mOutputFs = 0;

    void initDecoder(const StreamParameters& sp);

    // Allocate + initialize the EVS decoder state lazily on first decode().
    // Network-MOS-only streams resolve metadata but never decode, so they never
    // pay for the EVS decoder (Decoder_State + CLDFB/FD-CNG sub-allocations).
    void ensureDecoder();

    // Maps an EVS bandwidth (NB/WB/SWB/FB) to its output sample rate in Hz.
    static int outputFsFromBw(int bw);
};

} // End of namespace

#endif
