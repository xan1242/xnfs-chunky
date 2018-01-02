// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>

#define BUILDVER 1
#define PRINTTYPEINFO "INFO:"
#define PRINTTYPEERROR "ERROR:"
#define PRINTTYPEWARNING "WARNING:"

#define CHUNKDEFFILENAME "ChunkDef.txt"
#define UNKNOWNCHUNKSTR "BCHUNK_UNKNOWN"

// stolen from BlackBox :V
#define FORMATTHING1 "\
//      FilePos     Size        FullSize    ChunkID     PrePad      FullPrePad\n\
// =================================================================================================\n"

#define FORMATTHING2 "\
CHUNK:  0x%.08x  0x%.08x  0x%.08x  0x%.08x  0x%.08x  0x%.08x // %s\n"


// TODO: reference additional headers your program requires here
