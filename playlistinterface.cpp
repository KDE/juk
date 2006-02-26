/***************************************************************************
    copyright            : (C) 2004 by Scott Wheeler
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

#include "playlistinterface.h"
//Added by qt3to4:
#include <Q3ValueList>

////////////////////////////////////////////////////////////////////////////////
// Watched implementation
////////////////////////////////////////////////////////////////////////////////

void Watched::currentChanged()
{
    for(Q3ValueList<PlaylistObserver *>::ConstIterator it = m_observers.begin();
        it != m_observers.end();
        ++it)
    {
        (*it)->updateCurrent();
    }
}

void Watched::dataChanged()
{
    for(Q3ValueList<PlaylistObserver *>::ConstIterator it = m_observers.begin();
        it != m_observers.end();
        ++it)
    {
        (*it)->updateData();
    }
}

void Watched::addObserver(PlaylistObserver *observer)
{
    m_observers.append(observer);
}

void Watched::removeObserver(PlaylistObserver *observer)
{
    m_observers.remove(observer);
}

Watched::~Watched()
{
    for(Q3ValueList<PlaylistObserver *>::Iterator it = m_observers.begin();
        it != m_observers.end();
        ++it)
    {
        (*it)->clearWatched();
    }
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistObserver implementation
////////////////////////////////////////////////////////////////////////////////

PlaylistObserver::~PlaylistObserver()
{
    if(m_playlist)
        m_playlist->removeObserver(this);
}

PlaylistObserver::PlaylistObserver(PlaylistInterface *playlist) :
    m_playlist(playlist)
{
    playlist->addObserver(this);
}

const PlaylistInterface *PlaylistObserver::playlist() const
{
    return m_playlist;
}

// vim: set et sw=4 tw=0 sta:
