/***************************************************************************
                          searchplaylist.h
                             -------------------
    begin                : Mon May 5 2003
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

#ifndef SEARCHPLAYLIST_H
#define SEARCHPLAYLIST_H

#include "dynamicplaylist.h"
#include "playlistsearch.h"

class SearchPlaylist : public DynamicPlaylist
{
    Q_OBJECT
public:
    SearchPlaylist(const PlaylistSearch &search, QWidget *parent, const QString &name = QString::null);
    
protected:
    virtual void showEvent(QShowEvent *e);
    virtual PlaylistItemList items();

private slots:
    void slotSetDirty() { m_dirty = true; }

private:
    void search();

    PlaylistSearch m_search;
    bool m_dirty;
};

#endif
