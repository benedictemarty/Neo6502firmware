// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      filenames.cpp
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      12th October 2026
//      Purpose :   File names : Latin-1 on the 6502 side, UTF-8 on the storage side (T-130)
//
//		The screen, the keyboard and the fr locale are Latin-1 ; FatFs (FF_LFN_UNICODE 2)
//		and the emulator's host take UTF-8. Names are converted at the FIS layer, both
//		ways. A character outside Latin-1, or a byte which is not valid UTF-8, reads "?".
//		Self-contained (no common.h) : compiled alone by tests/noms (make test-noms).
//
// ***************************************************************************************
// ***************************************************************************************

#include "interface/filenames.h"

// ***************************************************************************************
//
//								Latin-1 -> UTF-8
//
// ***************************************************************************************

std::string FNMToStorage(const std::string& latin1) {
	std::string s;
	s.reserve(latin1.size() * 2);
	for (unsigned char c : latin1) {
		if (c < 0x80) {
			s += (char)c;
		} else {
			s += (char)(0xC0 | (c >> 6));
			s += (char)(0x80 | (c & 0x3F));
		}
	}
	return s;
}

// ***************************************************************************************
//
//								UTF-8 -> Latin-1
//
// ***************************************************************************************

std::string FNMFromStorage(const char *utf8) {
	std::string s;
	const unsigned char *p = (const unsigned char *)utf8;
	while (*p != 0) {
		unsigned char c = *p++;
		if (c < 0x80) { s += (char)c;continue; }
		int follow = (c >= 0xF0 && c < 0xF8) ? 3 : (c >= 0xE0) ? 2 : (c >= 0xC0) ? 1 : -1;
		if (follow < 0 || c >= 0xF8) { s += '?';continue; }							// stray continuation byte
		unsigned int code = c & (0x3F >> follow);
		int n = 0;
		while (n < follow && (*p & 0xC0) == 0x80) { code = (code << 6) | (*p++ & 0x3F);n++; }
		s += (n == follow && code >= 0x80 && code <= 0xFF) ? (char)code : '?';			// truncated, overlong, or not Latin-1
	}
	return s;
}

// ***************************************************************************************
//
//		Date 		Revision
//		==== 		========
//		12-10-26 	First version (T-130, D-03 of NeoWin)
//
// ***************************************************************************************
