/***************************************************************************
    begin                : Sun Feb 2 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler
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

#include <qptrvector.h>

#include "filehandle.h"

/**
 * A simple hash representing an (un-mapped) set of data.
 */

template <class T> class Hash
{
    friend class Iterator;
public:

    Hash() : m_table(m_tableSize) {}
    ~Hash();

    /**
     * To combine two operations into one (that takes the same amount as each
     * independantly) this inserts an item and returns true if the item was
     * already in the set or false if it did not.
     */
    bool insert(T value);

    /**
     * Returns true if the set contains the item \a value.
     */
    bool contains(T value) const;

    /**
     * Removes an item.  Returns true if the item was present and false if not.
     */
    bool remove(T value);

    QValueList<T> values() const;

    int hash(T key) const;

    static inline int tableSize() { return m_tableSize; }

protected:

    struct Node
    {
        Node(T value) : key(value), next(0) {}
        T key;
        Node *next;
    };

public:

    class Iterator
    {
    friend class Hash<T>;
    public:
        Iterator(const Hash<T> &hash) : m_hash(hash), m_index(0), m_node(hash.m_table[0]) {}
        const T &operator*() const { return m_node->key; }
        T &operator*() { return m_node->key; }

        bool operator==(const Iterator &it) const { return m_index == it.m_index && m_node == it.m_node; }
        bool operator!=(const Iterator &it) const { return !(it == *this); }

        T &operator++();

    private:
        const Hash<T> &m_hash;
        int m_index;
        Node *m_node;
    };

    Iterator begin() const
    {
        Iterator it(*this);
        while(!it.m_node && it.m_index < m_tableSize - 1) {
            it.m_index++;
            it.m_node = m_table[it.m_index];
        }

        return it;
    }

    Iterator end() const
    {
        Iterator it(*this);
        it.m_node = 0;
        it.m_index = m_tableSize - 1;
        return it;
    }

protected:

    void deleteNode(Node *n);
    
    QPtrVector<Node> m_table;
    static const int m_tableSize = 5003;
};

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

inline char hashStringAccess(const QString &in, int index)
{
    return in.unicode()[index].cell();
}

inline char hashStringAccess(const QCString &in, int index)
{
    return in[index];
}

// Based on QGDict's hash functions, Copyright (C) 1992-2000 Trolltech AS

template <typename StringType>
inline int hashString(const StringType &s)
{
    uint h = 0;
    uint g;
    for(uint i = 0; i < s.length(); i++)
    {
        h = (h << 4) + hashStringAccess(s, i);
        if((g = h & 0xf0000000))
            h ^= g >> 24;
        h &= ~g;
    }
    int index = h;
    if(index < 0)
        index = -index;
    return index;        
}

////////////////////////////////////////////////////////////////////////////////
// specializations
////////////////////////////////////////////////////////////////////////////////

// StringHash

template<> inline int Hash<QString>::hash(QString key) const
{
    return hashString(key) % tableSize();
}
typedef Hash<QString> StringHash;

// PtrHash

template<> inline int Hash<void *>::hash(void *key) const
{
    return long(key) % tableSize();
}
typedef Hash<void *> PtrHash;

// FileHandleHash

template<> inline int Hash<FileHandle>::hash(FileHandle key) const
{
    return hashString(key.absFilePath()) % tableSize();
}

class FileHandleHash : public Hash<FileHandle>
{
public:
    FileHandleHash() : Hash<FileHandle>() {}

    FileHandle value(const QString &key) const
    {
        int h = hashString(key) % tableSize();
        Node *i = m_table[h];

        while(i && i->key.absFilePath() != key)
            i = i->next;

        return i ? i->key : FileHandle::null();
    }
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
QValueList<T> Hash<T>::values() const
{
    QValueList<T> l;

    Node *n;

    for(int i = 0; i < tableSize(); i++) {
        n = m_table[i];
        while(n) {
            l.append(n->key);
            n = n->next;
        }
    }

    return l;
}

template <class T>
void Hash<T>::deleteNode(Node *n)
{
    if(n) {
        deleteNode(n->next);
        delete n;
    }
}

template <class T>
T &Hash<T>::Iterator::operator++()
{
    if(m_node)
        m_node = m_node->next;

    while(!m_node && m_index < m_tableSize - 1) {
        m_index++;
        m_node = m_hash.m_table[m_index];
    }

    return m_node->key;
}


#endif
