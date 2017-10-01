/**
 * Copyright (C) 2004 Scott Wheeler <wheeler@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "playlistinterface.h"

////////////////////////////////////////////////////////////////////////////////
// Watched implementation
////////////////////////////////////////////////////////////////////////////////

void Watched::currentPlayingItemChanged()
{
    foreach(PlaylistObserver *observer, m_observers)
        observer->playingItemHasChanged();
}

void Watched::playlistItemsChanged()
{
    foreach(PlaylistObserver *observer, m_observers)
        observer->playlistItemDataHasChanged();
}

void Watched::addObserver(PlaylistObserver *observer)
{
    m_observers.append(observer);
}

void Watched::removeObserver(PlaylistObserver *observer)
{
    if(observer)
        observer->clearWatched();

    m_observers.removeAll(observer);
}

void Watched::clearObservers()
{
    foreach(PlaylistObserver *observer, m_observers)
        observer->clearWatched();

    m_observers.clear();
}

Watched::~Watched()
{
    clearObservers();
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
