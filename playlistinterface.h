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


/**
 * This is a simple interface that should be used by things that implement a
 * playlist-like API.
 */

class PlaylistInterface
{
public:
    virtual QString nextFile() = 0;
    virtual QString currentFile() = 0;
    virtual QString previousFile() = 0;
};

#endif
