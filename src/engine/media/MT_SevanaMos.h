#ifndef _SEVANA_MOS_H
#define _SEVANA_MOS_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <map>
#include <memory.h>

#if defined(USE_PVQA_LIBRARY)
# include "pvqa.h"
# if !defined(PVQA_INTERVAL)
#  define PVQA_INTERVAL (0.68)
# endif
#endif

#if defined(USE_AQUA_LIBRARY)
# include "aqua.h"
# include <json/json.h>
#endif


namespace MT
{
  enum class ReportType
  {
    PlainText,
    Html
  };
  
  #if defined(USE_PVQA_LIBRARY)
  class SevanaMosUtility
  {
  public:
    // Returns MOS estimation as text representation of float value or "failed" word.
    static void run(const std::string& pcmPath, const std::string& intervalPath,
                    std::string& estimation, std::string& intervals);
  };

  extern float getSevanaMos(const std::string& audioPath, const std::string& intervalReportPath,
                            std::string& intervalReport);

  class SevanaPVQA
  {
  public:
    enum class Model
    {
      Stream,
      Interval
    };

  protected:
    static void* mLibraryConfiguration;
    static int mLibraryErrorCode;
    static std::atomic_int mInstanceCounter;
    static std::atomic_uint_least64_t mAllProcessedMilliseconds;
    static bool mPvqaLoaded;

    void* mContext = nullptr;
    Model mModel = Model::Interval;
    double mIntervalLength = 0.68;
    uint64_t mProcessedSamples = 0,
      mProcessedMilliseconds = 0;

    bool mAudioLineInitialized = false;
    std::string mDumpWavPath;
    bool mOpenFailed = false;

  public:
    static std::string getVersion();

    // Required to call before any call to SevanaPVQA instance methods
    
  #if defined(TARGET_ANDROID)
    static void setupAndroidEnvironment(void* environment, void* appcontext);
  #endif
    static bool initializeLibrary(const std::string& pathToLicenseFile, const std::string& pathToConfigFile);
    static bool isInitialized();

    static int getLibraryError();
    static void releaseLibrary();
    static int getInstanceCounter() { return mInstanceCounter; }
    static uint64_t getProcessedMilliseconds() { return mAllProcessedMilliseconds; }

    SevanaPVQA();
    ~SevanaPVQA();

    void open(double interval, Model model);
    void close();
    bool isOpen() const;

    // Update/Get model
    void update(int samplerate, int channels, const void* pcmBuffer, int pcmLength);

    typedef std::vector<std::vector<float>> EchoData;
    enum class Codec
    {
      None,
      G711,
      ILBC,
      G722,
      G729,
      GSM,
      AMRNB,
      AMRWB,
      OPUS
    };

    float getResults(std::string& report, const EchoData** echo, int samplerate, Codec codec);

    // Report is interval report. Names are output detector names. startIndex is column's start index in interval report of first detector.
    struct DetectorsList
    {
      std::vector<std::string> mNames;
      int mStartIndex = 0;
    };

    static DetectorsList getDetectorsNames(const std::string& report);

    // Get MOS in one shot
    void setPathToDumpFile(const std::string& path);
    float process(int samplerate, int channels, const void* pcmBuffer, int pcmLength, std::string& report, Codec codec);

    Model getModel() const { return mModel; }

    int getSize() const;

    static std::string mosToColor(float mos);
  };

  typedef std::shared_ptr<SevanaPVQA> PSevanaPVQA;
  #endif

  #if defined(USE_AQUA_LIBRARY)
  class SevanaAqua
  {
  protected:
    void* mContext = nullptr;
    std::mutex mMutex;
    std::string mTempPath;

  public:
    // Returns 0 (zero) on successful initialization, otherwise it is error code
    static int initializeLibrary(const std::string& pathToLicenseFile);
    static void releaseLibrary();
    static std::string getVersion();

    SevanaAqua();
    ~SevanaAqua();

    void open();
    void close();
    bool isOpen() const;

    void setTempPath(const std::string& temp_path);
    std::string getTempPath() const;

    typedef std::map<std::string, std::string> Config;
    void configureWith(const Config& config);
    static Config parseConfig(const std::string& line);

    // Report is returned in JSON format
    struct AudioBuffer
    {
        AudioBuffer()
        {}

        AudioBuffer(int size)
        {
            mData = std::make_shared<std::vector<unsigned char>>();
            mData->resize(size);
        }

        AudioBuffer(const void* data, int size)
        {
            mData = std::make_shared<std::vector<unsigned char>>();
            mData->resize(size);
            memcpy(mData->data(), data, size);
        }

        void* data()
        {
            return mData ? mData->data() : nullptr;
        }

        const void* data() const
        {
            return mData ? mData->data() : nullptr;
        }

        int size() const
        {
            return mData ? mData->size() : 0;
        }

        int mRate = 8000;
        int mChannels = 1;
        std::shared_ptr<std::vector<unsigned char>> mData;
        bool isInitialized() const { return mRate > 0 && mChannels > 0 && mData; }
    };

      
    struct FaultsReport
    {
      float mSignalAdvancedInMilliseconds = -5000.0;
      float mMistimingInPercents = -5000.0;

      struct Result
      {
        std::string mSource, mDegrated, mUnit;
      };
      typedef std::map<std::string, Result> ResultMap;
      ResultMap mResultMap;

      Json::Value toJson() const;
      std::string toText() const;
    };

    typedef std::shared_ptr<FaultsReport> PFaultsReport;

    static PFaultsReport loadFaultsReport(std::istream& input);

    // Compare in one shot. Report will include text representation of json report.
    struct CompareResult
    {
      float mMos = 0.0f;
      Json::Value mReport;
      PFaultsReport mFaults;
    };

    CompareResult compare(AudioBuffer& reference, AudioBuffer& test);
  };
  typedef std::shared_ptr<SevanaAqua> PSevanaAqua;
  #endif
}

#endif
