/***************************************************************************
                          stringhash.cpp  -  description
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

#include "stringhash.h"

static const int tableSize = 5003;

class StringHash::Node
{
public:
    Node(const QString &value) : key(value), next(0) {}
    ~Node() {}
    
    QString key;
    Node *next;
};

StringHash::StringHash() : m_table(tableSize)
{

}

StringHash::~StringHash()
{
    for(int i = 0; i < tableSize; i++)
	deleteNode(m_table[i]);
}

bool StringHash::insert(const QString &value)
{
    int h = hash(value);
    Node *i = m_table[h];
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
	m_table.insert(h, new Node(value));

    return false;
}

bool StringHash::contains(const QString &value) const
{
    int h = hash(value);
    Node *i = m_table[h];

    while(i && i->key != value)
	i = i->next;

    return bool(i);
}

bool StringHash::remove(const QString &value)
{
    int h = hash(value);
    Node *previous = 0;
    Node *i = m_table[h];

    while(i && i->key != value) {
	previous = i;
	i = i->next;
    }

    if(!i)
	return false;

    if(previous)
	previous->next = i->next;
    else {
	if(i->next)
	    m_table.insert(h, i->next);
	else
	    m_table.remove(h);
    }

    delete i;

    return true;
}

/*
QStringList StringHash::values() const
{
    QStringList l;

    Node *n;

    for(int i = 0; i < tableSize; i++) {
	n = m_table[i];
	while(n) {
	    l.append(n->key);
	    n = n->next;
	}
    }

    return l;
}
*/

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

int StringHash::hash(const QString &key) const
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

    return index % tableSize;
}

void StringHash::deleteNode(Node *n)
{
    if(n) {
	deleteNode(n->next);
	delete n;
    }
}
