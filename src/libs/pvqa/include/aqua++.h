#ifndef _AQUA_CPP_H
#define _AQUA_CPP_H

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <map>
#include <memory.h>

#include "aqua.h"

namespace sevana
{

class aqua
{
protected:
    void* mContext = nullptr;
    std::mutex mMutex;

public:
    // Returns 0 (zero) on successful initialization, otherwise it is error code
    static int initialize(const std::string& pathToLicenseFile);
    static int initialize(const void* buffer, size_t len);

    // Deinitialize library
    static void release();

    // Current version number
    static std::string version();

    aqua();
    ~aqua();

    // Open library instance
    void open();

    // Close library instance
    void close();
    bool is_open() const;

    // Config
    typedef std::map<std::string, std::string> config;
    void configure_with(const config& config);
    static config parse(const std::string& line);

    struct audio_buffer
    {
        audio_buffer()
        {}

        audio_buffer(int size)
        {
            mData = std::make_shared<std::vector<unsigned char>>();
            mData->resize(size);
        }

        audio_buffer(const void* data, int size)
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
        bool is_initialized() const { return mRate > 0 && mChannels > 0 && mData; }
    };


    struct faults_report
    {
        float mSignalAdvancedInMilliseconds = -5000.0;
        float mMistimingInPercents = -5000.0;

        struct item
        {
            std::string mSource, mDegrated, mUnit;
        };
        typedef std::map<std::string, item> result_map;
        result_map mResultMap;

        std::string text() const;
    };

    static faults_report load_report(std::istream& input);

    // Compare in one shot. Report will include text representation of json report.
    struct result
    {
        float mMos = 0.0f, mPercents = 0.0f, mPesqLike = 0.0f;
        faults_report mFaults;

        // This can be both plain text or json depending on configuration parameter 'output': 'txt' or 'json'
        std::string mFaultsText;
    };

    result compare(audio_buffer& reference, audio_buffer& test);
};

}

#endif
