/***************************************************************************
                          tag.cpp  -  description
                             -------------------
    begin                : Sun Feb 17 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qdir.h>

#include <id3/misc_support.h>

#include "tag.h"

#include "genre.h"
#include "genrelist.h"
#include "genrelistlist.h"

#define REPLACE true

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Tag::Tag(QString file) 
{
  fileName = file;
  tag.Link(file.latin1());
  
  changed = false;
  
  ///////////////////////////////////////////////////////////
  // the easy ones -- these are supported in the id3 class 
  ///////////////////////////////////////////////////////////

  artistName = ID3_GetArtist(&tag);
  albumName = ID3_GetAlbum(&tag);
  trackName = ID3_GetTitle(&tag);
  trackNumber = ID3_GetTrackNum(&tag);
  trackNumberString = ID3_GetTrack(&tag);
  comment = ID3_GetComment(&tag);
  genreNumber = int(ID3_GetGenreNum(&tag));
  genre = ID3_GetGenre(&tag);
  yearString = ID3_GetYear(&tag);
  
  hasTagBool = (tag.HasV2Tag() || tag.HasV1Tag());
  
  ///////////////////////////////////////////////////////
  // try to guess the track name if there's no id3 tag 
  ///////////////////////////////////////////////////////

  if(trackName.length() <= 0) {
    trackName = fileName;
    while((trackName.right(4)).lower() == ".mp3") {
      trackName = trackName.left(trackName.length()-4);
    }
    trackName = trackName.right(trackName.length() - trackName.findRev(QDir::separator(), -1) -1);
  }
  
  ///////////////////////////////////////////////////////
  // get the genrename from the genre number           
  ///////////////////////////////////////////////////////

  if(genre == "(" + QString::number(genreNumber) + ")" || genre == QString::null) 
    genre = GenreListList::id3v1List().name(genreNumber);
  
  ///////////////////////////////////////////////////////
  // convert the year to a string                      
  ///////////////////////////////////////////////////////

  year = yearString.toInt();
}


Tag::~Tag() 
{
  if(changed) {
    if(artistName.length()>0) {
      ID3_AddArtist(&tag, artistName.latin1(), REPLACE);
    }
    else {
      ID3_RemoveArtists(&tag);
    }
    if(albumName.length()>0) {
      ID3_AddAlbum(&tag, albumName.latin1(), REPLACE);
    }
    else {
      ID3_RemoveAlbums(&tag);
    }
    if(trackName.length()>0) {
      ID3_AddTitle(&tag, trackName.latin1(), REPLACE);
    }
    else {
      ID3_RemoveTitles(&tag);
    }
    if(trackNumber>0) {
      ID3_AddTrack(&tag,  uchar(trackNumber),  uchar(0),  REPLACE);
    }
    else {
      ID3_RemoveTracks(&tag);
    }
    ID3_AddGenre(&tag, 1, REPLACE);
    if(genreNumber>=0 && genreNumber<255) {
      ID3_AddGenre(&tag, genreNumber, REPLACE);
    }
    else {
      ID3_RemoveGenres(&tag);
    }
    if(year>0) {
      ID3_AddYear(&tag, yearString.latin1(), REPLACE);
    }
    else {
      ID3_RemoveYears(&tag);
    }
    
    ID3_RemoveComments(&tag);
    if(comment.length()>0) {
      ID3_AddComment(&tag, comment.latin1(), REPLACE);
    }

    tag.Update();    
  }
}


bool Tag::exists() 
{
  QFile id3_file(fileName);
  return(id3_file.exists()); 
}


////////////////////////////////////////////////
// functions to gather information
////////////////////////////////////////////////

QString Tag::getFileName() { return fileName; }
QString Tag::getTrack() { return trackName; }               // The song's name, not it's track number
QString Tag::getArtist() { return artistName; }
QString Tag::getAlbum() { return albumName; }
QString Tag::getGenre() { return genre; }
int Tag::getGenreNumber() { return genreNumber; }
int Tag::getTrackNumber() { return trackNumber; }
QString Tag::getTrackNumberString() { return trackNumberString; }
int Tag::getYear() { return year; }
QString Tag::getYearString() { return yearString; }
QString Tag::getComment() { return comment; }
bool Tag::hasTag() { return hasTagBool; }

/////////////////////////////////////////////////////
// functions that set information
/////////////////////////////////////////////////////


void Tag::setTrack(QString value) 
{
  changed = true;
  trackName = value;
};
void Tag::setArtist(QString value) 
{
  changed = true;
  artistName = value;
};              
void Tag::setAlbum(QString value) 
{
  changed = true;
  albumName = value;
};               
void Tag::setGenre(int value) 
{
  changed = true;
  if(value >=  0 && value <=  255) {
    genreNumber = value;
  }
  else {
    genreNumber = 255;
  }
}; 
void Tag::setTrackNumber(int value) 
{
  changed = true;
  trackNumber = value;
  trackNumberString.setNum(value);
};
void Tag::setYear(int value) 
{
  changed = true;
  year = value;
  yearString.setNum(value);
};
void Tag::setComment(QString value) 
{
  changed = true;
  comment = value;
};

