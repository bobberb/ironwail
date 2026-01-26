/*
 * host_string.h --
 * Internationalized string resource for Hexen II
 */

#ifndef __HOST_STRING_H
#define __HOST_STRING_H

extern int host_string_count;

void Host_LoadStrings (void);
const char *Host_GetString (int idx);

#endif /* __HOST_STRING_H */
