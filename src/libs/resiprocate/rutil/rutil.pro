#-------------------------------------------------
#
# Project created by QtCreator 2010-11-29T21:30:22
#
#-------------------------------------------------

QT       -= core gui

TARGET = rutil
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += ../ ../contrib/ares ../../contrib/ares
DEFINES += USE_ARES USE_IPV6 _WIN32_WINNT=0x0501

win32 {
    DESTDIR = ../../../Libs/compiled/win
}

SOURCES += \
    FileSystem.cxx \
    DnsUtil.cxx \
    DataStream.cxx \
    Data.cxx \
    CountStream.cxx \
    Condition.cxx \
    Coders.cxx \
    BaseException.cxx \
    AbstractFifo.cxx \
    Log.cxx \
    Lock.cxx \
    HeapInstanceCounter.cxx \
    resipfaststreams.cxx \
    RecursiveMutex.cxx \
    Random.cxx \
    RADIUSDigestAuthenticator.cxx \
    Poll.cxx \
    ParseException.cxx \
    ParseBuffer.cxx \
    Mutex.cxx \
    MD5Stream.cxx \
    vmd5.cxx \
    TransportType.cxx \
    Timer.cxx \
    Time.cxx \
    ThreadIf.cxx \
    SysLogStream.cxx \
    SysLogBuf.cxx \
    Subsystem.cxx \
    Socket.cxx \
    RWMutex.cxx \
    dns/RRVip.cxx \
    dns/RROverlay.cxx \
    dns/RRList.cxx \
    dns/RRCache.cxx \
    dns/QueryTypes.cxx \
    dns/LocalDns.cxx \
    dns/ExternalDnsFactory.cxx \
    dns/DnsStub.cxx \
    dns/DnsSrvRecord.cxx \
    dns/DnsResourceRecord.cxx \
    dns/DnsNaptrRecord.cxx \
    dns/DnsHostRecord.cxx \
    dns/DnsCnameRecord.cxx \
    dns/DnsAAAARecord.cxx \
    dns/AresDns.cxx \
    ssl/SHA1Stream.cxx \
    ssl/OpenSSLInit.cxx \
    stun/Udp.cxx \
    stun/Stun.cxx \
    ssl/SHA1Stream.cxx \
    ssl/OpenSSLInit.cxx

HEADERS += \
    FiniteFifo.hxx \
    FileSystem.hxx \
    Fifo.hxx \
    DnsUtil.hxx \
    DataStream.hxx \
    DataException.hxx \
    Data.hxx \
    CountStream.hxx \
    Condition.hxx \
    compat.hxx \
    Coders.hxx \
    CircularBuffer.hxx \
    BaseException.hxx \
    AsyncProcessHandler.hxx \
    AsyncID.hxx \
    AbstractFifo.hxx \
    Logger.hxx \
    Log.hxx \
    Lockable.hxx \
    Lock.hxx \
    IntrusiveListElement.hxx \
    Inserter.hxx \
    HeapInstanceCounter.hxx \
    HashMap.hxx \
    GenericTimerQueue.hxx \
    GenericIPAddress.hxx \
    resipfaststreams.hxx \
    RecursiveMutex.hxx \
    Random.hxx \
    RADIUSDigestAuthenticator.hxx \
    Poll.hxx \
    ParseException.hxx \
    ParseBuffer.hxx \
    Mutex.hxx \
    MD5Stream.hxx \
    vthread.hxx \
    vmd5.hxx \
    TransportType.hxx \
    Timer.hxx \
    TimeLimitFifo.hxx \
    Time.hxx \
    ThreadIf.hxx \
    SysLogStream.hxx \
    SysLogBuf.hxx \
    Subsystem.hxx \
    Socket.hxx \
    SharedPtr.hxx \
    SharedCount.hxx \
    RWMutex.hxx \
    dns/RRVip.hxx \
    dns/RROverlay.hxx \
    dns/RRList.hxx \
    dns/RRFactory.hxx \
    dns/RRCache.hxx \
    dns/QueryTypes.hxx \
    dns/LocalDns.hxx \
    dns/ExternalDnsFactory.hxx \
    dns/ExternalDns.hxx \
    dns/DnsStub.hxx \
    dns/DnsSrvRecord.hxx \
    dns/DnsResourceRecord.hxx \
    dns/DnsNaptrRecord.hxx \
    dns/DnsHostRecord.hxx \
    dns/DnsHandler.hxx \
    dns/DnsCnameRecord.hxx \
    dns/DnsAAAARecord.hxx \
    dns/AresDns.hxx \
    dns/AresCompat.hxx \
    ssl/SHA1Stream.hxx \
    ssl/OpenSSLInit.hxx \
    stun/Udp.hxx \
    stun/Stun.hxx \
    ssl/SHA1Stream.hxx \
    ssl/OpenSSLInit.hxx
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/local/lib
    }
    INSTALLS += target
}


