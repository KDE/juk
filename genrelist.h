/***************************************************************************
                          genrelist.h  -  description
                             -------------------
    begin                : Sun Mar 3 2002
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

#ifndef GENRELIST_H
#define GENRELIST_H

#include <qvaluelist.h>
#include <qvaluevector.h>

#include "genre.h"

class GenreList : public QValueList<Genre>
{
public:
    GenreList(bool createIndex = false);
    GenreList(const QString &file, bool createIndex = false);
    virtual ~GenreList();

    void load(const QString &file);

    /**
     * Given an ID3v1 "number", look up the name.
     */
    QString ID3v1Name(int ID3v1);

    /**
     * Do a "reverse" lookup.  Given an ID3v1 genre name, find the m_index.  Though
     * I didn't realize it at the time that I wrote it, this is a 
     * reimplimentation from QValueList; ours however caches the last search so
     * it should speed things up a bit for "two in a row" searches that are 
     * common.
     */
    int findIndex(const QString &item);

    QString name() const;
    void setName(const QString &n);
    bool readOnly() const { return m_readOnlyList; }
    void setReadOnly(bool ro) { m_readOnlyList = ro; }

private:
    /** 
     * This is used for creating a mapping between ID3v1 genre numbers and the
     * name that is associated with those genres.  There is no reason that this
     * should be called for GenreLists other than the ID3v1 GenreList. 
     */
    void initializeIndex();

    QValueVector<QString> m_index;
    bool m_hasIndex;
    QString m_listName;
    bool m_readOnlyList;
};

#endif
