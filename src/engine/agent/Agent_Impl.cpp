#include "Agent_Impl.h"
#include "json/json.h"
#include "helper/HL_String.h"
#include "helper/HL_StreamState.h"
#include "helper/HL_VariantMap.h"
#include "helper/HL_CsvReader.h"
#include <fstream>

#if defined(USE_PVQA_LIBRARY)
# include "pvqa++.h"
#endif

#if defined(USE_AQUA_LIBRARY)
# include "aqua++.h"
#endif

const std::string Status_Ok = "ok";
const std::string Status_SessionNotFound = "session not found";
const std::string Status_AccountNotFound = "account not found";
const std::string Status_FailedToOpenFile = "failed to open file";
const std::string Status_NoActiveProvider = "no active provider";
const std::string Status_NoMediaAction = "no valid media action";
const std::string Status_NoCommand = "no valid command";

#define LOG_SUBSYSTEM "Agent"

AgentImpl::AgentImpl()
    :mShutdown(false), mEventListChangeCondVar()
{
#if defined(TARGET_ANDROID) || defined(TARGET_WIN)
    ice::GLogger.useDebugWindow(true);
#endif
    ice::GLogger.setLevel(ice::LogLevel::LL_DEBUG);
}

AgentImpl::~AgentImpl()
{
    stopAgentAndThread();
}

void AgentImpl::run()
{
    while (!mShutdown)
    {
        {
            std::unique_lock<std::recursive_mutex> l(mAgentMutex);
            process();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

std::string AgentImpl::command(const std::string& command)
{
    if (command.empty())
        return "";

    Json::Value d, answer;
    try
    {
        Json::Reader r;
        if (!r.parse(command, d))
            return "";

        std::string cmd = d["command"].asString();
        if (cmd != "wait_for_event")
        {
            ICELogInfo(<< command);
        }
        if (cmd == "config")
            processConfig(d, answer);
        else
        if (cmd == "start")
            processStart(d, answer);
        else
        if (cmd == "stop")
            processStop(d, answer);
        else
        if (cmd == "account_create")
            processCreateAccount(d, answer);
        else
        if (cmd == "account_start")
            processStartAccount(d, answer);
        else
        if (cmd == "account_setuserinfo")
            processSetUserInfoToAccount(d, answer);
        else
        if (cmd == "session_create")
            processCreateSession(d, answer);
        else
        if (cmd == "session_start")
            processStartSession(d, answer);
        else
        if (cmd == "session_stop")
            processStopSession(d, answer);
        else
        if (cmd == "session_accept")
            processAcceptSession(d, answer);
        else
        if (cmd == "session_destroy")
            processDestroySession(d, answer);
        else
        if (cmd == "session_use_stream")
            processUseStreamForSession(d, answer);
        else
        if (cmd == "wait_for_event")
            processWaitForEvent(d, answer);
        else
        if (cmd == "session_get_media_stats")
            processGetMediaStats(d, answer);
        else
        if (cmd == "agent_network_changed")
            processNetworkChanged(d, answer);
        else
        if (cmd == "agent_add_root_cert")
            processAddRootCert(d, answer);
        else
        if (cmd == "detach_log")
        {
            GLogger.closeFile();
            answer["status"] = Status_Ok;
        }
        else
        if (cmd == "attach_log")
        {
            GLogger.openFile();
            answer["status"] = Status_Ok;
        }
        else
        if (cmd == "log_message")
            processLogMessage(d, answer);
        else
        {
            answer["status"] = Status_NoCommand;
        }
    }
    catch(std::exception& e)
    {
        answer["status"] = e.what();
    }
    return answer.toStyledString();
}

bool AgentImpl::waitForData(int /*milliseconds*/)
{
    return false;
}

std::string AgentImpl::read()
{
    return "";
}

void AgentImpl::processConfig(Json::Value &d, Json::Value &answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);

#if !defined(TARGET_ANDROID) && defined(USE_PVQA_LIBRARY)
    // It works for desktop OSes only
    // Because Android requires special initializing procedure (valid JNI environment context)
    std::string pvqaLicense = d["pvqa-license"].asString(), pvqaConfig = d["pvqa-config"].asString();
    if (!pvqaLicense.empty() && !pvqaConfig.empty())
        sevana::pvqa::initialize(pvqaLicense, pvqaConfig);
#endif

#if !defined(TARGET_ANDROID) && defined(USE_AQUA_LIBRARY)
    std::string aquaLicense = d["aqua-license"].asString();
    if (!aquaLicense.empty())
        sevana::aqua::initialize(aquaLicense.c_str(), aquaLicense.size());
#endif

    std::string transport = d["transport"].asString();
    config()[CONFIG_TRANSPORT] = (transport == "any") ? 0 : (transport == "udp" ? 1 : (transport == "tcp" ? 2 : 3));
    config()[CONFIG_IPV4] = d["ipv4"].asBool();
    config()[CONFIG_IPV6] = d["ipv6"].asBool();

    // Log file
    std::string logfile = d["logfile"].asString();
    ice::Logger& logger = ice::GLogger;

    logger.useFile(logfile.empty() ? nullptr : logfile.c_str());

    config()[CONFIG_MULTIPLEXING] = true;
    config()[CONFIG_DISPLAYNAME] = "Voip quality tester";

    mUseNativeAudio = d["nativeaudio"].asBool();
    config()[CONFIG_OWN_DNS] = d["dns_servers"].asString();
    answer["status"] = Status_Ok;
}

void AgentImpl::processStart(Json::Value& /*request*/, Json::Value &answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);
    if (mThread)
    {
        answer["status"] = Status_Ok;
        return; // Started already
    }

    // Start socket thread
    SocketHeap::instance().start();

    // Initialize terminal
    MT::CodecList::Settings settings;
    mTerminal = std::make_shared<MT::Terminal>(settings);

    // Enable/disable codecs
    PVariantMap priorityConfig = std::make_shared<VariantMap>();
    MT::CodecList& cl = mTerminal->codeclist();
    for (int i=0; i<cl.count(); i++)
        if (cl.codecAt(i).payloadType() < 96)
            priorityConfig->at(i) = i;
        else
            priorityConfig->at(i) = -1;

    config()[CONFIG_CODEC_PRIORITY] = priorityConfig;

    // Enable audio
    mAudioManager = std::make_shared<AudioManager>();
    mAudioManager->setTerminal(mTerminal.get());

    // Do not start here. Start right before call.

    // Initialize endpoint
    start();

    // Start worker thread
    mShutdown = false;
    mThread = std::make_shared<std::thread>(&AgentImpl::run, this);
    answer["status"] = Status_Ok;
}

void AgentImpl::processStop(Json::Value& /*request*/, Json::Value& answer)
{
    stopAgentAndThread();
    answer["status"] = Status_Ok;
}

void AgentImpl::processCreateAccount(Json::Value &d, Json::Value& answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);
    PVariantMap c = std::make_shared<VariantMap>();

    (*c)[CONFIG_USERNAME] = d["username"].asString();
    (*c)[CONFIG_PASSWORD] = d["password"].asString();
    (*c)[CONFIG_DOMAIN] = d["domain"].asString();
    (*c)[CONFIG_EXTERNALIP] = d["use_external_ip"].asBool();

    auto nameAndPort = StringHelper::parseHost(d["stun_server"].asString(), 3478);
    (*c)[CONFIG_STUNSERVER_NAME] = nameAndPort.first;
    (*c)[CONFIG_STUNSERVER_PORT] = nameAndPort.second;

    PAccount account = createAccount(c);
    mAccountMap[account->id()] = account;
    answer["account_id"] = account->id();
    answer["status"] = Status_Ok;
}

void AgentImpl::processStartAccount(Json::Value& request, Json::Value& answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);
    // Locate account in map
    auto accountIter = mAccountMap.find(request["account_id"].asInt());
    if (accountIter != mAccountMap.end())
    {
        accountIter->second->start();
        answer["status"] = Status_Ok;
    }
    else
        answer["status"] = Status_AccountNotFound;
}

void AgentImpl::processSetUserInfoToAccount(Json::Value &request, Json::Value &answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);
    // Locate account in map
    auto accountIter = mAccountMap.find(request["account_id"].asInt());
    if (accountIter != mAccountMap.end())
    {
        Account::UserInfo info;
        Json::Value& arg = request["userinfo"];
        std::vector<std::string> keys = arg.getMemberNames();
        for (const std::string& k: keys)
            info[k] = arg[k].asString();
        accountIter->second->setUserInfo(info);

        answer["status"] = Status_Ok;
    }
    else
        answer["status"] = Status_AccountNotFound;
}

void AgentImpl::processCreateSession(Json::Value &request, Json::Value &answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);
    auto accountIter = mAccountMap.find(request["account_id"].asInt());
    if (accountIter != mAccountMap.end())
    {
        PSession session = createSession(accountIter->second);
        mSessionMap[session->id()] = session;
        answer["session_id"] = session->id();
        answer["status"] = Status_Ok;
    }
    else
        answer["status"] = Status_AccountNotFound;
}

void AgentImpl::processStartSession(Json::Value& request, Json::Value& answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);

    // For debugging only
    /*mIncomingAudioDump = std::make_shared<Audio::WavFileWriter>();
  mIncomingAudioDump->open("incoming_dump", AUDIO_SAMPLERATE, AUDIO_CHANNELS);
  mOutgoingAudioDump = std::make_shared<Audio::WavFileWriter>();
  mOutgoingAudioDump->open("outgoing_dump", AUDIO_SAMPLERATE, AUDIO_CHANNELS);*/

    // Start audio manager
    if (!mAudioManager)
    {
        // Agent was not started
        ICELogError(<< "No audio manager installed.");
        answer["status"] = "Audio manager not started. Most probably agent is not started.";
        return;
    }

    mAudioManager->start(mUseNativeAudio ? AudioManager::atReceiver : AudioManager::atNull);

    auto sessionIter = mSessionMap.find(request["session_id"].asInt());
    if (sessionIter != mSessionMap.end())
    {
        // Ensure audio provider is here
        PSession session = sessionIter->second;
        PDataProvider audioProvider = std::make_shared<AudioProvider>(*this, *mTerminal);
        audioProvider->setState(audioProvider->state() | static_cast<int>(StreamState::Grabbing) | static_cast<int>(StreamState::Playing));

#if defined(USE_AQUA_LIBRARY)
        std::string path_faults = request["path_faults"].asString();

        sevana::aqua::config config = {
                { "avlp",              "off"   },
                { "decor",             "off"   },
                { "mprio",             "off"   },
                { "miter",             "1"     },
                { "enorm",             "off"   },
                { "voip",              "on"    },
                { "g711",              "on"    },
                { "spfrcor",           "on"    },
                { "grad",              "off"   },
                { "ratem",             "%%m"   },
                { "trim",              "a 2"   },
                { "output",            "json"  },
                { "fau",               path_faults},
                { "specp",             "32"}
        };

        // std::string config = "-avlp on -smtnrm on -decor off -mprio off -npnt auto -voip off -enorm off -g711 on -spfrcor off -grad off -tmc on -miter 1 -trim a 10 -output json";
        /*if (temp_path.size())
            config += " -fau " + temp_path; */

        auto qc = std::make_shared<sevana::aqua>();
        if (!qc->is_open())
        {
            ICELogError( << "Problem when initializing AQuA library");
        }
        else
            qc->configure_with(config);

        mAquaMap[sessionIter->first] = qc;
        dynamic_cast<AudioProvider*>(audioProvider.get())->configureMediaObserver(this, (void*)qc.get());
#endif

        // TODO: support SRTP via StreamState::Srtp option in audio provider state

        // Get user headers
        Session::UserHeaders info;
        Json::Value& arg = request["userinfo"];
        std::vector<std::string> keys = arg.getMemberNames();
        for (const std::string& k: keys)
            info[k] = arg[k].asString();
        session->setUserHeaders(info);

        session->addProvider(audioProvider);
        session->start(request["target"].asString());
        answer["status"] = Status_Ok;
    }
    else
    {
        answer["status"] = Status_SessionNotFound;
    }
}

void AgentImpl::processStopSession(Json::Value& request, Json::Value& answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);

    auto sessionIter = mSessionMap.find(request["session_id"].asInt());
    if (sessionIter != mSessionMap.end())
    {
        PSession session = sessionIter->second;
        session->stop();
        answer["status"] = Status_Ok;
    }
    else
        answer["status"] = Status_SessionNotFound;
}

void AgentImpl::processAcceptSession(Json::Value& request, Json::Value& answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);
    auto sessionIter = mSessionMap.find(request["session_id"].asInt());
    if (sessionIter != mSessionMap.end())
    {
        // Ensure audio manager is here
        mAudioManager->start(mUseNativeAudio ? AudioManager::atReceiver : AudioManager::atNull);

        // Accept session on SIP level
        PSession session = sessionIter->second;

        // Get user headers
        Session::UserHeaders info;
        Json::Value& arg = request["userinfo"];
        std::vector<std::string> keys = arg.getMemberNames();
        for (const std::string& k: keys)
            info[k] = arg[k].asString();
        session->setUserHeaders(info);

        // Accept finally
        session->accept();

        answer["status"] = Status_Ok;
    }
    else
        answer["status"] = Status_SessionNotFound;

}

void AgentImpl::processDestroySession(Json::Value& request, Json::Value& answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);

    int sessionId = request["session_id"].asInt();
    auto sessionIter = mSessionMap.find(sessionId);
    if (sessionIter != mSessionMap.end())
        mSessionMap.erase(sessionIter);
#if defined(USE_AQUA_LIBRARY)
    closeAqua(sessionId);
#endif
    answer["status"] = Status_Ok;
}

void AgentImpl::processWaitForEvent(Json::Value &request, Json::Value &answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);

    //int x = 0;
    //int y = 1/x;

    int timeout = request["timeout"].asInt();
    std::unique_lock<std::mutex> eventLock(mEventListMutex);
    if (mEventList.empty())
        mEventListChangeCondVar.wait_for(eventLock, chrono::milliseconds(timeout));

    if (!mEventList.empty())
    {
        answer = mEventList.front();
        mEventList.erase(mEventList.begin());
    }
    answer["status"] = Status_Ok;
}

#if defined(USE_PVQA_LIBRARY)
static Json::Value CsvReportToJson(const std::string& report)
{
    Json::Value detectorValues;
    std::istringstream iss(report);
    CsvReader reader(iss);
    std::vector<std::string> cells;
    if (reader.readLine(cells))
    {
        Json::Value detectorNames;
        for (size_t nameIndex = 0; nameIndex < cells.size(); nameIndex++)
            detectorNames[static_cast<int>(nameIndex)] = StringHelper::trim(cells[nameIndex]);
        // Put first line name of columns
        detectorValues[0] = detectorNames;

        int rowIndex = 1;
        while (reader.readLine(cells))
        {
            // Skip last column for now
            Json::Value row;
            for (size_t valueIndex = 0; valueIndex < cells.size(); valueIndex++)
            {
                bool isFloat = true;
                float v = StringHelper::toFloat(cells[valueIndex], 0.0, &isFloat);
                if (isFloat)
                    row[static_cast<int>(valueIndex)] = static_cast<double>(v);
                else
                    row[static_cast<int>(valueIndex)] = cells[valueIndex];
            }
            detectorValues[rowIndex++] = row;
        }
    }
    return detectorValues;
}
#endif

void AgentImpl::processGetMediaStats(Json::Value& request, Json::Value& answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);
    int sessionId = request["session_id"].asInt();
    SessionMap::iterator sessionIter = mSessionMap.find(sessionId);
    if (sessionIter != mSessionMap.end())
    {
        PSession session = sessionIter->second;
        VariantMap result;
        bool includePvqa = request["include_pvqa"].asBool();
#if defined(USE_AQUA_LIBRARY)
        bool includeAqua = request["include_aqua"].asBool();
        std::string aquaReference = request["aqua_reference_audio"].asString();
#endif
        session->getSessionInfo(includePvqa ? Session::InfoOptions::Detailed : Session::InfoOptions::Standard,
                                result);

        if (result.exists(SessionInfo_AudioCodec))
            answer["codec"] = result[SessionInfo_AudioCodec].asStdString();
        if (result.exists(SessionInfo_NetworkMos))
            answer["network_mos"] = result[SessionInfo_NetworkMos].asFloat();
#if defined(USE_PVQA_LIBRARY)
        if (result.exists(SessionInfo_PvqaMos))
            answer["pvqa_mos"] = result[SessionInfo_PvqaMos].asFloat();
        if (result.exists(SessionInfo_PvqaReport))
            answer["pvqa_report"] = CsvReportToJson(result[SessionInfo_PvqaReport].asStdString());
#endif
        if (result.exists(SessionInfo_PacketLoss))
            answer["rtp_lost"] = result[SessionInfo_LostRtp].asInt();
        if (result.exists(SessionInfo_SentRtp))
            answer["rtp_sent"] = result[SessionInfo_SentRtp].asInt();
        if (result.exists(SessionInfo_ReceivedRtp))
            answer["rtp_received"] = result[SessionInfo_ReceivedRtp].asInt();
        if (result.exists(SessionInfo_Duration))
            answer["duration"] = result[SessionInfo_Duration].asInt();
        if (result.exists(SessionInfo_Jitter))
            answer["jitter"] = result[SessionInfo_Jitter].asFloat() * 1000; // Take milliseconds
        if (result.exists(SessionInfo_Rtt))
            answer["rtt"] = result[SessionInfo_Rtt].asFloat();
        if (result.exists(SessionInfo_BitrateSwitchCounter))
            answer["bitrate_switch_counter"] = result[SessionInfo_BitrateSwitchCounter].asInt();
        if (result.exists(SessionInfo_SSRC))
            answer["rtp_ssrc"] = result[SessionInfo_SSRC].asInt();
        if (result.exists(SessionInfo_RemotePeer))
            answer["rtp_remotepeer"] = result[SessionInfo_RemotePeer].asStdString();

#if defined(USE_AQUA_LIBRARY)
        if (includeAqua)
        {
            // Send recorded audio to upper level
            answer["incoming_audio"] = mAquaIncoming.hexstring();

            ICELogInfo(<< "Running AQuA analyzer.");
            ByteBuffer referenceAudio;
            // Read AQuA reference audio from file if available
            if (aquaReference.empty())
            {
                ICELogCritical(<< "AQuA reference audio file is not set, skipping analyzing.");
            }
            else {
                auto sa = mAquaMap[sessionIter->first];
                if (sa) {
                    Audio::WavFileReader reader;
                    reader.open(StringHelper::makeTstring(aquaReference));

                    if (reader.isOpened()) {
                        char buffer[1024];
                        int wasRead = 0;
                        do {
                            wasRead = reader.read(buffer, 1024);
                            if (wasRead > 0)
                                referenceAudio.appendBuffer(buffer, wasRead);
                        } while (wasRead == 1024);
                    }

                    sevana::aqua::audio_buffer test(mAquaIncoming.data(), mAquaIncoming.size()),
                            reference(referenceAudio.data(), referenceAudio.size());
                    test.mRate = AUDIO_SAMPLERATE;
                    reference.mRate = AUDIO_SAMPLERATE;
                    test.mChannels = AUDIO_CHANNELS;
                    reference.mChannels = AUDIO_CHANNELS;
                    ICELogInfo(
                            << "Comparing test audio " << mAquaIncoming.size() << " bytes with reference audio " << referenceAudio.size() << " bytes.");
                    auto r = sa->compare(reference, test);
                    if (r.mErrorCode) {
                        ICELogInfo(
                                << "Error code: " << r.mErrorCode << ", msg: " << r.mErrorMessage);
                    } else {
                        ICELogInfo(<< "MOS: " << r.mMos << ", faults: " << r.mFaultsText);
                    }
                    answer["aqua_mos"] = r.mMos;
                    answer["aqua_report"] = r.mFaultsText;
                    if (r.mErrorCode) {
                        answer["aqua_error_code"] = r.mErrorCode;
                        answer["aqua_error_message"] = r.mErrorMessage;
                    }
                    closeAqua(sessionIter->first);
                }
            }
            // Remove test audio
            mAquaIncoming.clear(); mAquaOutgoing.clear();
        }
#endif

        answer["status"] = Status_Ok;
    }
    else
        answer["status"] = Status_SessionNotFound;
}

void AgentImpl::processNetworkChanged(Json::Value& /*request*/, Json::Value& /*answer*/)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);
}

const std::string BeginCertificate = "-----BEGIN CERTIFICATE-----";
const std::string EndCertificate = "-----END CERTIFICATE-----";

void AgentImpl::processAddRootCert(Json::Value& request, Json::Value& answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);
    std::string pem = request["cert"].asString();

    std::string::size_type pb = 0, pe = 0;

    for(pb = pem.find(BeginCertificate, pb), pe = pem.find(EndCertificate, pe);
        pb != std::string::npos && pe != std::string::npos;
        pb = pem.find(BeginCertificate, pb + BeginCertificate.size()), pe = pem.find(EndCertificate, pe + EndCertificate.size()))
    {
        // Get single certificate
        std::string cert = pem.substr(pb, pe + EndCertificate.size());
        //int size = cert.size();
        addRootCert(ByteBuffer(cert.c_str(), cert.size()));

        // Delete processed part
        pem.erase(0, pe + EndCertificate.size());
    }

    answer["status"] = Status_Ok;
}

void AgentImpl::processLogMessage(Json::Value &request, Json::Value &answer)
{
    int level = request["level"].asInt();
    std::string message = request["message"].asString();

    ICELog(static_cast<ice::LogLevel>(level), "App", << message);

    answer["status"] = Status_Ok;
}

void AgentImpl::stopAgentAndThread()
{
    // Stop user agent
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);
    try
    {
        stop();
    }
    catch (...)
    {}

    // Stop worker thread
    if (mThread)
    {
        mShutdown = true;
        if (mThread->joinable())
        {
            l.unlock();
            mThread->join();
            l.lock();
        }
        mThread.reset();
    }

    // Close used audio
    if (mAudioManager)
    {
        // Ensure audio manager is stopped
        mAudioManager->stop(mUseNativeAudio ? AudioManager::atReceiver : AudioManager::atNull);

        // Free audio manager
        mAudioManager.reset();
    }

    // Close terminal after audio manager - because audio manager has reference to terminal
    mTerminal.reset();

    SocketHeap::instance().stop();
}

void AgentImpl::processUseStreamForSession(Json::Value& request, Json::Value& answer)
{
    std::unique_lock<std::recursive_mutex> l(mAgentMutex);
    SessionMap::iterator sessionIter = mSessionMap.find(request["session_id"].asInt());
    if (sessionIter != mSessionMap.end())
    {
        // Extract ptr to session
        PSession session = sessionIter->second;

        // Parse command
        std::string actionText = request["media_action"].asString(),
                directionText = request["media_direction"].asString();

        MT::Stream::MediaDirection direction = directionText == "incoming" ? MT::Stream::MediaDirection::Incoming
                                                                           : MT::Stream::MediaDirection::Outgoing;
        std::string path = request["path"].asString();

        // Try to open file
        AudioProvider* prov = session->findProviderForActiveAudio();
        if (prov)
        {
            if (actionText == "read")
            {
                if (path.empty())
                {
                    // Turn off playing into the stream
                    prov->readFile(Audio::PWavFileReader(), direction);
                    answer["status"] = Status_Ok;
                }
                else
                {
                    Audio::PWavFileReader reader = std::make_shared<Audio::WavFileReader>();
                    if (!reader->open(StringHelper::makeTstring(path)))
                        answer["status"] = Status_FailedToOpenFile;
                    else
                    {
                        prov->readFile(reader, direction);
                        answer["status"] = Status_Ok;
                    }
                }
            }
            else
                if (actionText == "write")
                {
                    if (path.empty())
                    {
                        // Turn off recording from the stream
                        prov->writeFile(Audio::PWavFileWriter(), direction);
                        answer["status"] = Status_Ok;
                    }
                    else
                    {
                        Audio::PWavFileWriter writer = std::make_shared<Audio::WavFileWriter>();
                        if (!writer->open(StringHelper::makeTstring(path), AUDIO_SAMPLERATE, AUDIO_CHANNELS))
                            answer["status"] = Status_FailedToOpenFile;
                        else
                        {
                            prov->writeFile(writer, direction);
                            answer["status"] = Status_Ok;
                        }
                    }
                }
                else
                    if (actionText == "mirror")
                    {
                        prov->setupMirror(request["enable"].asBool());
                        answer["status"] = Status_Ok;
                    }
                    else
                        answer["status"] = Status_AccountNotFound;
        }
        else
            answer["status"] = Status_NoMediaAction;
    }
}

#if defined(USE_AQUA_LIBRARY)
void AgentImpl::onMedia(const void* data, int length, MT::Stream::MediaDirection direction, void* context, void* userTag)
{
    /*  if (mIncomingAudioDump && direction == MT::Stream::MediaDirection::Incoming)
    mIncomingAudioDump->write(data, length);

  if (mOutgoingAudioDump && direction == MT::Stream::MediaDirection::Outgoing)
    mOutgoingAudioDump->write(data, length);*/

    // User tag points to accumulator object which includes
    // auto* sa = reinterpret_cast<sevana::aqua*>(userTag);

    switch (direction)
    {
    case MT::Stream::MediaDirection::Incoming: mAquaIncoming.appendBuffer(data, length); break;
    case MT::Stream::MediaDirection::Outgoing: mAquaOutgoing.appendBuffer(data, length); break;
    }
}
#endif


// Called on new incoming session; providers shoukld
#define EVENT_WITH_NAME(X) Json::Value v; v["event_name"] = X;

PDataProvider AgentImpl::onProviderNeeded(const std::string& name)
{
    EVENT_WITH_NAME("provider_needed");
    v["provider_name"] = name;
    addEvent(v);

    return PDataProvider(new AudioProvider(*this, *mTerminal));
}

// Called on new session offer
void AgentImpl::onNewSession(PSession s)
{
    EVENT_WITH_NAME("session_incoming");
    v["session_id"] = s->id();
    v["remote_peer"] = s->remoteAddress();
    mSessionMap[s->id()] = s;

    addEvent(v);
}

// Called when session is terminated
void AgentImpl::onSessionTerminated(PSession s, int responsecode, int reason)
{
    /*if (mIncomingAudioDump)
    mIncomingAudioDump->close();
  if (mOutgoingAudioDump)
    mOutgoingAudioDump->close();
  */
    mAudioManager->stop(mUseNativeAudio ? AudioManager::atReceiver : AudioManager::atNull);
    // Gather statistics before
    EVENT_WITH_NAME("session_terminated");
    v["session_id"] = s->id();
    v["error_code"] = responsecode;
    v["reason_code"] = reason;
    addEvent(v);
}

// Called when session is established ok i.e. after all ICE signalling is finished
// Conntype is type of establish event - EV_SIP or EV_ICE
void AgentImpl::onSessionEstablished(PSession s, int conntype, const RtpPair<InternetAddress>& p)
{
    EVENT_WITH_NAME("session_established");
    v["session_id"] = s->id();
    v["conn_type"] = conntype == EV_SIP ? "sip" : "ice";
    if (conntype == EV_ICE)
        v["media_target"] = p.mRtp.toStdString();

    addEvent(v);
}

void AgentImpl::onSessionProvisional(PSession s, int code)
{
    EVENT_WITH_NAME("session_provisional");
    v["session_id"] = s->id();
    v["code"] = code;
    addEvent(v);
}

// Called when user agent started
void AgentImpl::onStart(int errorcode)
{
    EVENT_WITH_NAME("agent_started");
    v["error_code"] = errorcode;
    addEvent(v);
}

// Called when user agent stopped
void AgentImpl::onStop()
{
    EVENT_WITH_NAME("agent_stopped");
    addEvent(v);
}

// Called when account registered
void AgentImpl::onAccountStart(PAccount account)
{
    EVENT_WITH_NAME("account_started");
    v["account_id"] = account->id();
    addEvent(v);
}

// Called when account removed or failed (non zero error code)
void AgentImpl::onAccountStop(PAccount account, int error)
{
    EVENT_WITH_NAME("account_stopped");
    v["account_id"] = account->id();
    v["error_code"] = error;
    addEvent(v);
}

// Called when connectivity checks failed.
void AgentImpl::onConnectivityFailed(PSession s)
{
    EVENT_WITH_NAME("session_connectivity_failed");
    v["session_id"] = s->id();
    addEvent(v);
}

// Called when new candidate is gathered
void AgentImpl::onCandidateGathered(PSession s, const char* address)
{
    EVENT_WITH_NAME("session_candidate_gathered");
    v["session_id"] = s->id();
    v["address"] = address;
    addEvent(v);
}

// Called when network change detected
void AgentImpl::onNetworkChange(PSession s)
{
    EVENT_WITH_NAME("session_network_changed");
    v["session_id"] = s->id();
    addEvent(v);
}

// Called when all candidates are gathered
void AgentImpl::onGathered(PSession s)
{
    EVENT_WITH_NAME("session_candidates_gathered");
    v["session_id"] = s->id();
    addEvent(v);
}

// Called when new connectivity check is finished
void AgentImpl::onCheckFinished(PSession s, const char* description)
{
    EVENT_WITH_NAME("session_conncheck_finished");
    v["session_id"] = s->id();
    v["description"] = description;
    addEvent(v);
}

// Called when log message must be recorded
void AgentImpl::onLog(const char* /*msg*/)
{}

// Called when problem with SIP connection(s) detected
void AgentImpl::onSipConnectionFailed()
{
    EVENT_WITH_NAME("sip_connection_failed");
    addEvent(v);
}

void AgentImpl::addEvent(const Json::Value& v)
{
    std::unique_lock<std::mutex> lock(mEventListMutex);
    mEventList.push_back(v);
    mEventListChangeCondVar.notify_one();
}

#if defined(USE_AQUA_LIBRARY)
void AgentImpl::closeAqua(int sessionId)
{
    auto aquaIter = mAquaMap.find(sessionId);
    if (aquaIter != mAquaMap.end()) {
        aquaIter->second->close();
        mAquaMap.erase(aquaIter);
    }
}
#endif