/***************************************************************************
                          id3tag.cpp  -  description
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

ID3Tag::ID3Tag(const QString &fileName) : Tag(fileName), 
					  m_fileName(fileName), m_changed(false)
{
    m_metaInfo = KFileMetaInfo(fileName, QString::null, KFileMetaInfo::Fastest);

    m_tag.Link(m_fileName.latin1());

    // the easy ones -- these are supported in the id3 class

    char *temp;

    temp = ID3_GetArtist(&m_tag);
    m_tagArtist = temp;
    delete [] temp;

    temp = ID3_GetAlbum(&m_tag);
    m_tagAlbum = temp;
    delete [] temp;

    temp = ID3_GetTitle(&m_tag);
    m_tagTrack = temp;
    delete [] temp;

    temp = ID3_GetTrack(&m_tag);
    m_tagTrackNumberString = temp;
    delete [] temp;

    m_tagTrackNumberString.replace(QRegExp("^0+"), QString::null);

    m_tagTrackNumber = ID3_GetTrackNum(&m_tag);

    temp = ID3_GetComment(&m_tag);
    m_tagComment = temp;
    delete [] temp;

    temp = ID3_GetGenre(&m_tag);
    m_tagGenre = temp;
    delete [] temp;

    m_tagGenre.setID3v1(int(ID3_GetGenreNum(&m_tag)));

    temp = ID3_GetYear(&m_tag);
    m_tagYearString = temp;
    delete [] temp;

    m_tagExists = (m_tag.HasV2Tag() || m_tag.HasV1Tag());

    if(m_tagTrack.length() <= 0) {
        m_tagTrack = m_fileName;

        while((m_tagTrack.right(4)).lower() == ".mp3")
            m_tagTrack = m_tagTrack.left(m_tagTrack.length() - 4);

        m_tagTrack = m_tagTrack.right(m_tagTrack.length() - m_tagTrack.findRev(QDir::separator(), -1) -1);
    }

    // parse the genre string for (<ID3v1 number>)

    if(m_tagGenre == "(" + QString::number(m_tagGenre.getID3v1()) + ")" || m_tagGenre.isNull())
        m_tagGenre = GenreListList::ID3v1List().ID3v1Name(m_tagGenre.getID3v1());
    else if(m_tagGenre.find(QRegExp("\\([0-9]+\\)")) == 0)
        m_tagGenre = m_tagGenre.mid(m_tagGenre.find(")") + 1);

    // convert the year

    m_tagYear = m_tagYearString.toInt();

    if(m_tagYear <= 0)
        m_tagYearString = QString::null;
}


ID3Tag::~ID3Tag()
{

}


void ID3Tag::save()
{
    if(m_changed) {
        if(m_tagArtist.length() > 0)
            ID3_AddArtist(&m_tag, m_tagArtist.latin1(), REPLACE);
        else
            ID3_RemoveArtists(&m_tag);

        if(m_tagAlbum.length() > 0)
            ID3_AddAlbum(&m_tag, m_tagAlbum.latin1(), REPLACE);
        else
            ID3_RemoveAlbums(&m_tag);

        if(m_tagTrack.length() > 0)
            ID3_AddTitle(&m_tag, m_tagTrack.latin1(), REPLACE);
        else
            ID3_RemoveTitles(&m_tag);

        if(m_tagTrackNumber > 0)
            ID3_AddTrack(&m_tag,  uchar(m_tagTrackNumber),  uchar(0),  REPLACE);
        else
            ID3_RemoveTracks(&m_tag);

        if(m_tagGenre.getID3v1() >= 0 && m_tagGenre.getID3v1() <  int(GenreListList::ID3v1List().count())) {
            QString genreString;

            if(m_tagGenre != GenreListList::ID3v1List().ID3v1Name(m_tagGenre.getID3v1()))
                genreString = "(" + QString::number(m_tagGenre.getID3v1()) + ")" + m_tagGenre;
            else
                genreString = "(" + QString::number(m_tagGenre.getID3v1()) + ")";

            ID3_AddGenre(&m_tag, genreString.latin1(), REPLACE);
        }
        else
            ID3_RemoveGenres(&m_tag);

        if(m_tagYear > 0)
            ID3_AddYear(&m_tag, m_tagYearString.latin1(), REPLACE);
        else
            ID3_RemoveYears(&m_tag);

        ID3_RemoveComments(&m_tag);
        if(m_tagComment.length()>0)
            ID3_AddComment(&m_tag, m_tagComment.latin1(), REPLACE);

        m_tag.Update();
        m_changed = false;
    }
}

bool ID3Tag::hasTag() const
{ 
    return m_tagExists;
}

QString ID3Tag::track() const  // The song's name, not it's track number
{ 
    return m_tagTrack; 
}              

QString ID3Tag::artist() const 
{ 
    return m_tagArtist;
}

QString ID3Tag::album() const
{ 
    return m_tagAlbum;
}

Genre ID3Tag::genre() const 
{ 
    return m_tagGenre;
}

int ID3Tag::trackNumber() const 
{ 
    return m_tagTrackNumber;
}

QString ID3Tag::trackNumberString() const
{ 
    return m_tagTrackNumberString;
}

int ID3Tag::year() const
{ 
    return m_tagYear;
}

QString ID3Tag::yearString() const
{ 
    return m_tagYearString;
}

QString ID3Tag::comment() const
{ 
    return m_tagComment;
}

/////////////////////////////////////////////////////
// functions that set information
/////////////////////////////////////////////////////

void ID3Tag::setTrack(const QString &value)
{
    m_changed = true;
    m_tagTrack = value;
};

void ID3Tag::setArtist(const QString &value)
{
    m_changed = true;
    m_tagArtist = value;
};

void ID3Tag::setAlbum(const QString &value)
{
    m_changed = true;
    m_tagAlbum = value;
};

void ID3Tag::setGenre(const Genre &value)
{
    m_changed = true;
    m_tagGenre = value;
};

void ID3Tag::setTrackNumber(int value)
{
    m_changed = true;
    m_tagTrackNumber = value;
    if(m_tagTrackNumber > 0)
        m_tagTrackNumberString.setNum(value);
    else
        m_tagTrackNumberString = QString::null;
};

void ID3Tag::setYear(int value)
{
    m_changed = true;
    m_tagYear = value;
    if(m_tagYear > 0)
        m_tagYearString.setNum(value);
    else
        m_tagYearString = QString::null;
};

void ID3Tag::setComment(const QString &value)
{
    m_changed = true;
    m_tagComment = value;
};

QString ID3Tag::bitrateString() const
{
    return readBitrate(m_metaInfo);
}

QString ID3Tag::lengthString() const
{
    return readLength(m_metaInfo);
}

int ID3Tag::seconds() const
{
    return readSeconds(m_metaInfo);
}
