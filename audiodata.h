/***************************************************************************
 AudioData.h is a modified version of fileinfo.h which is a part of
 Mp3Kult (C) 2001 by Stefano Brustia (hio@lombardiacom.it) which is
 available at:

 http://mp3kult.sourceforge.net/

 fileinfo.h was modified to just include header information instead of
 header and id3 information.  These changes were made by Scott Wheeler
 (wheeler@kde.org) on 1/13/2002 based on Mp3Kult v0.5.

***************************************************************************/

/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#ifndef AUDIODATA_H
#define AUDIODATA_H

#include <qstring.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

// I've started hacking a little on this file; both the API and the 
// implementation.  There's still a lot of ugly, non-Qt-happy stuff
// in here, but that will be fixed, well, as I feel motivated.
// (And fortunately I can blame someone else for this file. ;-) )

class AudioData
{
public:
    AudioData(const char* filein);
    virtual ~AudioData();

    int getBitrate() const;
    int getSamplerate() const;
    QString getMpegver() const;
    int getLayer() const;
    static QString getMode(int mode);
    int getMode() const;
    int getLength() const;
    QString getLengthChar() const;
    int getSize() const;

    bool getResult() const;

private:
    bool success;
    char* fileglob;
    int length;
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
