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

#ifndef PAINTEATER_H
#define PAINTEATER_H

#include <qobject.h>

class Playlist;

/**
 * This small class will block paint events on a Playlist until it passes out
 * of scope.  This is a bit of a hack to get around the fact that while
 * Playlists are loading items painting takes much more time than loading the
 * PlaylistItems.
 */

class PaintEater : public QObject
{
public:
    PaintEater(Playlist *list);

protected:
    virtual bool eventFilter(QObject *o, QEvent *e);

private:
    Playlist *m_list;
    bool     m_allowOne;
    int      m_previousHeight;
};

#endif
