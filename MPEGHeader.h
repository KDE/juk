/***************************************************************************
 MPEGHeader.h is a modified version of fileinfo.h which is a part of 
 Mp3Kult (C) 2001 by Stefano Brustia (hio@lombardiacom.it) which is 
 available at:

 http://mp3kult.sourceforge.net/

 fileinfo.h was modified to just include header information instead of 
 header and id3 information.  These changes were made by Scott Wheeler
 (scott@slackorama.net) on 1/13/2002 based on Mp3Kult v0.5.
 ***************************************************************************/

/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef MPEGHEADER_H
#define MPEGHEADER_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

class MPEGHeader
{

public:
     MPEGHeader(const char* filein);
     ~MPEGHeader();

     int getBitrate();
     int getSamplerate();
     const char* getMpegver();
     int getLayer();
     static const char* getMode(int mode);
     int getMode();
     int getLength();
     char* getLengthChar();
     int getSize();

     bool getResult();

private:

     bool success;
     char* fileglob;
     int length;
     char* lengthchar;
     int version;
     int filelen;
     int layer;
     int error_protection;
     int bitrate_index;
     int sampling_frequency;
     int padding;
     int extension;
     int mode;
     int mode_ext;
     int copyright;
     int original;
     int emphasis;

     bool headCheck(unsigned long head);
     bool readLayerInfo(FILE* file);
};

#endif // MPEGHEADER_H
