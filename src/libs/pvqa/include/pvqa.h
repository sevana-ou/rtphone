#ifndef __P_V_Q_A_dll_h__
#define __P_V_Q_A_dll_h__

#ifdef _WINDOWS
#if defined(PVQA_STATIC_LIBRARY)
#   define PVQA_API extern "C"
#else
#   ifdef PVQADLL_EXPORTS
	#define PVQA_API extern "C" __declspec(dllexport)
#   else
	#define PVQA_API extern "C" __declspec(dllimport)
#   endif
#endif
#else
#   if defined(__cplusplus)
#       define PVQA_API extern "C"
#   else
#       define PVQA_API extern
#   endif
#endif

// It is for size_t type
#include <stddef.h>

// The structure contains received quality measurement estimations
typedef struct
{
  double dMOSLike;   // MOS-like

  long   dNumPoorIntervals;
  long   dNumTotalIntervals;

  double dTimeShift;
} TPVQA_Results;

// 
typedef struct
{
  long    dSampleRate;  // Sample rate of signal
  long    dNChannels;   // Number of sound channels in audio
  short * pSamples;     // Pointer to the samples of signal
  long    dNSamples;    // Number of samples
}  TPVQA_AudioItem;

#if defined(__ANDROID__)
// Init Android related part. System info and JNI environment context sometimes has to be initialized in separate from other calls.
PVQA_API void PVQA_SetupAndroidEnvironment(void* /* JNIEnv* inside */environment, void* /* jobject */ appcontext);
#endif

// Library initialization, check library copyrights and software license period and validity.
// If initialization is successful the function returns 0, otherwise returns error code.
PVQA_API int PVQA_InitLib(const char * aPLicFName);
PVQA_API int PVQA_InitLibWithLicData(const void* buffer, size_t length);

// Returns a string with the name
PVQA_API const char * PVQA_GetName(void);

// Returns a string with the version number
PVQA_API const char * PVQA_GetVersion(void);

// Returns a string with the copyright
PVQA_API const char * PVQA_GetCopyrightString(void);

// This function is called when work with the library is finished. 
// Invoking this function is important for further work with the library.
PVQA_API void PVQA_ReleaseLib(void);

// Creates audio analyzer. If successful returns analyzer's handle, 
// if unsuccessful the function returns NULL.
PVQA_API void * PVQA_CreateAudioQualityAnalyzer(void * aCFG_ID);

// Finishes working with analyzer and deletes all variables used during analyzer work.
PVQA_API int PVQA_ReleaseAudioQualityAnalyzer(void * aPVQA_ID);

// Set length of samples delay line  
PVQA_API int PVQA_AudioQualityAnalyzerSetIntervalLength(void * aPVQA_ID, double length);

// Create audio-processing delay-line
PVQA_API int PVQA_AudioQualityAnalyzerCreateDelayLine(void * aPVQA_ID, long aSampleRate, long aNChannels, double aLineTime);

// Start recording a debug sound file
PVQA_API int PVQA_StartRecordingWave(void * aPVQA_ID, const char * aPWavFName, long aSampleRate, long aNChannels);

// Performs audio file quality estimation
PVQA_API int PVQA_OnTestAudioFile(void * aPVQA_ID, const char * aPFileName);

// Process loaded in memory audio data 
PVQA_API int PVQA_OnTestAudioData(void * aPVQA_ID, TPVQA_AudioItem * aPVQA_PAudio);

// Put Audio data to delay-line and process it
PVQA_API int PVQA_OnTestAddAudioData(void * aPVQA_ID, TPVQA_AudioItem * aPVQA_PAudio);

// Start audio-stream processing
PVQA_API int PVQA_OnStartStreamData(void * aPVQA_ID);

// Process streaming Audio data
PVQA_API int PVQA_OnAddStreamAudioData(void * aPVQA_ID, TPVQA_AudioItem * aPVQA_PAudio);
PVQA_API int PVQA_OnAddStreamAudioDataHex(void * aPVQA_ID, int rate, int channels, const char* samples);

// Updates quality results data
PVQA_API int PVQA_OnUpdateStreamQualityResults(void * aPVQA_ID, int rate, int millisecondsFromEnd);

// Finalize quality measurements at this moment (not finalize stream processing)
PVQA_API int PVQA_OnFinalizeStream(void * aPVQA_ID, long aSampleRate);



// --------------------------------------------------------------------------------------
// Displaying audio quality testing results
// --------------------------------------------------------------------------------------

// Returns string length containing test results in text.
PVQA_API int PVQA_GetQualityStringSize(void * anPVQA_ID);

// Fills string with the text of the test result. User should allocate memory 
// for the string by himself. Amount of the memory required can be found 
// by function PVQA_GetQualityStringSize.
PVQA_API int PVQA_FillQualityString(void * aPVQA_ID, char * aPString);

// Returns string by combining PVQA_GetQualityStringSize()  + PVQA_FillQualityString() + PVQA_FillQualityResultsStruct()
PVQA_API char* PVQA_GetQualityString(void * ctx, double* mos, int* errCode);

// Fills the structure with results of audio quality measurements.
PVQA_API int PVQA_FillQualityResultsStruct(void * aPVQA_ID, TPVQA_Results * aPQResults);

// Get a list of the names of the values of the detector
// No need to free() the pointer.
PVQA_API const char ** PVQA_GetProcessorValuesNamesList(void * aPVQA_ID, const char * aPDetectorName, int * errCode);

typedef struct 
{
    int columns, rows;
    float* data;
} PVQA_Array2D;

// Get a list of the values of the detector
// Returned value has to be released via PVQA_ReleaseArray2D
PVQA_API PVQA_Array2D* PVQA_GetProcessorValuesList(void * aPVQA_ID, const char * aPDetectorName, long aStartTime,
                                                                             long aStopTime, const char * aStatType, int * errCode);
// Release allocated 2D array
PVQA_API void PVQA_ReleaseArray2D(PVQA_Array2D* array);

// --------------------------------------------------------------------------------------
// Working with configuration files
// --------------------------------------------------------------------------------------

// Load settings File in memory
PVQA_API void * PVQA_LoadCFGFile(const char * aFileName, int * errCode);
PVQA_API void * PVQA_LoadCFGData(const void* buffer, size_t length, int* errCode);

// Release settings file structures
PVQA_API int PVQA_ReleaseCFGFile(void * aCFG_ID);

#endif // __P_V_Q_A_dll_h__

