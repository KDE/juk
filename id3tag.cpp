/***************************************************************************
                          tag.cpp  -  description
                             -------------------
    begin                : Sun Feb 17 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : wheeler@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kdebug.h>

#include <qdir.h>
#include <qregexp.h>

#include <id3/misc_support.h>

#include "id3tag.h"

#include "genrelist.h"
#include "genrelistlist.h"

#define REPLACE true

// Warning:  Some of this code is really old (~2 years) and may reflect some of 
// my C days and or general bad design.  However, I haven't gone to great length 
// to replace it since it will be replaced when TagLib is ready to drop in.
// Basically, if you are looking for an example of my programming style -- look
// into the other classes.  This is a leftover from QTagger 0.1.  ;-)

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

ID3Tag::ID3Tag(const QString &file) : Tag(file), fileName(file), changed(false)
{
    metaInfo = KFileMetaInfo(file, QString::null, KFileMetaInfo::Fastest);

    tag.Link(fileName.latin1());

    // the easy ones -- these are supported in the id3 class

    char *temp;

    temp = ID3_GetArtist(&tag);
    tagArtist = temp;
    delete [] temp;

    temp = ID3_GetAlbum(&tag);
    tagAlbum = temp;
    delete [] temp;

    temp = ID3_GetTitle(&tag);
    tagTrack = temp;
    delete [] temp;

    temp = ID3_GetTrack(&tag);
    tagTrackNumberString = temp;
    delete [] temp;

    tagTrackNumberString.replace(QRegExp("^0+"), QString::null);

    tagTrackNumber = ID3_GetTrackNum(&tag);

    temp = ID3_GetComment(&tag);
    tagComment = temp;
    delete [] temp;

    temp = ID3_GetGenre(&tag);
    tagGenre = temp;
    delete [] temp;

    tagGenre.setID3v1(int(ID3_GetGenreNum(&tag)));

    temp = ID3_GetYear(&tag);
    tagYearString = temp;
    delete [] temp;

    tagExists = (tag.HasV2Tag() || tag.HasV1Tag());

    if(tagTrack.length() <= 0) {
        tagTrack = fileName;

        while((tagTrack.right(4)).lower() == ".mp3")
            tagTrack = tagTrack.left(tagTrack.length() - 4);

        tagTrack = tagTrack.right(tagTrack.length() - tagTrack.findRev(QDir::separator(), -1) -1);
    }

    // parse the genre string for (<ID3v1 number>)

    if(tagGenre == "(" + QString::number(tagGenre.getID3v1()) + ")" || tagGenre == QString::null)
        tagGenre = GenreListList::ID3v1List().ID3v1Name(tagGenre.getID3v1());
    else if(tagGenre.find(QRegExp("\\([0-9]+\\)")) == 0)
        tagGenre = tagGenre.mid(tagGenre.find(")") + 1);

    // convert the year

    tagYear = tagYearString.toInt();

    if(tagYear <= 0)
        tagYearString = QString::null;
}


ID3Tag::~ID3Tag()
{

}


void ID3Tag::save()
{
    if(changed) {
        if(tagArtist.length() > 0)
            ID3_AddArtist(&tag, tagArtist.latin1(), REPLACE);
        else
            ID3_RemoveArtists(&tag);

        if(tagAlbum.length() > 0)
            ID3_AddAlbum(&tag, tagAlbum.latin1(), REPLACE);
        else
            ID3_RemoveAlbums(&tag);

        if(tagTrack.length() > 0)
            ID3_AddTitle(&tag, tagTrack.latin1(), REPLACE);
        else
            ID3_RemoveTitles(&tag);

        if(tagTrackNumber > 0)
            ID3_AddTrack(&tag,  uchar(tagTrackNumber),  uchar(0),  REPLACE);
        else
            ID3_RemoveTracks(&tag);

        if(tagGenre.getID3v1() >= 0 && tagGenre.getID3v1() <  int(GenreListList::ID3v1List().count())) {
            QString genreString;

            if(tagGenre != GenreListList::ID3v1List().ID3v1Name(tagGenre.getID3v1()))
                genreString = "(" + QString::number(tagGenre.getID3v1()) + ")" + tagGenre;
            else
                genreString = "(" + QString::number(tagGenre.getID3v1()) + ")";

            ID3_AddGenre(&tag, genreString.latin1(), REPLACE);
        }
        else
            ID3_RemoveGenres(&tag);

        if(tagYear > 0)
            ID3_AddYear(&tag, tagYearString.latin1(), REPLACE);
        else
            ID3_RemoveYears(&tag);

        ID3_RemoveComments(&tag);
        if(tagComment.length()>0)
            ID3_AddComment(&tag, tagComment.latin1(), REPLACE);

        tag.Update();
        changed = false;
    }
}

bool ID3Tag::hasTag() const
{ 
    return tagExists;
}

QString ID3Tag::track() const  // The song's name, not it's track number
{ 
    return tagTrack; 
}              

QString ID3Tag::artist() const 
{ 
    return tagArtist;
}

QString ID3Tag::album() const
{ 
    return tagAlbum;
}

Genre ID3Tag::genre() const 
{ 
    return tagGenre;
}

int ID3Tag::trackNumber() const 
{ 
    return tagTrackNumber;
}

QString ID3Tag::trackNumberString() const
{ 
    return tagTrackNumberString;
}

int ID3Tag::year() const
{ 
    return tagYear;
}

QString ID3Tag::yearString() const
{ 
    return tagYearString;
}

QString ID3Tag::comment() const
{ 
    return tagComment;
}

/////////////////////////////////////////////////////
// functions that set information
/////////////////////////////////////////////////////

void ID3Tag::setTrack(const QString &value)
{
    changed = true;
    tagTrack = value;
};

void ID3Tag::setArtist(const QString &value)
{
    changed = true;
    tagArtist = value;
};

void ID3Tag::setAlbum(const QString &value)
{
    changed = true;
    tagAlbum = value;
};

void ID3Tag::setGenre(const Genre &value)
{
    changed = true;
    tagGenre = value;
};

void ID3Tag::setTrackNumber(int value)
{
    changed = true;
    tagTrackNumber = value;
    if(tagTrackNumber > 0)
        tagTrackNumberString.setNum(value);
    else
        tagTrackNumberString = QString::null;
};

void ID3Tag::setYear(int value)
{
    changed = true;
    tagYear = value;
    if(tagYear > 0)
        tagYearString.setNum(value);
    else
        tagYearString = QString::null;
};

void ID3Tag::setComment(const QString &value)
{
    changed = true;
    tagComment = value;
};

QString ID3Tag::bitrateString() const
{
    return readBitrate(metaInfo);
}

QString ID3Tag::lengthString() const
{
    return readLength(metaInfo);
}

int ID3Tag::seconds() const
{
    return readSeconds(metaInfo);
}
