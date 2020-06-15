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

extern "C" {
    //#include "lib_dec/EvsRXlib.h"
    #include "libevs/lib_com/prot.h"
}

/*-------------------------------------------------------------------*
* rate2AMRWB_IOmode()
*
* lookup AMRWB IO mode
*-------------------------------------------------------------------*/

static Word16 rate2AMRWB_IOmode(
    Word32 rate                    /* i: bit rate */
);

/*Revers Function*/
extern Word16 AMRWB_IOmode2rate(
    Word32 mode                    
);

/*-------------------------------------------------------------------*
* rate2EVSmode()
*
* lookup EVS mode
*-------------------------------------------------------------------*/
extern Word16 rate2EVSmode(
    Word32 rate                    /* i: bit rate */
);

/*Revers Function*/
extern Word16 EVSmode2rate(
    Word32 mode                    
);

class Codec;
typedef std::shared_ptr<Codec> PCodec;

#define CMR_OFF	   -1
#define CMR_ON		0
#define CMR_ONLY	1


static const std::map<int, std::set<int>> BitrateToBandwidth_Tab{
    {5900,     {NB, WB}},
    {7200,     {NB, WB}},
    {8000,     {NB, WB}},
    {9600,	   {NB, WB, SWB}},
    {13200,	   {NB, WB, SWB}}, //if channel aware mode - WB, SWB
    {16400,    {NB, WB, SWB, FB}},
    {24400,    {NB, WB, SWB, FB}},
    {32000,    {WB, SWB, FB}},
    {48000,    {WB, SWB, FB}},
    {64000,    {WB, SWB, FB}},
    {96000,    {WB, SWB, FB}},
    {128000,   {WB, SWB, FB}}
};

/* Protected payload size/Fixed bitrate to EVS ()*/
static const std::map<int, int> Bitrate2PayloadSize_EVSAMR_WB{
       /*{bitrate, payload size}*/
             { SID_1k75,        56}, //AMR-WB I/O SIB
             {ACELP_6k60,       136},
             {ACELP_8k85,       184},
             {ACELP_12k65,      256},
             {ACELP_14k25,      288},
             {ACELP_15k85,      320},
             {ACELP_18k25,      368},
             {ACELP_19k85,      400},
             {ACELP_23k05,      464},
             {ACELP_23k85,      480}
}; 

static const std::map<int, int> Bitrate2PayloadSize_EVS{
        /*{bitrate, payload size}*/
            {FRAME__NO_DATA,    0},
            {SID_2k40,          48}, //EVS Primary SID
            {PPP_NELP_2k80,     56},  //special for full header
            {ACELP_7k20,        144},
            {ACELP_8k00,        160},
            {ACELP_9k60,        192},
            {ACELP_13k20,       264},
            {ACELP_16k40,       328},
            {ACELP_24k40,       488},
            {ACELP_32k,         640},
            {ACELP_48k,         960},
            {ACELP_64k,         1280},
            {HQ_96k,            1920},
            {HQ_128k,           2560}
};

/* Protected payload size/Fixed bitrate to EVS ()*/
static const std::map<int, int> FixedPayload_EVSPrimary{
    /*{payload size , bitrate}*/
                {48,        2400}, //EVS Primary SID
                {56,        2800},  //special for full header
                {144,       7200},
                {160,       8000},
                {192,       9600},
                {264,       13200},
                {328,       16400},
                {488,       24400},
                {640,       32000},
                {960,       48000},
                {1280,      64000},
                {1920,      96000},
                {2560,      128000}
};

static const std::map<int, int> FixedPayload_EVSAMR_WB{
    /*{payload size , bitrate}*/
                {136,       6600},
                {184,       8850},
                {256,       12650},
                {288,       14250},
                {320,       15850},
                {368,       18250},
                {400,       19850},
                {464,       23050},
                {480,       23850}
};

struct StreamParameters {
    bool mime = false;
    bool fh_only = false; /*not use*/
    int CRMByte = CMR_OFF;/*not use*/
    int br = 0;           /*not use*/
    int bw = NB;
    int ptime = 20;
}; //Emulated SDP config/meadia type parameters

class Codec
{
public:
    class Factory
    {
    public:
        virtual ~Factory() {}
        virtual const char* name() = 0;
        virtual int samplerate() = 0;
        virtual int payloadType() = 0;
        virtual PCodec create() = 0;

        virtual int channels() = 0;
    };
    virtual ~Codec() {}
    virtual const char* name() = 0;
    virtual int samplerate() = 0;
    virtual float timestampUnit() { return float(1.0 / samplerate()); }
    virtual int pcmLength() = 0;
    virtual int frameTime() = 0;
    virtual int rtpLength() = 0;
    virtual int channels() { return 1; }
    virtual int encode(const void* input, int inputBytes, void* output, int outputCapacity) = 0;
    virtual int decode(const void* input, int inputBytes, void* output, int outputCapacity) = 0;
    virtual int plc(int lostFrames, void* output, int outputCapacity) = 0;

    // Returns size of codec in memory
    virtual int getSize() const { return 0; };
};

class EVSCodec : public Codec
{
private:
    Decoder_State* st_dec;
    //Encoder_State_fx* st_enc;
    StreamParameters sp;

public:
    class EVSFactory : public Factory
    {
    protected:
        StreamParameters _sp;

    public:
        EVSFactory(StreamParameters sp);
        const char* name() { return "EVS"; };
        int samplerate();
        int payloadType();
        PCodec create();
        int channels() { return 1; } //without support stereo audio transmition
    };

    EVSCodec();
    EVSCodec(const StreamParameters& sp);
    ~EVSCodec() override;

    const char* name() { return "EVS"; };
    int samplerate();
    int samplerate(int CodecMode); // DEC or ENC defined in cnst.h
    int pcmLength();
    int pcmLength(int CodecMode); // DEC or ENC defined in cnst.h
    int frameTime();
    int rtpLength();
    int encode(const void* input, int inputBytes, void* output, int outputCapacity);
    int decode(const void* input, int inputBytes, void* output, int outputCapacity);
    int plc(int lostFrames, void* output, int outputCapacity) ;
    void initDecoder(const StreamParameters& sp);
};
