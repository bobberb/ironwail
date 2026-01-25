/*
Copyright (C) 2024 Ironwail developers

Hexen II puzzle and info string loading
*/

#ifndef CL_STRING_HEXEN2_H
#define CL_STRING_HEXEN2_H

/* Puzzle piece strings (puzzles.txt) */
extern int puzzle_string_count;

void CL_LoadPuzzleStrings(void);
const char *CL_FindPuzzleString(const char *shortname);

/* Mission pack objectives (infolist.txt) */
extern int info_string_count;

void CL_LoadInfoStrings(void);
const char *CL_GetInfoString(int idx);

#endif /* CL_STRING_HEXEN2_H */
