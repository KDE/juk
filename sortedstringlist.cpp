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

class SortedStringList::Node 
{
public:
    enum Color { Red, Black };
    
    Node(const QString &value) : key(value), parent(0), left(0), right(0), color(Black) {}
    ~Node();
    
    QString key;
    Node *parent;
    Node *left;
    Node *right;
    Color color;
};

SortedStringList::SortedStringList() : root(0)
{
    
}

SortedStringList::~SortedStringList()
{
    
}

bool SortedStringList::insert(const QString &value)
{
    return BSTInsert(value);
}

bool SortedStringList::contains(const QString &value) const
{
    Node *n = root;
    while(n && value != n->key) {
	if(value < n->key)
	    n = n->left;
	else
	    n = n->right;
    }

    if(n)
	return true;
    else
	return false;
}

QStringList SortedStringList::values() const
{
    QStringList l;
    traverse(root, l);
    return l;
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

bool SortedStringList::BSTInsert(const QString &value)
{
    Node *previousNode = 0;
    Node *node = root;
    
    while(node) {
	previousNode = node;
	if(value < node->key)
	    node = node->left;
	else
	    node = node->right;
    }
    
    if(previousNode && value == previousNode->key)
	return true;

    Node *n = new Node(value);

    n->parent = previousNode;

    if(!root)
	root = n;
    else {
	if(value < previousNode->key) {
	    previousNode->left = n;
//	    kdDebug() << "LEFT - " << value << endl;
	}
	else {
	    previousNode->right = n;
//	    kdDebug() << "RIGHT - " << value << endl;
	}
    }
    
    return false;
}

void SortedStringList::traverse(const Node *n, QStringList &list) const
{
    if(!n)
	return;

    traverse(n->left, list);
    list.append(n->key);
    traverse(n->right, list);
}
