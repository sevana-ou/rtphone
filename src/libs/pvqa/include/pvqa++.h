#ifndef _SEVANA_MOS_H
#define _SEVANA_MOS_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <map>
#include <memory.h>

# include "pvqa.h"
# if !defined(PVQA_INTERVAL)
#  define PVQA_INTERVAL (0.68)
# endif

namespace sevana {

enum
{
    pvqa_success = 0,
    pvqa_bad_config = 1,
    pvqa_not_initialized = 1000,
    pvqa_too_much_streams = 1001,
    pvqa_no_context = 1002
};

class detector_report
{
public:
    detector_report();
    detector_report(const std::string& report);
    ~detector_report();

    // Detector names
    const std::vector<std::string>& names() const;

    enum status
    {
        status_ok,
        status_poor,
        status_uncertain
    };

    // Row for single time interval
    struct row
    {
        float mStart = 0.0f, mEnd = 0.0f;
        std::vector<float> mData;
        status mStatus;
    };

    // All rows
    const std::vector<row>& rows() const;

    // Find difference in impairements
    detector_report get_changes(const detector_report& reference);

    // Get short report
    std::string to_string() const;

    enum display_options
    {
        show_regular,
        show_difference
    };

    std::string to_table(display_options options = show_regular, const char delimiter = ';') const;
    std::string to_json(display_options options = show_regular, bool dedicated_time_fields = true) const;

    // Find number of impairements for whole report
    size_t count_of(const std::string& detector_name) const;

private:
    std::vector<row> mRows;
    std::vector<std::string> mDetectorNames;
};

class pvqa
{
private:
    static void* mLibraryConfiguration;
    static int mLibraryErrorCode;
    static std::atomic_int mInstanceCounter;
    static std::atomic_uint_least64_t mAllProcessedMilliseconds;
    static bool mPvqaLoaded;

    void* mContext = nullptr;
    double mIntervalLength = 0.68;
    size_t mProcessedSamples = 0,
           mProcessedMilliseconds = 0;

    bool mAudioLineInitialized = false;
    std::string mDumpWavPath;
    int mErrorCode = 0;
    size_t mRate = 0, mChannels = 1;

public:
    static std::string version();
    static std::string mos_to_color(float mos);
    struct DetectorsList
    {
        std::vector<std::string> mNames;
        int mStartIndex = 0;
    };

    static DetectorsList detectors_names(const std::string& report);

#if defined(TARGET_ANDROID)
    // Required to call before any call to SevanaPVQA instance methods
    static void setupAndroidEnvironment(void* environment, void* appcontext);
#endif

    // Path to config file can be empty
    // In this case library will be considered initialized (but will produce zero MOS)
    static bool initialize(const std::string& pathToLicenseFile, const std::string& pathToConfigFile);
    static bool initialize(const void* license_buffer, size_t license_len,
            const void* config_buffer, size_t config_len);
    static bool is_initialized();

    static int library_error();
    static void release();
    static int instance_counter() { return mInstanceCounter; }
    static uint64_t processed_milliseconds() { return mAllProcessedMilliseconds; }

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
    static float max_mos(Codec c);

    pvqa();
    ~pvqa();

    bool open(size_t rate, size_t channels, double interval = 0.680);
    void close();
    bool is_open() const;

    // Update/Get model. pcm_length is bytes.
    bool update(const void* pcm_buffer, size_t pcm_length);
    int error() const;

    typedef std::vector<std::vector<float>> echo_data;
    struct result
    {
        float mMos = 0.0f;
        std::string mReport;
        size_t mIntervals = 0, mPoorIntervals = 0;
    };

    bool get_result(result& r, size_t last_milliseconds = 0);
    struct detector_data
    {
        // Names corresponding to mData array
        std::vector<std::string> mNames;

        // Zero element in every row is time offset (in milliseconds)
        std::vector<std::vector<float>> mData;
    };

    detector_data get_detector(const std::string& name, float start_time, float end_time);
    echo_data get_echo();

    // Combines update & get_result calls
    bool singleshot(result& r, const void* pcm_buffer, size_t pcm_length);
};

} // end of namespace

#endif
