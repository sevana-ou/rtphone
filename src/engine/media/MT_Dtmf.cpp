/* Copyright(C) 2007-2017 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../config.h"
#include "MT_Dtmf.h"


#ifdef TARGET_WIN
# include <WinSock2.h>
#endif
#include <assert.h>
#include <math.h>
#include <memory.h>

using namespace MT;

void DtmfBuilder::buildRfc2833(int tone, int duration, int volume, bool endOfEvent, void* output)
{
  assert(duration);
  assert(output);
  assert(tone);

  unsigned char toneValue = 0;
  if (tone >= '0' && tone <='9')
    toneValue = tone - '0';
  else
  if (tone >= 'A' && tone <='D' )
    toneValue = tone - 'A' + 12;
  else
  if (tone == '*')
    toneValue = 10;
  else
  if (tone == '#')
    toneValue = 11;

  char* packet = (char*)output;

  packet[0] = toneValue;
  packet[1] = 1 | (volume << 2);
  if (endOfEvent)
    packet[1] |= 128;
  else
    packet[1] &= 127;

  unsigned short durationValue = htons(duration);
  memcpy(packet + 2, &durationValue, 2);
}

#pragma region Inband DTMF support
#include <math.h>
#ifndef TARGET_WIN
#	include <ctype.h>
# if !defined(TARGET_ANDROID) && !defined(TARGET_WIN)
#	include <xlocale.h>
# endif
#endif

static bool sineTabInit = false;

static double sinetab[1 << 11];
static inline double sine(unsigned int ptr)
{ 
  return sinetab[ptr >> (32-11)]; 
}

#define TWOPI	      (2.0 * 3.14159265358979323846)
#define MAXSTR	     512
#define SINEBITS     11
#define SINELEN	     (1 << SINEBITS)
#define TWO32	      4294967296.0	/* 2^32 */


static double amptab[2] = { 8191.75, 16383.5 };
static inline int ifix(double x) 
{ 
  return (x >= 0.0) ? (int) (x+0.5) : (int) (x-0.5); 
}

// given frequency f, return corresponding phase increment 
static inline int phinc(double f)
{ 
  return ifix(TWO32 * f / (double) AUDIO_SAMPLERATE);
}

static char dtmfSymbols[16] = {
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    'A',
    'B',
    'C',
    'D',
    '*',
    '#'
};

char PDTMFEncoder_DtmfChar(int i)
{
  
  if (i < 16)
    return dtmfSymbols[i];
  else
    return 0;
}

// DTMF frequencies as per http://www.commlinx.com.au/DTMF_frequencies.htm

static double dtmfFreqs[16][2] = {
  { 941.0, 1336.0 },  // 0
  { 697.0, 1209.0 },  // 1
  { 697.0, 1336.0 },  // 2
  { 697.0, 1477.0 },  // 3
  { 770.0, 1209.0 },  // 4
  { 770.0, 1336.0 },  // 5
  { 770.0, 1477.0 },  // 6
  { 852.0, 1209.0 },  // 7
  { 852.0, 1336.0 },  // 8
  { 852.0, 1477.0 },  // 9
  { 697.0, 1633.0 },  // A
  { 770.0, 1633.0 },  // B
  { 852.0, 1633.0 },  // C
  { 941.0, 1633.0 },  // D
  { 941.0, 1209.0 },  // *
  { 941.0, 1477.0 }   // #
};


static Mutex LocalDtmfMutex;

void PDTMFEncoder_MakeSineTable()
{ 
  Lock lock(LocalDtmfMutex);
  
  if (!sineTabInit) {
    for (int k = 0; k < SINELEN; k++) { 
      double th = TWOPI * (double) k / (double) SINELEN;
      double v = sin(th);
      sinetab[k] = v;
    }
    sineTabInit = true;
  }
}

void PDTMFEncoder_AddTone(double f1, double f2, unsigned ms1, unsigned ms2, unsigned rate, short* result)
{
  int ak = 0;

  PDTMFEncoder_MakeSineTable();

  int dataPtr = 0;

  double amp = amptab[ak];
  int phinc1 = phinc(f1), phinc2 = phinc(f2);
  int ns1 = ms1 * (rate/1000);
  int ns2 = ms2 * (rate/1000);
  unsigned int ptr1 = 0, ptr2 = 0;
  ptr1 += phinc1 * ns1;
  ptr2 += phinc2 * ns1;

  for (int n = ns1; n < ns2; n++) { 

    double val = amp * (sine(ptr1) + sine(ptr2));
    int ival = ifix(val);
    if (ival < -32768)
      ival = -32768;
    else if (val > 32767) 
      ival = 32767;

    result[dataPtr++] = ival / 2;

    ptr1 += phinc1; 
    ptr2 += phinc2;
  }
}

void PDTMFEncoder_AddTone(char _digit, unsigned startTime, unsigned finishTime, unsigned rate, short* result)
{
  char digit = (char)toupper(_digit); 
  if ('0' <= digit && digit <= '9')
    digit = digit - '0';

  else if ('A' <= digit && digit <= 'D')
    digit = digit + 10 - 'A';

  else if (digit == '*')
    digit = 14;

  else if (digit == '#')
    digit = 15;

  else
    return ;

  PDTMFEncoder_AddTone(dtmfFreqs[(int)digit][0], dtmfFreqs[(int)digit][1], startTime, finishTime, rate, result);
}

#pragma endregion

void DtmfBuilder::buildInband(int tone, int startTime, int finishTime, int rate, short* buf)
{
  PDTMFEncoder_AddTone(tone, startTime, finishTime, rate, buf);
}

#pragma region DtmfContext
DtmfContext::DtmfContext()
:mType(Dtmf_Rfc2833)
{
}

DtmfContext::~DtmfContext()
{
}

void DtmfContext::setType(Type t)
{
  mType = t;
}

DtmfContext::Type DtmfContext::type()
{
  return mType;
}

void DtmfContext::startTone(int tone, int volume)
{
  Lock l(mGuard);
  
  // Stop current tone if needed
  if (mQueue.size())
    stopTone();
  mQueue.push_back(Dtmf(tone, volume, 0));
}

void DtmfContext::stopTone()
{
  Lock l(mGuard);
  
  // Switch to "emit 3 terminating packets" mode
  if (mQueue.size())
  {
    switch (mType)
    {
    case Dtmf_Rfc2833:
      mQueue.front().mStopped = true;
      mQueue.erase(mQueue.begin());      
      break;

    case Dtmf_Inband:
      if (!mQueue.front().mFinishCount)
        mQueue.front().mFinishCount = MT_DTMF_END_PACKETS;
      break;
    }
  }
}

void DtmfContext::queueTone(int tone, int volume, int duration)
{
  Lock l(mGuard);
  mQueue.push_back(Dtmf(tone, volume, duration));
}

void DtmfContext::clearAllTones()
{
  Lock l(mGuard);
  mQueue.clear();
}

bool DtmfContext::getInband(int milliseconds, int rate, ByteBuffer& output)
{
  Lock l(mGuard);

  if (!mQueue.size() || mType != Dtmf_Inband)
    return false;

  // 
  Dtmf& d = mQueue.front();
  
  output.resize(milliseconds * rate / 1000 * 2);
  DtmfBuilder::buildInband(d.mTone, d.mCurrentTime, d.mCurrentTime + milliseconds, rate, (short*)output.mutableData());
  d.mCurrentTime += milliseconds;
  return true;
}

bool DtmfContext::getRfc2833(int milliseconds, ByteBuffer& output, ByteBuffer& stopPacket)
{
  Lock l(mGuard);
  if (!mQueue.size() || mType != Dtmf_Rfc2833)
    return false;

  Dtmf& d = mQueue.front();
  // See if tone has enough duration to produce another packet
  if (d.mDuration > 0)
  {
    // Emit rfc2833 packet
    output.resize(4);
    DtmfBuilder::buildRfc2833(d.mTone, milliseconds, d.mVolume, false, output.mutableData());
    d.mDuration -= milliseconds;
    if(d.mDuration <= 0)
      d.mStopped = true;
  }
  else
  if (!d.mStopped)
  {
    output.resize(4);
    DtmfBuilder::buildRfc2833(d.mTone, milliseconds, d.mVolume, false, output.mutableData());
  }
  else
    output.clear();
  
  if (d.mStopped)
  {
    stopPacket.resize(4);
    DtmfBuilder::buildRfc2833(d.mTone, milliseconds, d.mVolume, true, stopPacket.mutableData());
  }
  else
    stopPacket.clear();
  
  if (d.mStopped)
    mQueue.erase(mQueue.begin());

  return true;
}

typedef struct
{
    float v2;
    float v3;
    float fac;
} goertzel_state_t;

#define MAX_DTMF_DIGITS 128

typedef struct
{
    int               hit1;
    int               hit2;
    int               hit3;
    int               hit4;
    int               mhit;

    goertzel_state_t  row_out[4];
    goertzel_state_t  col_out[4];
    goertzel_state_t  row_out2nd[4];
    goertzel_state_t  col_out2nd[4];
    goertzel_state_t  fax_tone;
    goertzel_state_t  fax_tone2nd;
    float             energy;
    
    int               current_sample;
    char              digits[MAX_DTMF_DIGITS + 1];
    int               current_digits;
    int               detected_digits;
    int               lost_digits;
    int               digit_hits[16];
    int               fax_hits;
} dtmf_detect_state_t;

typedef struct
{
    float fac;
} tone_detection_descriptor_t;

void  zap_goertzel_update(goertzel_state_t *s, int16_t x[], int samples);
float zap_goertzel_result (goertzel_state_t *s);
void  zap_dtmf_detect_init(dtmf_detect_state_t *s);
int   zap_dtmf_detect(dtmf_detect_state_t *s, int16_t amp[], int samples, int isradio);
int   zap_dtmf_get(dtmf_detect_state_t *s, char *buf, int max);

DTMFDetector::DTMFDetector()
:mState(NULL)
{
  mState = malloc(sizeof(dtmf_detect_state_t));

  memset(mState, 0, sizeof(dtmf_detect_state_t));
  zap_dtmf_detect_init((dtmf_detect_state_t*)mState);
}

DTMFDetector::~DTMFDetector()
{
  if (mState)
    free(mState);
}

std::string DTMFDetector::streamPut(unsigned char* samples, unsigned int size)
{
  char buf[16]; buf[0] = 0;
  if (zap_dtmf_detect((dtmf_detect_state_t*)mState, (int16_t*)samples, size/2, 0))
    zap_dtmf_get((dtmf_detect_state_t*)mState, buf, 15);
  return buf;
}

void DTMFDetector::resetState()
{
  zap_dtmf_detect_init((dtmf_detect_state_t*)mState);
}

#ifndef TRUE

# define FALSE   0
# define TRUE    (!FALSE)

#endif

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

//#define USE_3DNOW

/* Basic DTMF specs:
 *
 * Minimum tone on = 40ms
 * Minimum tone off = 50ms
 * Maximum digit rate = 10 per second
 * Normal twist <= 8dB accepted
 * Reverse twist <= 4dB accepted
 * S/N >= 15dB will detect OK
 * Attenuation <= 26dB will detect OK
 * Frequency tolerance +- 1.5% will detect, +-3.5% will reject
 */

#define SAMPLE_RATE                 8000.0

#define DTMF_THRESHOLD              8.0e7
#define FAX_THRESHOLD              8.0e7
#define FAX_2ND_HARMONIC       2.0     /* 4dB */
#define DTMF_NORMAL_TWIST           6.3     /* 8dB */
#define DTMF_REVERSE_TWIST          ((isradio) ? 4.0 : 2.5)     /* 4dB normal */
#define DTMF_RELATIVE_PEAK_ROW      6.3     /* 8dB */
#define DTMF_RELATIVE_PEAK_COL      6.3     /* 8dB */
#define DTMF_2ND_HARMONIC_ROW       ((isradio) ? 1.7 : 2.5)     /* 4dB normal */
#define DTMF_2ND_HARMONIC_COL       63.1    /* 18dB */

static tone_detection_descriptor_t dtmf_detect_row[4];
static tone_detection_descriptor_t dtmf_detect_col[4];
static tone_detection_descriptor_t dtmf_detect_row_2nd[4];
static tone_detection_descriptor_t dtmf_detect_col_2nd[4];
static tone_detection_descriptor_t fax_detect;
static tone_detection_descriptor_t fax_detect_2nd;

static float dtmf_row[] =
{
     697.0,  770.0,  852.0,  941.0
};
static float dtmf_col[] =
{
    1209.0, 1336.0, 1477.0, 1633.0
};

static float fax_freq = 1100.0;

static char dtmf_positions[] = "123A" "456B" "789C" "*0#D";

static void goertzel_init(goertzel_state_t *s,
                   tone_detection_descriptor_t *t)
{
    s->v2 =
    s->v3 = 0.0;
    s->fac = t->fac;
}
/*- End of function --------------------------------------------------------*/

#if defined(USE_3DNOW)
static inline void _dtmf_goertzel_update(goertzel_state_t *s,
                                         float x[],
                                         int samples)
{
    int n;
    float v;
    int i;
    float vv[16];

    vv[4] = s[0].v2;
    vv[5] = s[1].v2;
    vv[6] = s[2].v2;
    vv[7] = s[3].v2;
    vv[8] = s[0].v3;
    vv[9] = s[1].v3;
    vv[10] = s[2].v3;
    vv[11] = s[3].v3;
    vv[12] = s[0].fac;
    vv[13] = s[1].fac;
    vv[14] = s[2].fac;
    vv[15] = s[3].fac;

    //v1 = s->v2;
    //s->v2 = s->v3;
    //s->v3 = s->fac*s->v2 - v1 + x[0];

    __asm__ __volatile__ (
         " femms;\n"

             " movq        16(%%edx),%%mm2;\n"
             " movq        24(%%edx),%%mm3;\n"
             " movq        32(%%edx),%%mm4;\n"
             " movq        40(%%edx),%%mm5;\n"
             " movq        48(%%edx),%%mm6;\n"
             " movq        56(%%edx),%%mm7;\n"

             " jmp         1f;\n"
             " .align 32;\n"

         " 1: ;\n"
             " prefetch    (%%eax);\n"
             " movq        %%mm3,%%mm1;\n"
             " movq        %%mm2,%%mm0;\n"
             " movq        %%mm5,%%mm3;\n"
             " movq        %%mm4,%%mm2;\n"

             " pfmul       %%mm7,%%mm5;\n"
             " pfmul       %%mm6,%%mm4;\n"
             " pfsub       %%mm1,%%mm5;\n"
             " pfsub       %%mm0,%%mm4;\n"

             " movq        (%%eax),%%mm0;\n"
             " movq        %%mm0,%%mm1;\n"
             " punpckldq   %%mm0,%%mm1;\n"
             " add         $4,%%eax;\n"
             " pfadd       %%mm1,%%mm5;\n"
             " pfadd       %%mm1,%%mm4;\n"

             " dec         %%ecx;\n"

             " jnz         1b;\n"

             " movq        %%mm2,16(%%edx);\n"
             " movq        %%mm3,24(%%edx);\n"
             " movq        %%mm4,32(%%edx);\n"
             " movq        %%mm5,40(%%edx);\n"

             " femms;\n"
     :
             : "c" (samples), "a" (x), "d" (vv)
             : "memory", "eax", "ecx");

    s[0].v2 = vv[4];
    s[1].v2 = vv[5];
    s[2].v2 = vv[6];
    s[3].v2 = vv[7];
    s[0].v3 = vv[8];
    s[1].v3 = vv[9];
    s[2].v3 = vv[10];
    s[3].v3 = vv[11];
}
#endif
/*- End of function --------------------------------------------------------*/

void zap_goertzel_update(goertzel_state_t *s,
                     int16_t x[],
                     int samples)
{
    int i;
    float v1;
    
    for (i = 0;  i < samples;  i++)
    {
        v1 = s->v2;
        s->v2 = s->v3;
        s->v3 = s->fac*s->v2 - v1 + x[i];
    }
}
/*- End of function --------------------------------------------------------*/

float zap_goertzel_result (goertzel_state_t *s)
{
    return s->v3*s->v3 + s->v2*s->v2 - s->v2*s->v3*s->fac;
}
/*- End of function --------------------------------------------------------*/

void zap_dtmf_detect_init (dtmf_detect_state_t *s)
{
    int i;
    float theta;

    s->hit1 = 
    s->hit2 = 0;

    for (i = 0;  i < 4;  i++)
    {
        theta = float(2.0*M_PI*(dtmf_row[i]/SAMPLE_RATE));
        dtmf_detect_row[i].fac = float(2.0*cos(theta));

        theta = float(2.0*M_PI*(dtmf_col[i]/SAMPLE_RATE));
        dtmf_detect_col[i].fac = float(2.0*cos(theta));
    
        theta = float(2.0*M_PI*(dtmf_row[i]*2.0/SAMPLE_RATE));
        dtmf_detect_row_2nd[i].fac = float(2.0*cos(theta));

        theta = float(2.0*M_PI*(dtmf_col[i]*2.0/SAMPLE_RATE));
        dtmf_detect_col_2nd[i].fac = float(2.0*cos(theta));
    
   goertzel_init (&s->row_out[i], &dtmf_detect_row[i]);
    goertzel_init (&s->col_out[i], &dtmf_detect_col[i]);
    goertzel_init (&s->row_out2nd[i], &dtmf_detect_row_2nd[i]);
    goertzel_init (&s->col_out2nd[i], &dtmf_detect_col_2nd[i]);

s->energy = 0.0;
    }

    /* Same for the fax dector */
    theta = float(2.0*M_PI*(fax_freq/SAMPLE_RATE));
    fax_detect.fac = float(2.0 * cos(theta));
    goertzel_init (&s->fax_tone, &fax_detect);

    /* Same for the fax dector 2nd harmonic */
    theta = float(2.0*M_PI*(fax_freq * 2.0/SAMPLE_RATE));
    fax_detect_2nd.fac = float(2.0 * cos(theta));
    goertzel_init (&s->fax_tone2nd, &fax_detect_2nd);

    s->current_sample = 0;
    s->detected_digits = 0;
    s->lost_digits = 0;
    s->digits[0] = '\0';
    s->mhit = 0;
}
/*- End of function --------------------------------------------------------*/

int zap_dtmf_detect (dtmf_detect_state_t *s,
                 int16_t amp[],
                 int samples, 
 int isradio)
{

    float row_energy[4];
    float col_energy[4];
    float fax_energy;
    float fax_energy_2nd;
    float famp;
    float v1;
    int i;
    int j;
    int sample;
    int best_row;
    int best_col;
    int hit;
    int limit;

    hit = 0;
    for (sample = 0;  sample < samples;  sample = limit)
    {
        /* 102 is optimised to meet the DTMF specs. */
        if ((samples - sample) >= (102 - s->current_sample))
            limit = sample + (102 - s->current_sample);
        else
            limit = samples;
#if defined(USE_3DNOW)
        _dtmf_goertzel_update (s->row_out, amp + sample, limit - sample);
        _dtmf_goertzel_update (s->col_out, amp + sample, limit - sample);
        _dtmf_goertzel_update (s->row_out2nd, amp + sample, limit2 - sample);
        _dtmf_goertzel_update (s->col_out2nd, amp + sample, limit2 - sample);
/* XXX Need to fax detect for 3dnow too XXX */
#warning "Fax Support Broken"
#else
        /* The following unrolled loop takes only 35% (rough estimate) of the 
           time of a rolled loop on the machine on which it was developed */
        for (j = sample;  j < limit;  j++)
        {
            famp = amp[j];
    
    s->energy += famp*famp;
    
            /* With GCC 2.95, the following unrolled code seems to take about 35%
               (rough estimate) as long as a neat little 0-3 loop */
            v1 = s->row_out[0].v2;
            s->row_out[0].v2 = s->row_out[0].v3;
            s->row_out[0].v3 = s->row_out[0].fac*s->row_out[0].v2 - v1 + famp;
    
            v1 = s->col_out[0].v2;
            s->col_out[0].v2 = s->col_out[0].v3;
            s->col_out[0].v3 = s->col_out[0].fac*s->col_out[0].v2 - v1 + famp;
    
            v1 = s->row_out[1].v2;
            s->row_out[1].v2 = s->row_out[1].v3;
            s->row_out[1].v3 = s->row_out[1].fac*s->row_out[1].v2 - v1 + famp;
    
            v1 = s->col_out[1].v2;
            s->col_out[1].v2 = s->col_out[1].v3;
            s->col_out[1].v3 = s->col_out[1].fac*s->col_out[1].v2 - v1 + famp;
    
            v1 = s->row_out[2].v2;
            s->row_out[2].v2 = s->row_out[2].v3;
            s->row_out[2].v3 = s->row_out[2].fac*s->row_out[2].v2 - v1 + famp;
    
            v1 = s->col_out[2].v2;
            s->col_out[2].v2 = s->col_out[2].v3;
            s->col_out[2].v3 = s->col_out[2].fac*s->col_out[2].v2 - v1 + famp;
    
            v1 = s->row_out[3].v2;
            s->row_out[3].v2 = s->row_out[3].v3;
            s->row_out[3].v3 = s->row_out[3].fac*s->row_out[3].v2 - v1 + famp;

            v1 = s->col_out[3].v2;
            s->col_out[3].v2 = s->col_out[3].v3;
            s->col_out[3].v3 = s->col_out[3].fac*s->col_out[3].v2 - v1 + famp;

            v1 = s->col_out2nd[0].v2;
            s->col_out2nd[0].v2 = s->col_out2nd[0].v3;
            s->col_out2nd[0].v3 = s->col_out2nd[0].fac*s->col_out2nd[0].v2 - v1 + famp;
        
            v1 = s->row_out2nd[0].v2;
            s->row_out2nd[0].v2 = s->row_out2nd[0].v3;
            s->row_out2nd[0].v3 = s->row_out2nd[0].fac*s->row_out2nd[0].v2 - v1 + famp;
        
            v1 = s->col_out2nd[1].v2;
            s->col_out2nd[1].v2 = s->col_out2nd[1].v3;
            s->col_out2nd[1].v3 = s->col_out2nd[1].fac*s->col_out2nd[1].v2 - v1 + famp;
    
            v1 = s->row_out2nd[1].v2;
            s->row_out2nd[1].v2 = s->row_out2nd[1].v3;
            s->row_out2nd[1].v3 = s->row_out2nd[1].fac*s->row_out2nd[1].v2 - v1 + famp;
        
            v1 = s->col_out2nd[2].v2;
            s->col_out2nd[2].v2 = s->col_out2nd[2].v3;
            s->col_out2nd[2].v3 = s->col_out2nd[2].fac*s->col_out2nd[2].v2 - v1 + famp;
        
            v1 = s->row_out2nd[2].v2;
            s->row_out2nd[2].v2 = s->row_out2nd[2].v3;
            s->row_out2nd[2].v3 = s->row_out2nd[2].fac*s->row_out2nd[2].v2 - v1 + famp;
        
            v1 = s->col_out2nd[3].v2;
            s->col_out2nd[3].v2 = s->col_out2nd[3].v3;
            s->col_out2nd[3].v3 = s->col_out2nd[3].fac*s->col_out2nd[3].v2 - v1 + famp;
        
            v1 = s->row_out2nd[3].v2;
            s->row_out2nd[3].v2 = s->row_out2nd[3].v3;
            s->row_out2nd[3].v3 = s->row_out2nd[3].fac*s->row_out2nd[3].v2 - v1 + famp;

/* Update fax tone */
            v1 = s->fax_tone.v2;
            s->fax_tone.v2 = s->fax_tone.v3;
            s->fax_tone.v3 = s->fax_tone.fac*s->fax_tone.v2 - v1 + famp;

            v1 = s->fax_tone.v2;
            s->fax_tone2nd.v2 = s->fax_tone2nd.v3;
            s->fax_tone2nd.v3 = s->fax_tone2nd.fac*s->fax_tone2nd.v2 - v1 + famp;
        }
#endif
        s->current_sample += (limit - sample);
        if (s->current_sample < 102)
            continue;

/* Detect the fax energy, too */
fax_energy = zap_goertzel_result(&s->fax_tone);

        /* We are at the end of a DTMF detection block */
        /* Find the peak row and the peak column */
        row_energy[0] = zap_goertzel_result (&s->row_out[0]);
        col_energy[0] = zap_goertzel_result (&s->col_out[0]);

for (best_row = best_col = 0, i = 1;  i < 4;  i++)
{
        row_energy[i] = zap_goertzel_result (&s->row_out[i]);
            if (row_energy[i] > row_energy[best_row])
                best_row = i;
        col_energy[i] = zap_goertzel_result (&s->col_out[i]);
            if (col_energy[i] > col_energy[best_col])
                best_col = i;
    }
        hit = 0;
        /* Basic signal level test and the twist test */
        if (row_energy[best_row] >= DTMF_THRESHOLD
    &&
    col_energy[best_col] >= DTMF_THRESHOLD
            &&
            col_energy[best_col] < row_energy[best_row]*DTMF_REVERSE_TWIST
            &&
            col_energy[best_col]*DTMF_NORMAL_TWIST > row_energy[best_row])
        {
            /* Relative peak test */
            for (i = 0;  i < 4;  i++)
            {
                if ((i != best_col  &&  col_energy[i]*DTMF_RELATIVE_PEAK_COL > col_energy[best_col])
                    ||
                    (i != best_row  &&  row_energy[i]*DTMF_RELATIVE_PEAK_ROW > row_energy[best_row]))
                {
                    break;
                }
            }
            /* ... and second harmonic test */
            if (i >= 4
        &&
(row_energy[best_row] + col_energy[best_col]) > 42.0*s->energy
                &&
                zap_goertzel_result (&s->col_out2nd[best_col])*DTMF_2ND_HARMONIC_COL < col_energy[best_col]
                &&
                zap_goertzel_result (&s->row_out2nd[best_row])*DTMF_2ND_HARMONIC_ROW < row_energy[best_row])
            {
                hit = dtmf_positions[(best_row << 2) + best_col];
                /* Look for two successive similar results */
                /* The logic in the next test is:
                   We need two successive identical clean detects, with
   something different preceeding it. This can work with
   back to back differing digits. More importantly, it
   can work with nasty phones that give a very wobbly start
   to a digit. */
                if (hit == s->hit3  &&  s->hit3 != s->hit2)
                {
    s->mhit = hit;
                    s->digit_hits[(best_row << 2) + best_col]++;
                    s->detected_digits++;
                    if (s->current_digits < MAX_DTMF_DIGITS)
                    {
                        s->digits[s->current_digits++] = hit;
                        s->digits[s->current_digits] = '\0';
                    }
                    else
                    {
                        s->lost_digits++;
                    }
                }
            }
        } 
if (!hit && (fax_energy >= FAX_THRESHOLD) && (fax_energy > s->energy * 21.0)) {
fax_energy_2nd = zap_goertzel_result(&s->fax_tone2nd);
if (fax_energy_2nd * FAX_2ND_HARMONIC < fax_energy) {
#if 0
printf("Fax energy/Second Harmonic: %f/%f\n", fax_energy, fax_energy_2nd);
#endif
/* XXX Probably need better checking than just this the energy XXX */
hit = 'f';
s->fax_hits++;
} /* Don't reset fax hits counter */
} else {
if (s->fax_hits > 5) {
 s->mhit = 'f';
             s->detected_digits++;
             if (s->current_digits < MAX_DTMF_DIGITS)
             {
                  s->digits[s->current_digits++] = hit;
                  s->digits[s->current_digits] = '\0';
             }
             else
             {
                   s->lost_digits++;
             }
}
s->fax_hits = 0;
}
        s->hit1 = s->hit2;
        s->hit2 = s->hit3;
        s->hit3 = hit;
        /* Reinitialise the detector for the next block */
        for (i = 0;  i < 4;  i++)
        {
           goertzel_init (&s->row_out[i], &dtmf_detect_row[i]);
            goertzel_init (&s->col_out[i], &dtmf_detect_col[i]);
        goertzel_init (&s->row_out2nd[i], &dtmf_detect_row_2nd[i]);
        goertzel_init (&s->col_out2nd[i], &dtmf_detect_col_2nd[i]);
        }
    goertzel_init (&s->fax_tone, &fax_detect);
    goertzel_init (&s->fax_tone2nd, &fax_detect_2nd);
s->energy = 0.0;
        s->current_sample = 0;
    }
    if ((!s->mhit) || (s->mhit != hit))
    {
s->mhit = 0;
return(0);
    }
    return (hit);
}
/*- End of function --------------------------------------------------------*/

int zap_dtmf_get (dtmf_detect_state_t *s,
              char *buf,
              int max)
{
    if (max > s->current_digits)
        max = s->current_digits;
    if (max > 0)
    {
        memcpy (buf, s->digits, max);
        memmove (s->digits, s->digits + max, s->current_digits - max);
        s->current_digits -= max;
    }
    buf[max] = '\0';
    return  max;
}


