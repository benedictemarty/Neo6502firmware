// SPDX-License-Identifier: EUPL-1.2
// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      infopic.h
//      Authors :   bmarty (bmarty@mailo.com)
//      Date :      24th November 2026
//      Purpose :   Group 41, Infocom v6 pictures (T-164) : draws an image of an IBM Infocom
//                  picture file (.EG1 / .MG1 : 16 byte header, directory, LZW pixels) on the
//                  draw page, straight from the file (nothing but an 8 KB table in the heap).
//
// ***************************************************************************************
// ***************************************************************************************

#pragma once

#define IP_ERR_OK        0
#define IP_ERR_PARAM     1                                                      // Bad channel, not a picture file, unsupported mode
#define IP_ERR_IO        2                                                      // Storage error
#define IP_ERR_NOTFOUND  3                                                      // No such picture number
#define IP_ERR_NODATA    4                                                      // Entry without data (a size only)
#define IP_ERR_MEMORY    5                                                      // No heap for the LZW table
#define IP_ERR_DATA      6                                                      // Corrupt or truncated LZW data

#define IP_OPT_HALF      0x01                                                   // Divide the width by 2 (keep even columns)
#define IP_OPT_TRANSP    0x02                                                   // Do not write the transparent colour of the picture
#define IP_OPT_PALETTE   0x04                                                   // Load the picture palette in entries 2 to n+1
#define IP_OPT_EGA       0x08                                                   // Load the 16 standard EGA colours in entries 0 to 15

uint8_t IPInfo(uint8_t channel,uint16_t number,uint16_t *width,uint16_t *height,uint16_t *flags,uint8_t *what);  // 41,1
uint8_t IPDraw(uint8_t channel,uint16_t number,int16_t x,int16_t y,uint8_t options);                           // 41,2
