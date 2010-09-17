/*
 *  TigerAdditionals.h
 *  VoodooHDA
 *
 *  Created by Andy Vandijck on 12/03/10.
 *  Copyright 2010 AnV Software. All rights reserved.
 *
 */

#ifdef TIGER // needed for compile fix...
int vprintf_vhda(const char *fmt, va_list ap);

#define vprintf vprintf_vhda

enum {
    kIOUCVariableStructureSize = 0xffffffff
};
#endif

