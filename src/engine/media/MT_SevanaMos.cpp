#define NOMINMAX

//#include "config.h"
#include "MT_SevanaMos.h"

#if defined(USE_PVQA_LIBRARY)

#if defined(PVQA_SERVER)
# include <boost/filesystem.hpp>
# include <boost/algorithm/string.hpp>
using namespace boost::filesystem;
#endif

#include "../engine/helper/HL_Log.h"
#include "../engine/helper/HL_CsvReader.h"
#include "../engine/helper/HL_String.h"
#include "../engine/audio/Audio_WavFile.h"

#include <assert.h>
#include <fstream>
#include <streambuf>
#include <iostream>
#include <atomic>
#include <algorithm>

#if defined(PVQA_SERVER)
extern std::string IntervalCacheDir;
#endif

#define LOG_SUBSYSTEM "Sevana"

#ifdef WIN32
# define popen _popen
# define pclose _pclose
#endif

#define PVQA_ECHO_DETECTOR_NAME "ECHO"
//#define PVQA_ECHO_DETECTOR_NAME "EchoM-00"

namespace MT {

#if !defined(MOS_BEST_COLOR)
# define MOS_BEST_COLOR 0x11FF11
# define MOS_BAD_COLOR 0x000000
#endif

#if defined(TARGET_WIN)
#   define popen _popen
#   define pclose _pclose
#endif

static std::string execCommand(const std::string& cmd)
{
    std::cout << cmd << "\n";

    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe)
        throw std::runtime_error("Failed to run.");

    char buffer[1024];
    std::string result = "";
    while (!feof(pipe.get()))
    {
        if (fgets(buffer, 1024, pipe.get()) != nullptr)
            result += buffer;
    }
    return result;
}

// ------------ PvqaUtility ------------
PvqaUtility::PvqaUtility()
{

}

PvqaUtility::~PvqaUtility()
{

}

void PvqaUtility::setPath(const std::string& path)
{
    mPvqaPath = path;
}

std::string PvqaUtility::getPath() const
{
    return mPvqaPath;
}

void PvqaUtility::setLicensePath(const std::string& path)
{
    mLicensePath = path;
}

std::string PvqaUtility::getLicensePath() const
{
    return mLicensePath;
}

void PvqaUtility::setConfigPath(const std::string& path)
{
    mConfigPath = path;
}

std::string PvqaUtility::getConfigPath() const
{
    return mConfigPath;
}

float PvqaUtility::process(const std::string& filepath, std::string& outputReport)
{
    float result = 0.0f;

    // Generate temporary filename to receive .csv file
    char report_filename[L_tmpnam];
    tmpnam(report_filename);

    // Build command line
    char cmdbuffer[1024];
    sprintf(cmdbuffer, "\"%s\" \"%s\" analysis \"%s\" \"%s\" \"%s\" 0.799", mPvqaPath.c_str(), mLicensePath.c_str(),
            report_filename, mConfigPath.c_str(), filepath.c_str());
    try
    {
        std::string output = execCommand(cmdbuffer);

        std::string line;
        std::istringstream is(output);
        std::string estimation;
        while (std::getline(is, line))
        {
            std::string::size_type mosPosition = line.find("MOS = ");
            if ( mosPosition != std::string::npos)
            {
                estimation = line.substr(mosPosition + 6);
                estimation = StringHelper::trim(estimation);
            }
        }

        if (!estimation.size())
            throw std::runtime_error("Bad response from pvqa: " + output);

        result = std::stof(estimation);

        // Read intervals report file
        std::ifstream t(report_filename);
        std::string str((std::istreambuf_iterator<char>(t)),
                         std::istreambuf_iterator<char>());
        outputReport = str;
        ::remove(report_filename);
        ::remove(report_filename);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return result;
}

// ---------------- AquaUtility -----------------
AquaUtility::AquaUtility()
{}

AquaUtility::~AquaUtility()
{}

void AquaUtility::setPath(const std::string& path)
{
    mAquaPath = path;
}

std::string AquaUtility::getPath() const
{
    return mAquaPath;
}

void AquaUtility::setLicensePath(const std::string& path)
{
    mLicensePath = path;
}

std::string AquaUtility::getLicensePath() const
{
    return mLicensePath;
}

void AquaUtility::setConfigLine(const std::string& line)
{
    mConfigLine = line;
}

std::string AquaUtility::getConfigLine() const
{
    return mConfigLine;
}

float AquaUtility::compare(const std::string& src_path, const std::string& dst_path, std::string& output_report)
{
    float result = 0.0f;

    // Generate temporary filename to receive .csv file
    char spectrum_filename[L_tmpnam], report_filename[L_tmpnam];
    tmpnam(spectrum_filename);
    tmpnam(report_filename);

    std::string config_line = mConfigLine;
    StringHelper::replace(config_line, ":src_file", "\"" + src_path + "\"");
    StringHelper::replace(config_line, ":tst_file", "\"" + dst_path + "\"");
    StringHelper::replace(config_line, ":spectrum_file", spectrum_filename);
    StringHelper::replace(config_line, ":report_file", report_filename);
    StringHelper::replace(config_line, ":license_file", "\"" + mLicensePath + "\"");

    // Build command line
    char cmdbuffer[2048];
    sprintf(cmdbuffer, "\"%s\" %s", mAquaPath.c_str(), config_line.c_str());

    try
    {
        std::string output = execCommand(cmdbuffer);

        std::string line;
        std::istringstream is(output);
        std::string estimation;
        while (std::getline(is, line))
        {
            std::string::size_type mosPosition = line.find("MOS value ");
            if ( mosPosition != std::string::npos)
            {
                estimation = line.substr(mosPosition + 6);
                estimation = StringHelper::trim(estimation);
            }
        }

        if (!estimation.size())
            throw std::runtime_error("Bad response from pvqa: " + output);

        result = std::stof(estimation);

        // Read intervals report file
        std::ifstream t(report_filename);
        std::string str((std::istreambuf_iterator<char>(t)),
                         std::istreambuf_iterator<char>());

        output_report = str;
        ::remove(report_filename);
        ::remove(report_filename);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << "\n";
    }

    return result;
}

// -------------- SevanaMosUtility --------------
void SevanaMosUtility::run(const std::string& pcmPath, const std::string& intervalPath,
                           std::string& estimation, std::string& intervals)
{
#if defined(PVQA_SERVER)
    path sevana = current_path() / "sevana";
#if defined(TARGET_LINUX) || defined(TARGET_OSX)
    path exec = sevana / "pvqa";
#else
    path exec = sevana / "pvqa.exe";
#endif
    path lic = sevana / "pvqa.lic";
    path cfg = sevana / "settings.cfg";

    estimation.clear();
    char cmdbuffer[1024];
    sprintf(cmdbuffer, "%s %s analysis %s %s %s 0.799", exec.string().c_str(), lic.string().c_str(),
            intervalPath.c_str(), cfg.string().c_str(), pcmPath.c_str());
    std::string output = execCommand(cmdbuffer);

    //ICELogDebug(<< "Got PVQA analyzer output: " << output);

    std::string line;
    std::istringstream is(output);
    while (std::getline(is, line))
    {
        std::string::size_type mosPosition = line.find("MOS = ");
        if ( mosPosition != std::string::npos)
        {
            estimation = line.substr(mosPosition + 6);
            boost::algorithm::trim(estimation);
        }
    }

    if (!estimation.size())
    {
        // Dump utility output if estimation failed
        ICELogError(<< "PVQA failed with message: " << output);
        return;
    }

    // Read intervals report file
    if (boost::filesystem::exists(intervalPath) && !estimation.empty())
    {
        std::ifstream t(intervalPath);
        std::string str((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());
        intervals = str;
    }
#endif
}

float getSevanaMos(const std::string& audioPath, const std::string& intervalReportPath,
                   std::string& intervalReport)
{
    // Find Sevana MOS estimation
    ICELogDebug( << "Running MOS utitlity on resulted PCM file " << audioPath );
    try
    {
        std::string buffer;
        SevanaMosUtility::run(audioPath, intervalReportPath, buffer, intervalReport);
        ICELogDebug( << "MOS utility is finished on PCM file " << audioPath );
        return (float)atof(buffer.c_str());
    }
    catch(std::exception& e)
    {
        ICELogError( << "MOS utility failed on PCM file " << audioPath << ". Error msg: " << e.what() );
        return 0.0;
    }
}

// ------------------- SevanaPVQA -------------------
void* SevanaPVQA::mLibraryConfiguration = nullptr;
int SevanaPVQA::mLibraryErrorCode = 0;
std::atomic_int SevanaPVQA::mInstanceCounter;
std::atomic_uint_least64_t SevanaPVQA::mAllProcessedMilliseconds;
bool SevanaPVQA::mPvqaLoaded = false;

std::string SevanaPVQA::getVersion()
{
    return PVQA_GetVersion();
}

#if defined(TARGET_ANDROID)
void SevanaPVQA::setupAndroidEnvironment(void *environment, void *appcontext)
{
    PVQA_SetupAndroidEnvironment(environment, appcontext);
}
#endif

bool SevanaPVQA::initializeLibrary(const std::string& pathToLicenseFile, const std::string& pathToConfigFile)
{
    mPvqaLoaded = false;

    ICELogInfo(<< "Sevana PVQA is about to be initialized.");

    // Initialize PVQA library
    if (!mLibraryConfiguration)
    {
        mInstanceCounter = 0;
        mLibraryErrorCode = PVQA_InitLib(const_cast<char*>(pathToLicenseFile.c_str()));
        if (mLibraryErrorCode)
        {
            ICELogError(<< "Problem when initializing PVQA library. Error code: " << mLibraryErrorCode
                           << ". Path to license file is " << pathToLicenseFile
                           << ". Path to config file is " << pathToConfigFile);
            return false;
        }

        mLibraryConfiguration = PVQA_LoadCFGFile(const_cast<char*>(pathToConfigFile.c_str()), &mLibraryErrorCode);
        if (!mLibraryConfiguration)
        {
            PVQA_ReleaseLib();
            ICELogError(<< "Problem with PVQA configuration file.");
            return false;
        }
        mPvqaLoaded = true;
    }
    return true;
}

bool SevanaPVQA::isInitialized()
{
    return mPvqaLoaded;
}

int SevanaPVQA::getLibraryError()
{
    return mLibraryErrorCode;
}

void SevanaPVQA::releaseLibrary()
{
    PVQA_ReleaseLib();
}

SevanaPVQA::SevanaPVQA()
{
}

SevanaPVQA::~SevanaPVQA()
{
    close();
}

void SevanaPVQA::open(double interval, Model model)
{
    if (!isInitialized())
    {
        ICELogError(<< "PVQA library is not initialized.");
        return;
    }

    if (mOpenFailed)
    {
        ICELogError(<< "Open failed already, reject this attempt.");
        return;
    }

    if (mContext)
    {
        ICELogError(<< "Already opened (context is not nullptr).");
        return;
    }

    ICELogDebug(<<"Attempt to create PVQA instance.");
    mProcessedSamples = 0;
    mModel = model;
    mIntervalLength = interval;
    mAudioLineInitialized = false;

    mContext = PVQA_CreateAudioQualityAnalyzer(mLibraryConfiguration);
    if (!mContext)
    {
        ICELogError(<< "Failed to create PVQA instance. Instance counter: " << mInstanceCounter);
        mOpenFailed = true;
        return;
    }

    mInstanceCounter++;

    int rescode = 0;
    rescode = PVQA_AudioQualityAnalyzerSetIntervalLength(mContext, interval);
    if (rescode)
    {
        ICELogError(<< "Failed to set interval length on PVQA instance. Result code: " << rescode);
        close();
        mOpenFailed = true;
        return;
    }

    if (mModel == Model::Stream)
    {
        rescode = PVQA_OnStartStreamData(mContext);
        if (rescode)
        {
            ICELogError(<< "Failed to start streaming analysis on PVQA instance. Result code: " << rescode);
            close();
            mOpenFailed = true;
            return;
        }
    }
    ICELogDebug(<<"PVQA instance is created. Instance counter: " << mInstanceCounter);
}

void SevanaPVQA::close()
{
    if (mContext)
    {
        ICELogDebug(<< "Attempt to destroy PVQA instance.");
        PVQA_ReleaseAudioQualityAnalyzer(mContext);
        mInstanceCounter--;
        ICELogDebug(<< "PVQA instance destroyed. Current instance counter: " << mInstanceCounter);
        mContext = nullptr;
        mOpenFailed = false;
    }
}

bool SevanaPVQA::isOpen() const
{
    return mContext != nullptr;
}

void SevanaPVQA::update(int samplerate, int channels, const void *pcmBuffer, int pcmLength)
{
    if (!mContext)
    {
        ICELogError(<< "No PVQA context.");
        return;
    }
    // Model is assert here as it can be any if context is not created.
    assert (mModel == Model::Stream);

    TPVQA_AudioItem item;
    item.dNChannels = channels;
    item.dSampleRate = samplerate;
    item.dNSamples = pcmLength / 2 / channels;
    item.pSamples = (short*)pcmBuffer;
    int rescode = PVQA_OnAddStreamAudioData(mContext, &item);
    if (rescode)
    {
        ICELogError(<< "Failed to stream data to PVQA instance. Result code: " << rescode);
    }
    int milliseconds = pcmLength / 2 / channels / (samplerate / 1000);
    mProcessedMilliseconds += milliseconds;
    mAllProcessedMilliseconds += milliseconds;
}

SevanaPVQA::DetectorsList SevanaPVQA::getDetectorsNames(const std::string& report)
{
    DetectorsList result;

    if (!report.empty())
    {
        std::istringstream iss(report);
        CsvReader reader(iss);
        reader.readLine(result.mNames);
        result.mStartIndex = 2;

        // Remove first columns
        if (result.mStartIndex < (int)result.mNames.size() - 1)
        {
            result.mNames.erase(result.mNames.begin(), result.mNames.begin() + result.mStartIndex);

            // Remove last column
            result.mNames.erase(result.mNames.begin() + result.mNames.size() - 1);

            for (auto& name: result.mNames)
                name = StringHelper::trim(name);
        }
    }
    return result;
}

float SevanaPVQA::getResults(std::string& report, EchoData** echo, int samplerate, Codec codec)
{
    if (!mContext)
    {
        ICELogError(<< "No PVQA context.");
        return 0.0;
    }

    if (mModel == Model::Stream)
    {
        if (mProcessedMilliseconds == 0)
        {
            ICELogError(<< "No audio in PVQA.");
            return -1;
        }

        if (PVQA_OnFinalizeStream(mContext, (long)samplerate))
        {
            ICELogError(<< "Failed to finalize results from PVQA.");
            return -1;
        }
        ICELogInfo(<< "Processed " << mProcessedMilliseconds << " milliseconds.");
    }

    TPVQA_Results results;
    if (PVQA_FillQualityResultsStruct(mContext, &results))
    {
        ICELogError(<< "Failed to get results from PVQA.");
        return -1;
    }

    int reportLength = PVQA_GetQualityStringSize(mContext);
    if (reportLength)
    {
        char* buffer = (char*)alloca(reportLength + 1);
        if (PVQA_FillQualityString(mContext, buffer))
        {
            ICELogError(<< "Failed to fill intervals report.");
        }
        else
            report = buffer;
    }

#if defined(TARGET_LINUX) && defined(PVQA_WITH_ECHO_DATA)
    if (mModel == SevanaPVQA::Model::Stream && echo)
    {
        // Return echo detector counters
        // Get list of names for echo detector - for debugging only
        std::vector<std::string> names;
        int errCode = 0;
        const char** iNames = (const char **)PVQA_GetProcessorValuesNamesList(mContext, PVQA_ECHO_DETECTOR_NAME, &errCode);
        if (!errCode && iNames)
        {
            int nameIndex = 0;
            for(const char * locName = iNames[nameIndex]; locName; locName = iNames[++nameIndex])
                names.push_back(locName);

            // Get values for echo detector
            PVQA_Array2D* array = PVQA_GetProcessorValuesList(mContext, PVQA_ECHO_DETECTOR_NAME, 0, mProcessedMilliseconds, "values", &errCode);
            if (array)
            {
                *echo = new std::vector<std::vector<float>>();
                for (int r = 0; r < array->rows; r++)
                {
                    std::vector<float> row;
                    for (int c = 0; c < array->columns; c++)
                        row.push_back(array->data[r * array->columns + c]);
                    (*echo)->push_back(row);
                }
                PVQA_ReleaseArray2D(array); array = nullptr;
            }
            // For debugging only
            /*if (*echo)
      {
        for (const auto& row: **echo)
        {
          std::cout << "<";
          for (const auto& v: row)
            std::cout << v << " ";
          std::cout << ">" << std::endl;
        }
      }*/
            // No need to delete maxValues - it will be deleted on PVQA analyzer context freeing.
        }
    }
#endif

    // Limit maximal value of MOS depending on codec
    float result = (float)results.dMOSLike;
    float mv = 5.0;
    switch (codec)
    {
    case Codec::G711:       mv = 4.1f; break;
    case Codec::G729:       mv = 3.92f; break;
    default:
        mv = 5.0;
    }

    return std::min(result, mv);
}

void SevanaPVQA::setPathToDumpFile(const std::string& path)
{
    mDumpWavPath = path;
}

float SevanaPVQA::process(int samplerate, int channels, const void *pcmBuffer, int pcmLength, std::string &report, Codec codec)
{
    //std::cout << "Sent " << pcmLength << " bytes of audio to analyzer." << std::endl;
    assert (mModel == Model::Interval);
    if (!mContext)
        return 0.0;

    /*if (!mAudioLineInitialized)
  {
    mAudioLineInitialized = true;
    if (PVQA_AudioQualityAnalyzerCreateDelayLine(mContext, samplerate, channels, 20))
      ICELogError(<< "Failed to create delay line.");
  }*/

    TPVQA_AudioItem item;
    item.dNChannels = channels;
    item.dSampleRate = samplerate;
    item.dNSamples = pcmLength / 2 / channels;
    item.pSamples = (short*)pcmBuffer;

    //std::cout << "Sending chunk of audio with rate = " << samplerate << ", channels = " << channels << ", number of samples " << item.dNSamples << std::endl;

    /*
  if (!mDumpWavPath.empty())
  {
    WavFileWriter writer;
    writer.open(mDumpWavPath, samplerate, channels);
    writer.write(item.pSamples, item.dNSamples * 2 * channels);
    writer.close();
    ICELogError(<< "Sending chunk of audio with rate = " << samplerate << ", channels = " << channels << ", number of samples " << item.dNSamples);
  }
  */
    int code = PVQA_OnTestAudioData(mContext, &item);
    if (code)
    {
        ICELogError(<< "Failed to run PVQA on audio buffer with code " << code);
        return 0.0;
    }

    /*
  if (item.pSamples != pcmBuffer || item.dNSamples != pcmLength / 2 / channels || item.dSampleRate != samplerate || item.dNChannels != channels)
  {
    ICELogError(<< "PVQA changed input parameters!!!!");
  }
  */
    // Increase counter of processed samples
    mProcessedSamples += pcmLength / channels / 2;

    int milliseconds = pcmLength / channels / 2 / (samplerate / 1000);
    mProcessedMilliseconds += milliseconds;

    // Overall counter
    mAllProcessedMilliseconds += milliseconds;

    // Get results
    return getResults(report, nullptr, samplerate, codec);
}

struct RgbColor
{
    uint8_t mRed = 0;
    uint8_t mGreen = 0;
    uint8_t mBlue = 0;

    static RgbColor parse(uint32_t rgb)
    {
        RgbColor result;
        result.mBlue = (uint8_t)(rgb & 0xff);
        result.mGreen = (uint8_t)((rgb >> 8) & 0xff);
        result.mRed = (uint8_t)((rgb >> 16) & 0xff);
        return result;
    }

    std::string toHex() const
    {
        char result[7];
        sprintf(result, "%02x%02x%02x", int(mRed), int(mGreen), int(mBlue));
        return std::string(result);
    }
};

int SevanaPVQA::getSize() const
{
    int result = 0;
    result += sizeof(*this);

    // TODO: add PVQA analyzer size
    return result;
}


std::string SevanaPVQA::mosToColor(float mos)
{
    // Limit MOS value by 5.0
    mos = mos > 5.0f ? 5.0f : mos;
    mos = mos < 1.0f ? 1.0f : mos;

    // Split to components
    RgbColor start = RgbColor::parse(MOS_BEST_COLOR), end = RgbColor::parse(MOS_BAD_COLOR);

    float mosFraction = (mos - 1.0f) / 4.0f;

    end.mBlue += (uint8_t)((start.mBlue - end.mBlue) * mosFraction);
    end.mGreen += (uint8_t)((start.mGreen - end.mGreen) * mosFraction);
    end.mRed += (uint8_t)((start.mRed - end.mRed) * mosFraction);

    return end.toHex();
}

} // end of namespace MT
#endif

#if defined(USE_AQUA_LIBRARY)

#include <string.h>
#include "helper/HL_String.h"
#include <sstream>
#include <json/json.h>

namespace MT
{

int SevanaAqua::initializeLibrary(const std::string& pathToLicenseFile)
{
    //char buffer[pathToLicenseFile.length() + 1];
    //strcpy(buffer, pathToLicenseFile.c_str());
    return SSA_InitLib(const_cast<char*>(pathToLicenseFile.data()));
}


void SevanaAqua::releaseLibrary()
{
    SSA_ReleaseLib();
}

std::string SevanaAqua::FaultsReport::toText() const
{
    std::ostringstream oss;

    if (mSignalAdvancedInMilliseconds > -4999.0)
        oss <<  "Signal advanced in milliseconds: " << mSignalAdvancedInMilliseconds << std::endl;
    if (mMistimingInPercents > -4999.0)
        oss << "Mistiming in percents: " << mMistimingInPercents << std::endl;

    for (ResultMap::const_iterator resultIter = mResultMap.begin(); resultIter != mResultMap.end(); resultIter++)
    {
        oss << resultIter->first << ":\t\t\t" << resultIter->second.mSource << "  :   \t" << resultIter->second.mDegrated << " \t" << resultIter->second.mUnit << std::endl;
    }

    return oss.str();
}

Json::Value SevanaAqua::FaultsReport::toJson() const
{
    std::ostringstream oss;
    Json::Value result;

    result["Mistiming"] = mMistimingInPercents;
    result["SignalAdvanced"] = mSignalAdvancedInMilliseconds;
    Json::Value items;
    for (ResultMap::const_iterator resultIter = mResultMap.begin(); resultIter != mResultMap.end(); resultIter++)
    {
        Json::Value item;
        item["name"] = resultIter->first;
        item["source"] = resultIter->second.mSource;
        item["degrated"] = resultIter->second.mDegrated;
        item["unit"] = resultIter->second.mUnit;

        items.append(item);
    }

    result["items"] = items;

    return result;
}

std::string SevanaAqua::getVersion()
{
    TSSA_AQuA_Info* info = SSA_GetPAQuAInfo();
    if (info)
        return info->dVersionString;

    return "";
}

SevanaAqua::SevanaAqua()
{
    open();
}

SevanaAqua::~SevanaAqua()
{
    close();
}

void SevanaAqua::open()
{
    std::unique_lock<std::mutex> l(mMutex);
    if (mContext)
        return;

    mContext = SSA_CreateAudioQualityAnalyzer();
    if (!mContext)
        ;

    //setParam("OutputFormats", "json");
}

void SevanaAqua::close()
{
    std::unique_lock<std::mutex> l(mMutex);
    if (!mContext)
        return;
    SSA_ReleaseAudioQualityAnalyzer(mContext);
    mContext = nullptr;
}

bool SevanaAqua::isOpen() const
{
    return mContext != nullptr;
}

void SevanaAqua::setTempPath(const std::string& temp_path)
{
    mTempPath = temp_path;
}

std::string SevanaAqua::getTempPath() const
{
    return mTempPath;
}

SevanaAqua::CompareResult SevanaAqua::compare(AudioBuffer& reference, AudioBuffer& test)
{
    // Clear previous temporary file
    if (!mTempPath.empty())
        ::remove(mTempPath.c_str());

    // Result value
    CompareResult r;

    std::unique_lock<std::mutex> l(mMutex);

    if (!mContext || !reference.isInitialized() || !test.isInitialized())
        return r;

    // Make analysis
    TSSA_AQuA_AudioData aad;
    aad.dSrcData.dNChannels = reference.mChannels;
    aad.dSrcData.dSampleRate = reference.mRate;
    aad.dSrcData.pSamples = (short*)reference.mData->data();
    aad.dSrcData.dNSamples = (long)reference.mData->size() / 2 / reference.mChannels;

    aad.dTstData.dNChannels = test.mChannels;
    aad.dTstData.dSampleRate = test.mRate;
    aad.dTstData.pSamples = (short*)test.mData->data();
    aad.dTstData.dNSamples = (long)test.mData->size() / 2 / test.mChannels;

    int rescode;
    rescode = SSA_OnTestAudioData(mContext, &aad);
    if (rescode)
        return r;

    // Get results

    int len = SSA_GetQualityStringSize(mContext);
    char* qs = (char*)alloca(len + 10);
    SSA_FillQualityString(mContext, qs);
    //std::cout << qs << std::endl;
    std::istringstream iss(qs);
    while (!iss.eof())
    {
        std::string l;
        std::getline(iss, l);

        // Split by :
        std::vector<std::string> p;
        StringHelper::split(l, p, "\t");
        if (p.size() == 3)
        {
            p[1] = StringHelper::trim(p[1]);
            p[2] = StringHelper::trim(p[2]);
            r.mReport[p[1]] = p[2];
        }
    }

    len = SSA_GetSrcSignalSpecSize(mContext);
    float* srcSpecs = new float[len];
    SSA_FillSrcSignalSpecArray(mContext, srcSpecs);
    Json::Value src_spec_signal;

    for(int i=0; i<16 && i<len; i++)
        src_spec_signal.append(srcSpecs[i]);
    delete[] srcSpecs;

    r.mReport["SrcSpecSignal"] = src_spec_signal;

    len = SSA_GetTstSignalSpecSize(mContext);
    float* tstSpecs = new float[len];
    SSA_FillTstSignalSpecArray(mContext, tstSpecs);
    Json::Value tst_spec_signal;

    for(int i=0; i<16 && i<len; i++)
        tst_spec_signal.append(tstSpecs[i]);
    r.mReport["TstSpecSignal"] = tst_spec_signal;

    delete[] tstSpecs;

    char* faults_str = nullptr;
    int faults_str_len = 0;

    if (mTempPath.empty())
    {
        faults_str_len = SSA_GetFaultsAnalysisStringSize(mContext);

        if (faults_str_len > 0) {
            faults_str = new char[faults_str_len + 1];
            SSA_FillFaultsAnalysisString(mContext, faults_str);
            faults_str[faults_str_len] = 0;
        }
    }

    char* pairs_str = nullptr;
    int pairs_str_len = SSA_GetSpecPairsStringSize(mContext);
    if (pairs_str_len > 0)
    {
        char *pairs_str = new char[pairs_str_len + 1];
        SSA_FillSpecPairsString(mContext, pairs_str, pairs_str_len);
        pairs_str[pairs_str_len] = 0;
    }

    TSSA_AQuA_Results iResults;
    SSA_FillQualityResultsStruct(mContext, &iResults);
    r.mReport["dPercent"] = iResults.dPercent;
    r.mReport["dMOSLike"] = iResults.dMOSLike;

    if (faults_str_len > 0)
    {
        std::istringstream iss(faults_str);
        r.mFaults = loadFaultsReport(iss);
    }
    else
        if (!mTempPath.empty())
        {
            std::ifstream ifs(mTempPath.c_str());
            r.mFaults = loadFaultsReport(ifs);
        }

    delete[] faults_str; faults_str = nullptr;
    delete[] pairs_str; pairs_str = nullptr;

    r.mMos = (float)iResults.dMOSLike;

    return r;
}

void SevanaAqua::configureWith(const Config& config)
{
    if (!mContext)
        return;

    for (auto& item: config)
    {
        const std::string& name = item.first;
        const std::string& value = item.second;
        if (!SSA_SetAnyString(mContext, const_cast<char *>(name.c_str()), const_cast<char *>(value.c_str())))
            throw std::runtime_error(std::string("SSA_SetAnyString returned failed for pair ") + name + " " + value);
    }
}

SevanaAqua::Config SevanaAqua::parseConfig(const std::string& line)
{
    Config result;

    // Split command line to parts
    std::vector<std::string> pl;
    StringHelper::split(line, pl, "-");

    for (const std::string& s: pl)
    {
        std::string::size_type  p = s.find(' ');
        if (p != std::string::npos)
        {
            std::string name = StringHelper::trim(s.substr(0, p));
            std::string value = StringHelper::trim(s.substr(p + 1));

            result[name] = value;
        }
    }

    return result;
}


SevanaAqua::PFaultsReport SevanaAqua::loadFaultsReport(std::istream& input)
{
    PFaultsReport result = std::make_shared<FaultsReport>();
    std::string line;
    std::vector<std::string> parts;

    // Parse output
    while (!input.eof())
    {
        std::getline(input, line);
        if (line.size() < 3)
            continue;

        std::string::size_type p = line.find(":");
        if (p != std::string::npos)
        {
            std::string name = StringHelper::trim(line.substr(0, p));

            FaultsReport::Result r;

            // Split report line to components
            parts.clear();
            StringHelper::split(line.substr(p + 1), parts, " \t");

            // Remove empty components
            parts.erase(std::remove_if(parts.begin(), parts.end(), [](const std::string& item){return item.empty();}), parts.end());

            if (parts.size() >= 2)
            {
                r.mSource = parts[0];
                r.mDegrated = parts[1];
                if (parts.size()> 2)
                    r.mUnit = parts[2];
                result->mResultMap[name] = r;
            }
        }
        else
        {
            p = line.find("ms.");
            if (p != std::string::npos)
            {
                parts.clear();
                StringHelper::split(line, parts, " \t");
                if (parts.size() >= 3)
                {
                    if (parts.back() == "ms.")
                        result->mSignalAdvancedInMilliseconds = (float)std::atof(parts[parts.size() - 2].c_str());
                }
            }
            else
            {
                p = line.find("percent.");
                if (p != std::string::npos)
                {
                    parts.clear();
                    StringHelper::split(line, parts, " \t");
                    if (parts.size() >= 3)
                    {
                        if (parts.back() == "percent.")
                            result->mMistimingInPercents = (float)std::atof(parts[parts.size() - 2].c_str());
                    }
                }
            }
        }
    }

    return result;
}

} // end of namespace MT

// It is to workaround old AQuA NDK build - it has reference to ftime
/*#if defined(TARGET_ANDROID)
#include <sys/timeb.h>

// This was removed from POSIX 2008.
int ftime(struct timeb* tb) {
  struct timeval  tv;
  struct timezone tz;

  if (gettimeofday(&tv, &tz) < 0)
    return -1;

  tb->time    = tv.tv_sec;
  tb->millitm = (tv.tv_usec + 500) / 1000;

  if (tb->millitm == 1000) {
    ++tb->time;
    tb->millitm = 0;
  }

  tb->timezone = tz.tz_minuteswest;
  tb->dstflag  = tz.tz_dsttime;

  return 0;
}
#endif*/

#endif

