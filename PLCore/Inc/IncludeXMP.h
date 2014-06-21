#ifndef INCLUDEXMP_H
#define INCLUDEXMP_H

#ifndef DVACORE_CONFIG_UNICODETYPES_H
#include "dvacore/config/UnicodeTypes.h"
#endif

#define TXMP_STRING_TYPE	dvacore::StdStr

#if ASL_TARGET_OS_MAC
#define MAC_ENV 1
#endif
#if ASL_TARGET_OS_WIN
#define WIN_ENV 1
#endif

#ifndef XMP_INCLUDE_XMPFILES
#define XMP_INCLUDE_XMPFILES 1
#endif
#include "XMP.hpp"

#endif