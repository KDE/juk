/***************************************************************************
                          genrelist.h  -  description
                             -------------------
    begin                : Sun Mar 3 2002
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

#ifndef GENRELIST_H
#define GENRELIST_H

#include <qvaluelist.h>
#include <qstring.h>
#include <qvaluevector.h>

#include "genre.h"

class GenreList : public QValueList<Genre>  
{
public:
  GenreList(bool createIndex = false);
  GenreList(QString file, bool createIndex = false);
  ~GenreList();

  void load(QString file);
  QString name(int id3v1);
  int findIndex(QString item);

private:
  QValueVector<QString> index;
  bool hasIndex;
  void initializeIndex();
};

#endif
