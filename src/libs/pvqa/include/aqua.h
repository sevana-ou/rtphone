#ifndef __A_Qu_A_dll_h__
#define __A_Qu_A_dll_h__

#ifdef _WINDOWS
#   if defined(AQUA_STATIC_LIBRARY)
#       define SSA_SDK_API extern "C"
#   else
#       ifdef SSA_HEADERS_EXPORTS
#           define SSA_SDK_API extern "C" __declspec(dllexport)
#       else
#           define SSA_SDK_API extern "C" __declspec(dllimport)
#       endif
#   endif
#else
#   define SSA_SDK_API
#endif




// The structure containing information about AQuA library
struct TSSA_AQuA_Info
{
  int          dStructSize;              // Structure size
  char       * dCopyrightString;         // Copyright string
  char       * dVersionString;           // Product name and version number string
  int          dSampleRateLimit;         // Maximal sampling frequency supported by the library
  int          dChannelsLimit;           // Maximal amount of channels supported by the library
  bool         isDifferentFFmtCheckingEnabled; 
                                         // If comparison of audio files in different formats is allowed
  const char * pSupportedBitsPerSampleList;
                                         // List of supported sample bits
  const char * pSupportedCodecsList;     // List of supported audio compression algorithms
};

// The structure contains received quality measurement estimations
struct TSSA_AQuA_Results
{
  double dPercent;   // Percentage
  double dMOSLike;   // MOS-like
  double dPESQLike;  // PESQ-like
};

// The structure containing audio data from a single source
struct TSSA_AQuA_AudioItem
{
  long    dSampleRate;  // Sampling frequency
  long    dNChannels;   // The number of channels in the sound (1 mono, 2 stereo, etc.)
  short * pSamples;     // The signal samples
  long    dNSamples;    // Number of signal samples
};

// The structure contains the audio data for comparison
struct TSSA_AQuA_AudioData
{
  TSSA_AQuA_AudioItem dSrcData; // Data reference signal
  TSSA_AQuA_AudioItem dTstData; // These degraded signal
};


// Library initialization, check library copyrights and software license period and validity. 
// If initialization is successful the function returns 0, otherwise returns error code.
SSA_SDK_API int SSA_InitLib(char * aPLicFName);
SSA_SDK_API int SSA_InitLibWithData(const void* buffer, size_t len);

// This function is called when work with the library is finished. 
// Invoking this function is important for further work with the library.
SSA_SDK_API void SSA_ReleaseLib(void);

// Returns pointer to data structure containing information about AQuA library
SSA_SDK_API TSSA_AQuA_Info * SSA_GetPAQuAInfo(void);



// Creates audio analyzer. If successful returns analyzer's handle, 
// if unsuccessful the function returns NULL.
SSA_SDK_API void * SSA_CreateAudioQualityAnalyzer(void);

// Finishes working with analyzer and deletes all variables used during analyzer work.
SSA_SDK_API void SSA_ReleaseAudioQualityAnalyzer(void * anSSA_ID);

// Performs audio files quality estimation according to the files passed 
// to the analyzer in the setting functions.
SSA_SDK_API int SSA_OnTestAudioFiles(void * anSSA_ID);

// Performs audio quality estimation according to the data passed 
// to the analyzer in aSSA_PAudio structure.
SSA_SDK_API int SSA_OnTestAudioData(void * anSSA_ID, TSSA_AQuA_AudioData * aSSA_PAudio);

// Performs audio quality estimation according to the data passed 
// to the analyzer in aSSA_PAudio structure (with accumulation). 
SSA_SDK_API int SSA_OnTestAddAudioData(void * anSSA_ID, TSSA_AQuA_AudioData * aSSA_PAudio);



// --------------------------------------------------------------------------------------
// Checking file formats
// --------------------------------------------------------------------------------------

// Checks if file formats are supported by the library. If the format is supported 
// the function returns "true", otherwise the return value is "false".
SSA_SDK_API bool SSA_IsFileFormatSupportable(void * anSSA_ID, char * aPFName);

// Checks if file comparison is possible. If files format is supported and 
// file comparison is supported for these file formats then the return value 
// is "true", otherwise the function returns "false".
SSA_SDK_API bool SSA_AreFilesComparable(void * anSSA_ID, char * aPSrcFName, char * aPTstFName);



// --------------------------------------------------------------------------------------
// Displaying audio quality testing results
// --------------------------------------------------------------------------------------

// Returns string length containing test results in text.
SSA_SDK_API int SSA_GetQualityStringSize(void * anSSA_ID);

// Fills string with the text of the test result. User should allocate memory 
// for the string by himself. Amount of the memory required can be found 
// by function SSA_GetQualityStringSize.
SSA_SDK_API int SSA_FillQualityString(void * anSSA_ID, char * aPString);

// Fills the structure with results of audio quality measurements.
SSA_SDK_API int SSA_FillQualityResultsStruct(void * anSSA_ID, TSSA_AQuA_Results * aPQResults);

// Returns size of array for integral energy spectrum of the original signal. 
// Note that signal spectrum is available only after quality estimation has been performed 
// and only in the mode "QualityMode" = 0. If signal spectrum was not calculated 
// the function returns 0, in case of error the function returns -1.
SSA_SDK_API int SSA_GetSrcSignalSpecSize(void * anSSA_ID);

// Returns size of array for integral energy spectrum of the signal under test. 
// Note that signal spectrum is available only after quality estimation has been performed 
// and only in the mode "QualityMode" = 0. If signal spectrum was not calculated 
// the function returns 0, in case of error the function returns -1.
SSA_SDK_API int SSA_GetTstSignalSpecSize(void * anSSA_ID);

// Fills array with integral energy spectrum of the original signal. 
// Note that signal spectrum is available only after quality estimation has been performed 
// and only in the mode "QualityMode" = 0. If signal spectrum was not calculated 
// the function returns 0, in case of error the function returns -1.
SSA_SDK_API int SSA_FillSrcSignalSpecArray(void * anSSA_ID, float * aPSpecArray);

// Fills array with integral energy spectrum of the signal under test. 
// Note that signal spectrum is available only after quality estimation has been performed 
// and only in the mode "QualityMode" = 0. If signal spectrum was not calculated 
// the function returns 0, in case of error the function returns -1.
SSA_SDK_API int SSA_FillTstSignalSpecArray(void * anSSA_ID, float * aPSpecArray);

// Returns size of the string containing reasons for quality loss. 
// String size does not consider 0 symbol in the end of the string.
SSA_SDK_API int SSA_GetFaultsAnalysisStringSize(void * anSSA_ID);

// Fills string with reasons for audio quality loss. String aPString contains only 
// meaningful symbols and does not contain 0 symbol in the end.
SSA_SDK_API int SSA_FillFaultsAnalysisString(void * anSSA_ID, char * aPString);

// Returns size of the string containing signals spectrums pairs.
SSA_SDK_API int SSA_GetSpecPairsStringSize(void * anSSA_ID);

// Fills string with signals spectrums pairs.
SSA_SDK_API int SSA_FillSpecPairsString(void * anSSA_ID, char * aPString, int aSSize);

SSA_SDK_API int SSA_GetSpecPairsSize(void * anSSA_ID);
SSA_SDK_API int SSA_GetSpecPairsArray(void * anSSA_ID, float * aPArray);

// Sets any parameter of the analyzer
SSA_SDK_API bool SSA_SetAnyString(void * anSSA_ID, char * aPParName, char * aPParValue);

//
SSA_SDK_API int SSA_OnHSMGenerate(void * anSSA_ID, bool aFmtID);

#endif // __A_Qu_A_dll_h__

