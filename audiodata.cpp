/***************************************************************************
 AudioData.cpp is a modified version of fileinfo.cpp which is a part of
 Mp3Kult (C) 2001 by Stefano Brustia (hio@lombardiacom.it) which is
 available at:

 http://mp3kult.sourceforge.net/

 fileinfo.cpp was modified to just include header information instead of
 header and id3 information.  These changes were made by Scott Wheeler
 (scott@slackorama.net) on 1/13/2002 based on Mp3Kult v0.5.
***************************************************************************/

/***************************************************************************
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "audiodata.h"

#include <iostream.h>
#include <ctype.h>

unsigned int bitrates[3][3][15] =
{
    {
        {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
    },
    {
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
    },
    {
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
    }};

unsigned int s_freq[3][4] =
{
    {44100, 48000, 32000, 0},
    {22050, 24000, 16000, 0},
    {11025, 8000, 8000, 0}
};

static const char *mode_names[5] =
{"Stereo", "Joint Stereo", "Dual Ch", "Mono", "Multi Ch"};
const char *layer_names[3] =
{"I", "II", "III"};
const char *version_names[3] =
{"MPEG-1", "MPEG-2 LSF", "MPEG-2.5"};
const char *version_nums[3] =
{"1", "2", "2.5"};


AudioData::AudioData(const char* filein)
{
    success = false;
    fileglob = strdup(filein);

    lengthchar = 0;

    FILE* file;

    if ((file = fopen(filein, "r")))
    {
        success=readLayerInfo(file);
        fclose(file);
    }
    else   // File isn't readable
    {
        success = false;
    }
}

AudioData::~AudioData()
{
    free(fileglob);
    if(lengthchar)
        free(lengthchar);
}

bool AudioData::readLayerInfo (FILE* file)
{
    unsigned char buf[4];
    unsigned long head;
    struct stat fi;

    fstat(fileno(file), &fi);
    filelen = fi.st_size;

    fseek(file, 0, SEEK_SET);
    if (fread(buf, 1, 4, file) != 4)
        return false;

    head = ((unsigned long) buf[0] << 24) | ((unsigned long) buf[1] << 16) |
           ((unsigned long) buf[2] << 8) | ((unsigned long) buf[3]);

    while (!headCheck(head))
    {
        head <<= 8;
        if (fread(buf, 1, 1, file) != 1)
        {
            return false;
        }
        head |= buf[0];
    }

    switch (((head >> 19) & 0x3) )
    {
    case 3:
        version = 0;
        break;
    case 2:
        version = 1;
        break;
    case 0:
        version = 2;
        break;
    default:
        return false;
    }

    layer = 4 - ((head >> 17) & 3);
    error_protection = ((head >> 16) & 0x1) ^ 0x1;
    bitrate_index = ((head >> 12) & 0xf);
    sampling_frequency = ((head >> 10) & 0x3);
    padding = ((head >> 9) & 0x1);
    extension = ((head >> 8) & 0x1);
    mode = ((head >> 6) & 0x3);
    mode_ext = ((head >> 4) & 0x3);
    copyright = ((head >> 3) & 0x1);
    original = ((head >> 2) & 0x1);
    emphasis = head & 0x3;

    if (!bitrate_index)
        return false;
    length = filelen / ((getBitrate()) * 125);
    return true;
}

bool AudioData::headCheck(unsigned long head)
{
    if ((head & 0xffe00000) != 0xffe00000)
        return false;
    if (!((head >> 17) & 3))
        return false;
    if (((head >> 12) & 0xf) == 0xf)
        return false;
    if (!((head >> 12) & 0xf))
        return false;
    if (((head >> 10) & 0x3) == 0x3)
        return false;
    if (((head >> 19) & 1) == 1 && ((head >> 17) & 3) == 3 &&
        ((head >> 16) & 1) == 1)
        return false;
    if ((head & 0xffff0000) == 0xfffe0000)
        return false;

    return true;
}

int AudioData::getBitrate(){
    return (bitrates[version][layer - 1][bitrate_index]);
}

int AudioData::getSamplerate(){
    return (s_freq[version][sampling_frequency]);
}

const char* AudioData::getMpegver(){
    return (version_names[version]);
}

int AudioData::getLayer() {
    return layer;
}

const char* AudioData::getMode(int mode)
{
    if ((mode >= 0) && (mode < 5))
        return (mode_names[mode]);

    return "Stereo";
}

int AudioData::getMode()
{
    return mode;
}

int AudioData::getLength() {
    return length;
}

char* AudioData::getLengthChar() {
    if(success) {
        int min, sec;
        char buf[6];

        min=length/60;
        sec=length%60;
        sprintf (buf, "%d:%02d", min, sec);
        lengthchar=strdup (buf);
        return lengthchar;
    }
    else
        return 0;
}

int AudioData::getSize() {
    return filelen;
}

bool AudioData::getResult()
{
    return success;
}
