/***************************************************************************
                          cache.h  -  description
                             -------------------
    begin                : Sat Sep 7 2002
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

#ifndef CACHE_H
#define CACHE_H

#include <qstring.h>
#include <qdict.h>

#include "tag.h"

class Cache : public QDict<Tag>
{
public:
    static Cache *instance();
    void save();

protected:
    Cache();
    virtual ~Cache();
    void load();

private:
    static Cache *cache;
};

#endif
