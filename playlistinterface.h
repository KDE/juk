/***************************************************************************
                          playlistinterface.h
                             -------------------
    begin                : Fri Feb 27 2004
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

#ifndef PLAYLISTINTERFACE_H
#define PLAYLISTINTERFACE_H

#include "filehandle.h"

#include <qstring.h>

/**
 * This is a simple interface that should be used by things that implement a
 * playlist-like API.
 */

class PlaylistInterface
{
public:
    virtual QString name() const = 0;
    virtual FileHandle currentFile() const = 0;
    virtual int time() const = 0;
    virtual int count() const = 0;

    virtual void playFirst() = 0;
    virtual void playNext() = 0;
    virtual void playPrevious() = 0;
    virtual void stop() = 0;
};

#endif
