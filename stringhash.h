/***************************************************************************
                          stringhash.h  -  description
                             -------------------
    begin                : Sun Feb 2 2003
    copyright            : (C) 2003 by Scott Wheeler
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

#ifndef STRINGHASH_H
#define STRINGHASH_H

#include <qstring.h>
#include <qstringlist.h>
#include <qptrvector.h>

class StringHash 
{
public: 
    StringHash();
    ~StringHash();

    bool insert(const QString &value);
    bool contains(const QString &value) const;
    bool remove(const QString &value);

    /**
     * Returns a sorted list of the values.
     * Warning, this method is expensive and shouldn't be used except when 
     * necessary.
     */
    // QStringList values() const;

private:
    class Node;
    int hash(const QString &key) const;
    void deleteNode(Node *n);
    
    QPtrVector<Node> m_table;
};

#endif
