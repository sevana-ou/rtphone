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
    void initDecoder(const StreamParameters& sp);
};

} // End of namespace

#endif
