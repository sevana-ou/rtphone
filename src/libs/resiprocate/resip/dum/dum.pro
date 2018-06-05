#-------------------------------------------------
#
# Project created by QtCreator 2010-11-29T22:23:29
#
#-------------------------------------------------

QT       -= core gui

TARGET = dum
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += ../../
DEFINES += USE_IPV6 WINVER=0x0501

win32 {
    DESTDIR = ../../../../Libs/compiled/win/
}

SOURCES += \
    UserProfile.cxx \
    UserAuthInfo.cxx \
    TargetCommand.cxx \
    SubscriptionState.cxx \
    SubscriptionHandler.cxx \
    SubscriptionCreator.cxx \
    ServerSubscription.cxx \
    ServerRegistration.cxx \
    ServerPublication.cxx \
    ServerPagerMessage.cxx \
    ServerOutOfDialogReq.cxx \
    ServerInviteSession.cxx \
    ServerAuthManager.cxx \
    RegistrationHandler.cxx \
    RegistrationCreator.cxx \
    RedirectManager.cxx \
    RADIUSServerAuthManager.cxx \
    PublicationCreator.cxx \
    Profile.cxx \
    PagerMessageCreator.cxx \
    OutOfDialogReqCreator.cxx \
    OutgoingEvent.cxx \
    NonDialogUsage.cxx \
    NetworkAssociation.cxx \
    MergedRequestRemovalCommand.cxx \
    MergedRequestKey.cxx \
    MasterProfile.cxx \
    KeepAliveTimeout.cxx \
    KeepAliveManager.cxx \
    InviteSessionHandler.cxx \
    InviteSessionCreator.cxx \
    InviteSession.cxx \
    InMemoryRegistrationDatabase.cxx \
    IdentityHandler.cxx \
    HttpProvider.cxx \
    HttpGetMessage.cxx \
    HandleManager.cxx \
    HandleException.cxx \
    Handled.cxx \
    Handle.cxx \
    EncryptionRequest.cxx \
    DumTimeout.cxx \
    DumThread.cxx \
    DumProcessHandler.cxx \
    DumHelper.cxx \
    DumFeatureMessage.cxx \
    DumFeatureChain.cxx \
    DumFeature.cxx \
    DumDecrypted.cxx \
    DialogUsageManager.cxx \
    DialogUsage.cxx \
    DialogSetId.cxx \
    DialogSet.cxx \
    DialogId.cxx \
    DialogEventStateManager.cxx \
    DialogEventInfo.cxx \
    Dialog.cxx \
    DestroyUsage.cxx \
    DefaultServerReferHandler.cxx \
    ContactInstanceRecord.cxx \
    ClientSubscription.cxx \
    ClientRegistration.cxx \
    ClientPublication.cxx \
    ClientPagerMessage.cxx \
    ClientOutOfDialogReq.cxx \
    ClientInviteSession.cxx \
    ClientAuthManager.cxx \
    ClientAuthExtension.cxx \
    ChallengeInfo.cxx \
    CertMessage.cxx \
    BaseUsage.cxx \
    BaseSubscription.cxx \
    BaseCreator.cxx \
    AppDialogSetFactory.cxx \
    AppDialogSet.cxx \
    AppDialog.cxx

HEADERS += \
    UserProfile.hxx \
    UserAuthInfo.hxx \
    UsageUseException.hxx \
    TargetCommand.hxx \
    SubscriptionState.hxx \
    SubscriptionPersistenceManager.hxx \
    SubscriptionHandler.hxx \
    SubscriptionCreator.hxx \
    ServerSubscriptionFunctor.hxx \
    ServerSubscription.hxx \
    ServerRegistration.hxx \
    ServerPublication.hxx \
    ServerPagerMessage.hxx \
    ServerOutOfDialogReq.hxx \
    ServerInviteSession.hxx \
    ServerAuthManager.hxx \
    RemoteCertStore.hxx \
    RegistrationPersistenceManager.hxx \
    RegistrationHandler.hxx \
    RegistrationCreator.hxx \
    RefCountedDestroyer.hxx \
    RedirectManager.hxx \
    RedirectHandler.hxx \
    RADIUSServerAuthManager.hxx \
    PublicationHandler.hxx \
    PublicationCreator.hxx \
    Profile.hxx \
    Postable.hxx \
    PagerMessageHandler.hxx \
    PagerMessageCreator.hxx \
    OutOfDialogReqCreator.hxx \
    OutOfDialogHandler.hxx \
    OutgoingEvent.hxx \
    NonDialogUsage.hxx \
    NetworkAssociation.hxx \
    MergedRequestRemovalCommand.hxx \
    MergedRequestKey.hxx \
    MasterProfile.hxx \
    KeepAliveTimeout.hxx \
    KeepAliveManager.hxx \
    InviteSessionHandler.hxx \
    InviteSessionCreator.hxx \
    InviteSession.hxx \
    InviteDialogs.hxx \
    InMemoryRegistrationDatabase.hxx \
    IdentityHandler.hxx \
    HttpProvider.hxx \
    HttpGetMessage.hxx \
    Handles.hxx \
    HandleManager.hxx \
    HandleException.hxx \
    Handled.hxx \
    Handle.hxx \
    ExternalTimer.hxx \
    ExternalMessageHandler.hxx \
    ExternalMessageBase.hxx \
    EventDispatcher.hxx \
    EncryptionRequest.hxx \
    DumTimeout.hxx \
    DumThread.hxx \
    DumShutdownHandler.hxx \
    DumProcessHandler.hxx \
    DumHelper.hxx \
    DumFeatureMessage.hxx \
    DumFeatureChain.hxx \
    DumFeature.hxx \
    DumException.hxx \
    DumDecrypted.hxx \
    DumCommand.hxx \
    DialogUsageManager.hxx \
    DialogUsage.hxx \
    DialogSetId.hxx \
    DialogSetHandler.hxx \
    DialogSet.hxx \
    DialogId.hxx \
    DialogEventStateManager.hxx \
    DialogEventInfo.hxx \
    DialogEventHandler.hxx \
    Dialog.hxx \
    DestroyUsage.hxx \
    DefaultServerReferHandler.hxx \
    ContactInstanceRecord.hxx \
    ClientSubscriptionFunctor.hxx \
    ClientSubscription.hxx \
    ClientRegistration.hxx \
    ClientPublication.hxx \
    ClientPagerMessage.hxx \
    ClientOutOfDialogReq.hxx \
    ClientInviteSession.hxx \
    ClientAuthManager.hxx \
    ClientAuthExtension.hxx \
    ChallengeInfo.hxx \
    CertMessage.hxx \
    BaseUsage.hxx \
    BaseSubscription.hxx \
    BaseCreator.hxx \
    AppDialogSetFactory.hxx \
    AppDialogSet.hxx \
    AppDialog.hxx
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/local/lib
    }
    INSTALLS += target
}

OTHER_FILES +=
