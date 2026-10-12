// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      filenames.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      12th October 2026
//      Purpose :   File names : Latin-1 on the 6502 side, UTF-8 on the storage side (T-130)
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _FILENAMES_H
#define _FILENAMES_H

#include <string>

std::string FNMToStorage(const std::string& latin1);							// Latin-1 -> UTF-8
std::string FNMFromStorage(const char *utf8);									// UTF-8 -> Latin-1 ('?' outside it)

#endif
