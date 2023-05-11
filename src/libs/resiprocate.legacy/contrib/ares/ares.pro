#-------------------------------------------------
#
# Project created by QtCreator 2010-11-29T22:20:23
#
#-------------------------------------------------

QT       -= core gui

TARGET = ares
TEMPLATE = lib
CONFIG += staticlib
INCLUDEPATH += ../ ../../
DEFINES += WINVER=0x0501 USE_IPV6

win32 {
    DESTDIR = ../../../../Libs/compiled/win
}

SOURCES += \
    ares_timeout.c \
    ares_strerror.c \
    ares_send.c \
    ares_search.c \
    ares__read_line.c \
    ares_query.c \
    ares_process.c \
    ares_parse_ptr_reply.c \
    ares_parse_a_reply.c \
    ares_mkquery.c \
    ares_local.c \
    ares_init.c \
    ares__get_hostent.c \
    ares_gethostbyname.c \
    ares_gethostbyaddr.c \
    ares_free_string.c \
    ares_free_hostent.c \
    ares_free_errmem.c \
    ares_fds.c \
    ares_expand_name.c \
    ares_destroy.c \
    ares__close_sockets.c

HEADERS += \
    ares_version.h \
    ares_socketfunc.h \
    ares_private.h \
    ares_local.h \
    ares_dns.h \
    ares_compat.h \
    ares.h
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/local/lib
    }
    INSTALLS += target
}
