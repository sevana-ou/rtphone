#pragma once

#include <set>
#include <map>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <sstream>

#include "config.h"
#include "MT_Codec.h"

#include "libevs/lib_com/prot.h"

namespace MT {


class EVSCodec : public Codec
{
public:
    // Emulated SDP config/media type parameters
    struct StreamParameters
    {
        bool mime = false;
        bool fh_only = false;           /*not use*/
        int CRMByte = -1/*CMR_OFF*/;    /*not use*/
        int br = 0;                     /*not use*/
        int bw = NB;
        int ptime = 20;
    };

public:
    class EVSFactory : public Factory
    {
    protected:
        StreamParameters _sp;

    public:
        EVSFactory(StreamParameters sp);
        const char* name() { return MT_EVS_CODECNAME; };
        int samplerate();
        int payloadType();
        PCodec create();
        int channels() { return 1; } //without support stereo audio transmition
    };

    EVSCodec();
    EVSCodec(const StreamParameters& sp);
    ~EVSCodec() override;

    const char* name() { return MT_EVS_CODECNAME; };
    int samplerate();
    int samplerate(int CodecMode); // DEC or ENC defined in cnst.h
    int pcmLength();
    int pcmLength(int CodecMode); // DEC or ENC defined in cnst.h
    int frameTime();
    int rtpLength();
    int encode(const void* input, int inputBytes, void* output, int outputCapacity);
    int decode(const void* input, int inputBytes, void* output, int outputCapacity);
    int plc(int lostFrames, void* output, int outputCapacity) ;

private:
    Decoder_State* st_dec;
    //Encoder_State_fx* st_enc;
    StreamParameters sp;
    void initDecoder(const StreamParameters& sp);
};

} // End of namespace
