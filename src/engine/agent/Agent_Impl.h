#ifndef __AGENT_IMPL_H
#define __AGENT_IMPL_H

#include <string>
#include <memory>
#include <thread>
#include "../endpoint/EP_Engine.h"
#include "../endpoint/EP_AudioProvider.h"
#include "../media/MT_Box.h"
#include "../helper/HL_ByteBuffer.h"

#include "json/json.h"
#include "Agent_AudioManager.h"
#include <mutex>
#include <condition_variable>
#if defined(USE_AQUA_LIBRARY)
# include "aqua++.h"
#endif

#if defined(USE_PVQA_LIBRARY)
# include "pvqa++.h"
#endif

class AgentImpl: public UserAgent
#if defined(USE_AQUA_LIBRARY)
    , public MT::Stream::MediaObserver
#endif
{
protected:
  std::recursive_mutex mAgentMutex;
  std::mutex mEventListMutex;
  std::condition_variable mEventListChangeCondVar;
  std::vector<Json::Value> mEventList;
  bool mUseNativeAudio = false;

  typedef std::map<int, PAccount> AccountMap;
  AccountMap mAccountMap;

  typedef std::map<int, PSession> SessionMap;
  SessionMap mSessionMap;

#if defined(USE_AQUA_LIBRARY)
  // Keys are the same as used in mSessionMap
  typedef std::map<int, std::shared_ptr<sevana::aqua> AquaMap;
  AquaMap mAquaMap;
  ByteBuffer mAquaIncoming, mAquaOutgoing;
#endif

  std::shared_ptr<std::thread> mThread;
  volatile bool mShutdown;
  std::shared_ptr<MT::Terminal> mTerminal;
  std::shared_ptr<AudioManager> mAudioManager;
  //Audio::PWavFileWriter mIncomingAudioDump, mOutgoingAudioDump;

  void run();
  void addEvent(const Json::Value& v);
  void processConfig(Json::Value& request, Json::Value& answer);
  void processStart(Json::Value& request, Json::Value& answer);
  void processStop(Json::Value& request, Json::Value& answer);
  void processCreateAccount(Json::Value& request, Json::Value& answer);
  void processStartAccount(Json::Value& request, Json::Value& answer);
  void processSetUserInfoToAccount(Json::Value& request, Json::Value& answer);
  void processCreateSession(Json::Value& request, Json::Value& answer);
  void processStartSession(Json::Value& request, Json::Value& answer);
  void processStopSession(Json::Value& request, Json::Value& answer);
  void processAcceptSession(Json::Value& request, Json::Value& answer);
  void processDestroySession(Json::Value& request, Json::Value& answer);
  void processWaitForEvent(Json::Value& request, Json::Value& answer);
  void processGetMediaStats(Json::Value& request, Json::Value& answer);
  void processUseStreamForSession(Json::Value& request, Json::Value& answer);
  void processNetworkChanged(Json::Value& request, Json::Value& answer);
  void processAddRootCert(Json::Value& request, Json::Value& answer);
  void processLogMessage(Json::Value& request, Json::Value& answer);
  void stopAgentAndThread();

public:
  AgentImpl();
  ~AgentImpl();

  std::string command(const std::string& command);
  bool waitForData(int milliseconds);
  std::string read();

  // UserAgent overrides
  // Called on new incoming session; providers shoukld
  PDataProvider onProviderNeeded(const std::string& name) override;

  // Called on new session offer
  void onNewSession(PSession s) override;

  // Called when session is terminated
  void onSessionTerminated(PSession s, int responsecode, int reason) override;

  // Called when session is established ok i.e. after all ICE signalling is finished
  // Conntype is type of establish event - EV_SIP or EV_ICE
  void onSessionEstablished(PSession s, int conntype, const RtpPair<InternetAddress>& p) override;

  void onSessionProvisional(PSession s, int code) override;

  // Called when user agent started
  void onStart(int errorcode) override;

  // Called when user agent stopped
  void onStop() override;

  // Called when account registered
  void onAccountStart(PAccount account) override;

  // Called when account removed or failed (non zero error code)
  void onAccountStop(PAccount account, int error) override;

  // Called when connectivity checks failed.
  void onConnectivityFailed(PSession s) override;

  // Called when new candidate is gathered
  void onCandidateGathered(PSession s, const char* address) override;

  // Called when network change detected
  void onNetworkChange(PSession s) override;

  // Called when all candidates are gathered
  void onGathered(PSession s) override;

  // Called when new connectivity check is finished
  void onCheckFinished(PSession s, const char* description) override;

  // Called when log message must be recorded
  void onLog(const char* msg) override;

  // Called when problem with SIP connection(s) detected
  void onSipConnectionFailed() override;

#if defined(USE_AQUA_LIBRARY)
  // Called on incoming & outgoing audio for voice sessions
  void onMedia(const void* data, int length, MT::Stream::MediaDirection direction, void* context, void* userTag) override;
#endif
};

#endif
