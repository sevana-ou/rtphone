#include "MT_EvsCodec.h"

//bool 

EVSCodec::EVSFactory::EVSFactory(StreamParameters sp) : _sp(sp)
{}

int EVSCodec::EVSFactory::samplerate()
{
	return 0;
}

int EVSCodec::EVSFactory::payloadType()
{
	return 0;
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

    if ((st_dec = (Decoder_State*)malloc(sizeof(Decoder_State))) == NULL)
	{
		std::stringstream out;
		out << "Can not allocate memory for decoder state structure\n";
		throw std::out_of_range(out.str());
	}
    /*if ((st_enc = (Encoder_State*)malloc(sizeof(Encoder_State))) == NULL)
	{
		std::stringstream out;
		out << "Can not allocate memory for encoder state structure\n";
		throw std::out_of_range(out.str());
    }*/
	initDecoder(sp);
}

EVSCodec::~EVSCodec()
{
    if (st_dec)
    {
        destroy_decoder(st_dec);
        free(st_dec);
    }
}

int EVSCodec::samplerate()
{
    return st_dec->output_Fs;
}

int EVSCodec::samplerate(int CodecMode)
{
    return samplerate();
}

int EVSCodec::pcmLength()
{
    return samplerate() / 50;
}

int EVSCodec::pcmLength(int CodecMode)
{
    return pcmLength();
}

int EVSCodec::frameTime()
{
	return sp.ptime;
}

int EVSCodec::rtpLength()
{
	return 0;
}

int EVSCodec::encode(const void* input, int inputBytes, void* output, int outputCapacity)
{
	return 0;
}

int EVSCodec::decode(const void* input, int input_length, void* output, int outputCapacity)
{
    if (outputCapacity < L_FRAME8k)
    {
		std::stringstream out;
		out << "Buffer for pcm frame is to small\n";
		throw std::out_of_range(out.str());
	}

    std::string buffer;

	/*if we have FixedPayload without ToC*/
    if (FixedPayload_EVSPrimary.find(input_length * 8) != FixedPayload_EVSPrimary.end())
    {
        char c = rate2EVSmode(FixedPayload_EVSPrimary.find(input_length * 8)->second);
        /* Add ToC byte.
         * WARNING maybe it will be work incorrect with 56bit payload,
         * see 3GPP TS 26.445 Annex A, A.2.1.3 */
        buffer += c;
	}

    buffer += std::string(reinterpret_cast<const char*>(input), input_length);

    // Output buffer for 48 KHz
	float data[L_FRAME48k];
	int offset = 0;

    /* Decode process */
    size_t buffer_processed = 0;

    while (st_dec->bitstreamformat == G192 ? read_indices(st_dec, buffer.c_str(), buffer.size(), &buffer_processed, 0) : read_indices_mime(st_dec, buffer.c_str(), buffer.size(), &buffer_processed, 0))
	{
		if (st_dec->codec_mode == MODE1)
		{
            if (st_dec->Opt_AMR_WB)
			{
                amr_wb_dec(st_dec, data);
			}
			else
			{
                evs_dec(st_dec, data, FRAMEMODE_NORMAL);
			}
		}
		else
		{
            if (!st_dec->bfi)
			{
                evs_dec(st_dec, data, FRAMEMODE_NORMAL);
			}
			else
			{
                evs_dec(st_dec, data, FRAMEMODE_MISSING);
			}
		}

		/* convert 'float' output data to 'short' */
        syn_output(data, this->pcmLength(), static_cast<short*>(output) + offset);
		offset += this->pcmLength();
        if (st_dec->ini_frame < MAX_FRAME_COUNTER)
		{
            st_dec->ini_frame++;
		}
	}
    // std::fclose(temp);
	return 1;
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

	st_dec->bitstreamformat = G192;
	st_dec->amrwb_rfc4867_flag = -1;

	/*Set MIME type*/
	if (sp.mime) {
		st_dec->bitstreamformat = MIME;
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
