/***************************************************************************
                          sortedstringlist.cpp  -  description
                             -------------------
    begin                : Wed Jan 29 2003
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

#include <kdebug.h>

#include "sortedstringlist.h"

static const int tableSize = 5003;

class SortedStringList::Node
{
public:
    Node(const QString &value) : key(value), next(0) {}
    ~Node() {}
    
    QString key;
    Node *next;
};

SortedStringList::SortedStringList() : table(tableSize)
{

}

SortedStringList::~SortedStringList()
{
    for(int i = 0; i < tableSize; i++)
	deleteNode(table[i]);
}

bool SortedStringList::insert(const QString &value)
{
    int h = hash(value);
    Node *i = table[h];
    Node *j = 0;
    
    while(i) {
	if(i->key == value)
	    return true;
	else {
	    j = i;
	    i = i->next;
	}
    }

    if(j)
	j->next = new Node(value);
    else
	table.insert(h, new Node(value));

    return false;
}

bool SortedStringList::contains(const QString &value) const
{
    int h = hash(value);
    Node *i = table[h];

    while(i && i->key != value)
	i = i->next;

    return bool(i);
}

QStringList SortedStringList::values() const
{
    QStringList l;

    Node *n;

    for(int i = 0; i < tableSize; i++) {
	n = table[i];
	while(n) {
	    l.append(n->key);
	    n = n->next;
	}
    }

    return l;
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

int SortedStringList::hash(const QString &key) const
{
    uint h = 0;
    uint g;

    const QChar *p = key.unicode();

    for(int i = 0; i < int(key.length()); i++) {
	h = (h << 4) + p[i].cell();
	if((g = h & 0xf0000000))
	    h ^= g >> 24;
	h &= ~g;
    }

    int index = h;

    if(index < 0)
	index = -index;

    return(index % tableSize);
}

void SortedStringList::deleteNode(Node *n)
{
    if(n) {
	deleteNode(n->next);
	delete(n);
    }
}
