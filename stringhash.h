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

template <class T> class Hash
{
public:

    bool insert(T value);
    bool contains(T value) const;
    bool remove(T value);

protected:
    Hash() : m_table(m_tableSize) {}
    virtual ~Hash();

    /**
     * This should probably be done with partial template specialization, but
     * I'm too lazy at the moment.
     */
    virtual int hash(T key) const = 0;
    static int maxSize() { return m_tableSize; }
private:

    struct Node
    {
        Node(T value) : key(value), next(0) {}
        ~Node() {}
        T key;
        Node *next;
    };

    void deleteNode(Node *n);
    
    QPtrVector<Node> m_table;
    static const int m_tableSize = 5003;
};

class StringHash : public Hash<QString>
{
public:
    StringHash() : Hash<QString>() {}
    virtual int hash(QString) const;
};

class PtrHash : public Hash<void *>
{
public:
    PtrHash() : Hash<void *>() {}
    virtual int hash(void *key) const { return long(key) % maxSize(); }
};

////////////////////////////////////////////////////////////////////////////////
// template method implementations
////////////////////////////////////////////////////////////////////////////////

template <class T>
Hash<T>::~Hash()
{
    for(int i = 0; i < m_tableSize; i++)
        deleteNode(m_table[i]);
}

template <class T>
bool Hash<T>::insert(T value)
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

template <class T>
bool Hash<T>::contains(T value) const
{
    int h = hash(value);
    Node *i = m_table[h];

    while(i && i->key != value)
        i = i->next;

    return bool(i);
}

template <class T>
bool Hash<T>::remove(T value)
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

template <class T>
void Hash<T>::deleteNode(Node *n)
{
    if(n) {
        deleteNode(n->next);
        delete n;
    }
}

#endif
