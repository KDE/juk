/***************************************************************************
                          tag.h  -  description
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

#ifndef TAG_H
#define TAG_H

#include <qstring.h>

#include <id3/tag.h>

class Tag 
{
public: 
  Tag(QString file);
  ~Tag();
  
  bool exists();
  void save();
  
  // functions that gather information
  
  QString getFileName();           
  QString getTrack();              // The song's name, not it's track number
  QString getArtist();             
  QString getAlbum();              
  QString getGenre(); 
  int getGenreNumber();
  int getTrackNumber();
  QString getTrackNumberString();
  int getYear();
  QString getYearString();
  QString getComment();
  bool hasTag();

  // functions that set information

  void setTrack(QString value);               // The song's name, not it's track number
  void setArtist(QString value);              
  void setAlbum(QString value);               
  void setGenre(QString value); 
  void setGenre(int value); 
  void setTrackNumber(int value);
  void setYear(int value);
  void setComment(QString value); 

private:
  ID3_Tag tag;
  QString fileName;
  bool changed;

  // properties read or derived from the tag:
  
  bool hasTagBool;
  QString artistName;
  QString albumName;
  QString trackName;
  int trackNumber;
  QString trackNumberString;
  QString genre;
  int genreNumber;
  int year;
  QString yearString;
  QString comment;
};

#endif
