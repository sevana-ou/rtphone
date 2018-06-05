#-------------------------------------------------
#
# Project created by QtCreator 2010-11-29T21:54:39
#
#-------------------------------------------------

QT       -= core gui

TARGET = resiprocate
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += ../../ ../../contrib/ares ../../../contrib/ares
DEFINES += USE_ARES USE_IPV6 _WIN32_WINNT=0x0501

win32 {
    DESTDIR = ../../../../Libs/compiled/win
}

SOURCES += \
    CallId.cxx \
    BranchParameter.cxx \
    BasicNonceHelper.cxx \
    Auth.cxx \
    ApplicationSip.cxx \
    ApiCheck.cxx \
    Aor.cxx \
    Compression.cxx \
    DateCategory.cxx \
    DataParameter.cxx \
    CSeqCategory.cxx \
    CpimContents.cxx \
    ContentsFactoryBase.cxx \
    Contents.cxx \
    ConnectionManager.cxx \
    ConnectionBase.cxx \
    Connection.cxx \
    DnsResult.cxx \
    DnsInterface.cxx \
    DeprecatedDialog.cxx \
    ExternalBodyContents.cxx \
    ExtensionParameter.cxx \
    ExtensionHeader.cxx \
    ExpiresCategory.cxx \
    ExistsParameter.cxx \
    ExistsOrDataParameter.cxx \
    Embedded.cxx \
    DtlsMessage.cxx \
    GenericUri.cxx \
    GenericContents.cxx \
    FloatParameter.cxx \
    LazyParser.cxx \
    KeepAliveMessage.cxx \
    InvalidContents.cxx \
    InterruptableStackThread.cxx \
    InteropHelper.cxx \
    InternalTransport.cxx \
    IntegerParameter.cxx \
    IntegerCategory.cxx \
    Helper.cxx \
    HeaderTypes.cxx \
    Headers.cxx \
    HeaderHash.cxx \
    HeaderFieldValueList.cxx \
    HeaderFieldValue.cxx \
    SecurityAttributes.cxx \
    SdpContents.cxx \
    RportParameter.cxx \
    Rlmi.cxx \
    RequestLine.cxx \
    RAckCategory.cxx \
    QValueParameter.cxx \
    QValue.cxx \
    QuotedDataParameter.cxx \
    PrivacyCategory.cxx \
    PlainContents.cxx \
    Pkcs8Contents.cxx \
    Pkcs7Contents.cxx \
    Pidf.cxx \
    ParserContainerBase.cxx \
    ParserCategory.cxx \
    ParserCategories.cxx \
    ParameterTypes.cxx \
    ParameterHash.cxx \
    Parameter.cxx \
    OctetContents.cxx \
    NonceHelper.cxx \
    NameAddr.cxx \
    MultipartSignedContents.cxx \
    MultipartRelatedContents.cxx \
    MultipartMixedContents.cxx \
    MultipartAlternativeContents.cxx \
    MsgHeaderScanner.cxx \
    Mime.cxx \
    MethodTypes.cxx \
    MethodHash.cxx \
    MessageWaitingContents.cxx \
    MessageFilterRule.cxx \
    Message.cxx \
    XMLCursor.cxx \
    X509Contents.cxx \
    WarningCategory.cxx \
    Via.cxx \
    Uri.cxx \
    UnknownParameter.cxx \
    UInt32Parameter.cxx \
    UInt32Category.cxx \
    UdpTransport.cxx \
    TuSelector.cxx \
    TupleMarkManager.cxx \
    Tuple.cxx \
    TuIM.cxx \
    TransportSelector.cxx \
    TransportFailure.cxx \
    Transport.cxx \
    TransactionUserMessage.cxx \
    TransactionUser.cxx \
    TransactionState.cxx \
    TransactionMap.cxx \
    TransactionController.cxx \
    Token.cxx \
    TimerQueue.cxx \
    TimerMessage.cxx \
    TimeAccumulate.cxx \
    TcpTransport.cxx \
    TcpConnection.cxx \
    TcpBaseTransport.cxx \
    Symbols.cxx \
    StringCategory.cxx \
    StatusLine.cxx \
    StatisticsMessage.cxx \
    StatisticsManager.cxx \
    StatisticsHandler.cxx \
    StatelessHandler.cxx \
    StackThread.cxx \
    SipStack.cxx \
    SipMessage.cxx \
    SipFrag.cxx \
    SERNonceHelper.cxx \
    SelectInterruptor.cxx \
    ssl/TlsTransport.cxx \
    ssl/TlsConnection.cxx \
    ssl/Security.cxx \
    ssl/DtlsTransport.cxx

win32 {
    SOURCES += ssl/WinSecurity.cxx
}

HEADERS += \
    CancelClientInviteTransaction.hxx \
    CancelableTimerQueue.hxx \
    CallId.hxx \
    BranchParameter.hxx \
    BasicNonceHelper.hxx \
    Auth.hxx \
    ApplicationSip.hxx \
    ApplicationMessage.hxx \
    ApiCheckList.hxx \
    ApiCheck.hxx \
    Aor.hxx \
    AbandonServerTransaction.hxx \
    Compression.hxx \
    DateCategory.hxx \
    DataParameter.hxx \
    CSeqCategory.hxx \
    CpimContents.hxx \
    ContentsFactoryBase.hxx \
    ContentsFactory.hxx \
    Contents.hxx \
    ConnectionTerminated.hxx \
    ConnectionManager.hxx \
    ConnectionBase.hxx \
    Connection.hxx \
    DnsResult.hxx \
    DnsInterface.hxx \
    DeprecatedDialog.hxx \
    ExternalBodyContents.hxx \
    ExtensionParameter.hxx \
    ExtensionHeader.hxx \
    ExpiresCategory.hxx \
    ExistsParameter.hxx \
    ExistsOrDataParameter.hxx \
    Embedded.hxx \
    DtlsMessage.hxx \
    GenericUri.hxx \
    GenericContents.hxx \
    FloatParameter.hxx \
    LazyParser.hxx \
    KeepAliveMessage.hxx \
    InvalidContents.hxx \
    InterruptableStackThread.hxx \
    InteropHelper.hxx \
    InternalTransport.hxx \
    IntegerParameter.hxx \
    IntegerCategory.hxx \
    Helper.hxx \
    HeaderTypes.hxx \
    Headers.hxx \
    HeaderHash.hxx \
    HeaderFieldValueList.hxx \
    HeaderFieldValue.hxx \
    SecurityTypes.hxx \
    SecurityAttributes.hxx \
    SdpContents.hxx \
    RportParameter.hxx \
    Rlmi.hxx \
    RequestLine.hxx \
    RAckCategory.hxx \
    QValueParameter.hxx \
    QValue.hxx \
    QuotedDataParameter.hxx \
    PrivacyCategory.hxx \
    PlainContents.hxx \
    Pkcs8Contents.hxx \
    Pkcs7Contents.hxx \
    Pidf.hxx \
    ParserContainerBase.hxx \
    ParserContainer.hxx \
    ParserCategory.hxx \
    ParserCategories.hxx \
    ParameterTypes.hxx \
    ParameterTypeEnums.hxx \
    parametersA.gperf \
    ParameterHash.hxx \
    Parameter.hxx \
    OctetContents.hxx \
    NonceHelper.hxx \
    NameAddr.hxx \
    MultipartSignedContents.hxx \
    MultipartRelatedContents.hxx \
    MultipartMixedContents.hxx \
    MultipartAlternativeContents.hxx \
    MsgHeaderScanner.hxx \
    Mime.hxx \
    MethodTypes.hxx \
    MethodHash.hxx \
    MessageWaitingContents.hxx \
    MessageFilterRule.hxx \
    MessageDecorator.hxx \
    Message.hxx \
    MarkListener.hxx \
    XMLCursor.hxx \
    X509Contents.hxx \
    WarningCategory.hxx \
    Via.hxx \
    ValueFifo.hxx \
    Uri.hxx \
    UnknownParameterType.hxx \
    UnknownParameter.hxx \
    UnknownHeaderType.hxx \
    UInt32Parameter.hxx \
    UInt32Category.hxx \
    UdpTransport.hxx \
    TuSelector.hxx \
    TupleMarkManager.hxx \
    Tuple.hxx \
    TuIM.hxx \
    TransportSelector.hxx \
    TransportFailure.hxx \
    Transport.hxx \
    TransactionUserMessage.hxx \
    TransactionUser.hxx \
    TransactionTerminated.hxx \
    TransactionState.hxx \
    TransactionMessage.hxx \
    TransactionMap.hxx \
    TransactionController.hxx \
    Token.hxx \
    TimerQueue.hxx \
    TimerMessage.hxx \
    TimeAccumulate.hxx \
    TcpTransport.hxx \
    TcpConnection.hxx \
    TcpBaseTransport.hxx \
    Symbols.hxx \
    StringCategory.hxx \
    StatusLine.hxx \
    StatisticsMessage.hxx \
    StatisticsManager.hxx \
    StatisticsHandler.hxx \
    StatelessHandler.hxx \
    StackThread.hxx \
    SipStack.hxx \
    SipMessage.hxx \
    SipFrag.hxx \
    ShutdownMessage.hxx \
    SERNonceHelper.hxx \
    SendData.hxx \
    SelectInterruptor.hxx \
    ssl/TlsTransport.hxx \
    ssl/TlsConnection.hxx \
    ssl/Security.hxx \
    ssl/DtlsTransport.hxx

win32 {
    HEADERS += ssl/WinSecurity.hxx
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/local/lib
    }
    INSTALLS += target
}

OTHER_FILES +=


