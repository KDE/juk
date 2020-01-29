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

#ifndef PLAYLISTINTERFACE_H
#define PLAYLISTINTERFACE_H

#include <QVector>
#include <QObject>

class FileHandle;

class PlaylistInterfaceSignaller: public QObject{
    Q_OBJECT
signals:
    void playingItemChanged();
    void playingItemDataChanged();
};

class PlaylistInterface
{
public:
    virtual QString name() const = 0;
    virtual FileHandle currentFile() const = 0;
    virtual int time() const = 0;
    virtual int count() const = 0;

    virtual void playNext() = 0;
    virtual void playPrevious() = 0;
    virtual void stop() = 0;

    virtual bool playing() const = 0;
    void currentPlayingItemChanged();
    virtual void playlistItemsChanged();

    PlaylistInterfaceSignaller signaller;

    virtual ~PlaylistInterface();
};

#endif

// vim: set et sw=4 tw=0 sta:
