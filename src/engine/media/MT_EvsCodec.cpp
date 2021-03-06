#include "MT_EvsCodec.h"



/*-------------------------------------------------------------------*
* rate2AMRWB_IOmode()
*
* lookup AMRWB IO mode
*-------------------------------------------------------------------*/
namespace evs {
extern Word16 rate2AMRWB_IOmode(
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

#define CMR_OFF	   -1
#define CMR_ON		0
#define CMR_ONLY	1

}

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

namespace MT
{

EVSCodec::EVSFactory::EVSFactory(StreamParameters& sp) : _sp(sp)
{}

int EVSCodec::EVSFactory::samplerate()
{
    switch (_sp.bw)
    {
    case 0:     return 8000;
    case 1:     return 16000;
    case 2:     return 32000;
    case 3:     return 48000;
    }

    return 0;
}

int EVSCodec::EVSFactory::payloadType()
{
    return _sp.ptype;
}

PCodec EVSCodec::EVSFactory::create()
{
    return std::static_pointer_cast<Codec>(std::make_shared<EVSCodec>(_sp));
}

EVSCodec::EVSCodec(): EVSCodec(StreamParameters())
{
}

EVSCodec::EVSCodec(const StreamParameters &sp)
{
	EVSCodec::sp = sp;

    if ((st_dec = reinterpret_cast<evs::Decoder_State*>(malloc(sizeof(evs::Decoder_State)))) == nullptr)
        throw std::bad_alloc();

    initDecoder(sp);
}

EVSCodec::~EVSCodec()
{
    if (st_dec)
    {
        destroy_decoder(st_dec);
        free(st_dec); st_dec = nullptr;
    }
}

int EVSCodec::samplerate()
{
    return st_dec->output_Fs;
}

int EVSCodec::pcmLength()
{
    return samplerate() / 50 * 2;
}

int EVSCodec::frameTime()
{
	return sp.ptime;
}

int EVSCodec::rtpLength()
{
    // Variable sized codec - bitrate can be changed during the call
	return 0;
}

int EVSCodec::encode(const void* input, int inputBytes, void* output, int outputCapacity)
{
    // Encoding is not supported yet.
	return 0;
}

int EVSCodec::decode(const void* input, int input_length, void* output, int outputCapacity)
{
    if (outputCapacity < pcmLength())
        return 0;

    std::string buffer;

    // Check if we get payload with CMR
    auto payload_iter = FixedPayload_EVSPrimary.find((input_length - 2) * 8);
    if (payload_iter == FixedPayload_EVSPrimary.end())
    {
        // Check if we get payload with ToC and without CMR
        payload_iter = FixedPayload_EVSPrimary.find((input_length - 1) * 8);
        if (payload_iter == FixedPayload_EVSPrimary.end())
        {
            // Maybe there is no ToC ?
            payload_iter = FixedPayload_EVSPrimary.find(input_length * 8);
            if (payload_iter == FixedPayload_EVSPrimary.end())
            {
                // Bad payload size at all
                return 0;
            }
            /* Add ToC byte.
             * WARNING maybe it will be work incorrect with 56bit payload,
             * see 3GPP TS 26.445 Annex A, A.2.1.3 */
            char c = evs::rate2EVSmode(FixedPayload_EVSPrimary.find(input_length * 8)->second);
            buffer += c;
            buffer += std::string(reinterpret_cast<const char*>(input), input_length);
        }
        else
            buffer = std::string(reinterpret_cast<const char*>(input), input_length);
    }
    else
        // Skip CMR byte
        buffer = std::string(reinterpret_cast<const char*>(input) + 1, input_length-1);


    // Output buffer for 48 KHz
	float data[L_FRAME48k];
	int offset = 0;

    /* Decode process */
    size_t buffer_processed = 0;

    while (st_dec->bitstreamformat == evs::G192 ? read_indices(st_dec, buffer.c_str(), buffer.size(), &buffer_processed, 0) : read_indices_mime(st_dec, buffer.c_str(), buffer.size(), &buffer_processed, 0))
	{
		if (st_dec->codec_mode == MODE1)
		{
            if (st_dec->Opt_AMR_WB)
			{
                amr_wb_dec(st_dec, data);
			}
			else
			{
                evs_dec(st_dec, data, evs::FRAMEMODE_NORMAL);
			}
		}
		else
		{
            if (!st_dec->bfi)
			{
                evs_dec(st_dec, data, evs::FRAMEMODE_NORMAL);
			}
			else
			{
                evs_dec(st_dec, data, evs::FRAMEMODE_MISSING);
			}
		}

		/* convert 'float' output data to 'short' */
        evs::syn_output(data, static_cast<short>(pcmLength() / 2), static_cast<short*>(output) + offset);
        offset += pcmLength() / 2;
        if (st_dec->ini_frame < MAX_FRAME_COUNTER)
		{
            st_dec->ini_frame++;
		}
	}

    return pcmLength();
}

int EVSCodec::plc(int lostFrames, void* output, int outputCapacity)
{
	return 0;
}

void EVSCodec::initDecoder(const StreamParameters& sp)
{
	/* set to NULL, to avoid reading of uninitialized memory in case of early abort */
    st_dec->cldfbAna = st_dec->cldfbBPF = st_dec->cldfbSyn = nullptr;
    st_dec->hFdCngDec = nullptr;
	st_dec->codec_mode = 0; /* unknown before first frame */
    st_dec->Opt_AMR_WB = 0;
    st_dec->Opt_VOIP = 0;

    st_dec->bitstreamformat = evs::G192;
	st_dec->amrwb_rfc4867_flag = -1;

	/*Set MIME type*/
	if (sp.mime) {
        st_dec->bitstreamformat = evs::MIME;
		st_dec->amrwb_rfc4867_flag = 0;
	}
	/*Set Bandwidth*/
	switch (sp.bw) {
	case NB :
        st_dec->output_Fs = 8000;
		break;
	case WB:
        st_dec->output_Fs = 16000;
		break;
	case SWB:
        st_dec->output_Fs = 32000;
		break;
	case FB:
        st_dec->output_Fs = 48000;
		break;
	}
    // st_enc->input_Fs = st_dec->output_Fs; //TODO: remove when func initEncoder() is added
   /*------------------------------------------------------------------------------------------*
	* Allocate memory for static variables
	* Decoder initialization
	*------------------------------------------------------------------------------------------*/
	
    // TODO: add ability of reinit Decoder_state, may be need use destroy_decoder()
    init_decoder(st_dec);
    reset_indices_dec(st_dec);

    srand(static_cast<unsigned int>(time(nullptr)));
}

} // end of namespace MT
