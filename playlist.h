/***************************************************************************
                          playlist.h  -  description
                             -------------------
    begin                : Sat Feb 16 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <klistview.h>

#include <qstringlist.h>

#include "playlistitem.h"

class Playlist : public KListView
{
    Q_OBJECT
public:
    Playlist(QWidget *parent = 0, const char *name = 0);
    virtual ~Playlist();

    virtual void append(const QString &item);
    virtual void append(const QStringList &items);
    virtual void append(PlaylistItem *item);
    virtual void append(QPtrList<PlaylistItem> &items);

    virtual void clearItems(const QPtrList<PlaylistItem> &items);

    QPtrList<PlaylistItem> selectedItems() const;

    void remove();
    void remove(const QPtrList<PlaylistItem> &items);

    // These are used in a hard-core, encapsulation breaking way and should be
    // replaced soon (see PlaylistItem).  Unfortunately they're more efficient
    // than elegant solutions.  These also should be removed by doing the
    // checking that is currently done in PlaylistItem in the CollectionList
    // subclass of Playlist.

    QStringList &getArtistList();
    QStringList &getAlbumList();

private:
    void setup();
    void appendImpl(const QString &item);

    QStringList extensions;
    QStringList members;
    QStringList artistList;
    QStringList albumList;
    void processEvents();
    int processed;

private slots:
    void emitSelected();

signals:
    void dataChanged();

    // This signal should later be moved to the "CollectionList" subclass of
    // Playlist.

    void collectionChanged(Playlist *list);
    void selectionChanged(const QPtrList<PlaylistItem> &selection);
};

#endif
