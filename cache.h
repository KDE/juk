/***************************************************************************
                          cache.h  -  description
                             -------------------
    begin                : Sat Sep 7 2002
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

#ifndef CACHE_H
#define CACHE_H

#include <qstring.h>

// Eventually this should implement the same interface as the Tag class; in fact
// there should be an abstract API for both of them to use.  But, for the
// moment this is just a place holder to fill in the design.

class Cache
{
public:
    class Item;

    static Cache *instance();
    Item *item(const QString &fileName) const;

protected:
    Cache();
    virtual ~Cache();

private:
    static Cache *cache;
    
};

class Cache::Item
{
public:
    Item();
    virtual ~Item();
};

#endif
