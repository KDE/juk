/***************************************************************************
                          collectionlist.h  -  description
                             -------------------
    begin                : Fri Sep 13 2002
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

#ifndef COLLECTIONLIST_H
#define COLLECTIONLIST_H

#include "playlist.h"

class CollectionList : public Playlist
{
    Q_OBJECT
public: 
    CollectionList(QWidget *parent = 0, const char *name = 0);
    virtual ~CollectionList();
};

#endif
