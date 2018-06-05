#ifndef SRTP_CONFIG_H
#define SRTP_CONFIG_H

#define HAVE_STDLIB_H

#ifdef WIN32
# define inline __inline
#endif

#ifdef WIN32
#define HAVE_WINSOCK2_H   1
#define CPU_CISC 1
#else
#endif

#endif
