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

////////////////////////////////////////////////////////////////////////////////
// Watched implementation
////////////////////////////////////////////////////////////////////////////////

void Watched::currentChanged()
{
    foreach(PlaylistObserver *observer, m_observers)
        observer->updateCurrent();
}

void Watched::dataChanged()
{
    foreach(PlaylistObserver *observer, m_observers)
        observer->updateData();
}

void Watched::addObserver(PlaylistObserver *observer)
{
    m_observers.append(observer);
}

void Watched::removeObserver(PlaylistObserver *observer)
{
    m_observers.remove(observer);
}

void Watched::clearObservers()
{
    m_observers.clear();
}

Watched::~Watched()
{
    foreach(PlaylistObserver *observer, m_observers)
        observer->clearWatched();
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
    if(m_playlist)
        playlist->addObserver(this);
}

const PlaylistInterface *PlaylistObserver::playlist() const
{
    return m_playlist;
}

// vim: set et sw=4 tw=0 sta:
