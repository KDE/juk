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

class SortedStringList::m_Node 
{
public:
    enum Color { Red, Black };
    
    m_Node(const QString &value) : key(value), parent(0), left(0), right(0), color(Black) {}
    ~m_Node() {}
    
    QString key;
    m_Node *parent;
    m_Node *left;
    m_Node *right;
    Color color;
};

SortedStringList::SortedStringList() : m_root(0)
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
    return find(value);
}

SortedStringList::m_Node *SortedStringList::treeMinimum(m_Node *n) const
{
    while(n->left)
	n = n->left;
    return n;
}

SortedStringList::m_Node *SortedStringList::treeSuccessor(m_Node *n) const
{
    if(n->right)
	return treeMinimum(n->right);
    
    m_Node *p = n->parent;
    
    while(p && n == p->right) {
	n = p;
	p = p->parent;
    }
    
    return p;
}

bool SortedStringList::remove(const QString &value)
{
    m_Node *n = find(value);

    if(!n)
	return false;
    
    m_Node *y;
    m_Node *x;

    if(!n->left || !n->right)
	y = n;
    else
	y = treeSuccessor(n);

    if(y->left)
	x = y->left;
    else
	x = y->right;

    if(x)
	x->parent = y->parent;
    
    if(!y->parent)
	m_root = x;
    else {
	if(y == y->parent->left)
	    y->parent->left = x;
	else
	    y->parent->right = x;
    }
    
    if(y != x)
	n->key = y->key;
    
    delete y;

    return true;
}

QStringList SortedStringList::values() const
{
    QStringList l;
    traverse(m_root, l);
    return l;
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

SortedStringList::m_Node *SortedStringList::find(const QString &value) const
{
    m_Node *n = m_root;
    while(n && value != n->key) {
	if(value < n->key)
	    n = n->left;
	else
	    n = n->right;
    }

    return n;
}

bool SortedStringList::BSTInsert(const QString &value)
{
    m_Node *previousNode = 0;
    m_Node *node = m_root;
    
    while(node) {
	previousNode = node;
	if(value < node->key)
	    node = node->left;
	else
	    node = node->right;
    }
    
    if(previousNode && value == previousNode->key)
	return true;

    m_Node *n = new m_Node(value);

    n->parent = previousNode;

    if(!m_root)
	m_root = n;
    else {
	if(value < previousNode->key)
	    previousNode->left = n;
	else
	    previousNode->right = n;
    }
    
    return false;
}

void SortedStringList::traverse(const m_Node *n, QStringList &list) const
{
    if(!n)
	return;

    traverse(n->left, list);
    list.append(n->key);
    traverse(n->right, list);
}
