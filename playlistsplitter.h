/***************************************************************************
                          playlistsplitter.h  -  description
                             -------------------
    begin                : Fri Sep 13 2002
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

#ifndef PLAYLISTSPLITTER_H
#define PLAYLISTSPLITTER_H

#include <klocale.h>

#include <qsplitter.h>
#include <qwidgetstack.h>

#include "playlist.h"
#include "playlistbox.h"
#include "collectionlist.h"
#include "tageditor.h"

/** This is the main layout class of JuK.  It should contain a PlaylistBox and
    a QWidgetStack of the Playlists. */

class PlaylistSplitter : public QSplitter
{
    Q_OBJECT
public:
    PlaylistSplitter(QWidget *parent = 0, const char *name = 0);
    virtual ~PlaylistSplitter();

    void createPlaylist(const QString &name);
    QPtrList<PlaylistItem> playlistSelection() const;
    PlaylistItem *playlistFirstItem() const;

public slots:
    void open(const QStringList &files);
    void open(const QString &file);
    void save() {}
    /** Deletes the selected items from the hard disk. */
    void remove();
    /** Removes the selected items from the playlist. */
    void clearSelectedItems();
    void selectAll(bool select = true);

    void setEditorVisible(bool visible);

private:
    void setupLayout();
    void readConfig();

    PlaylistBox *playlistBox;
    QWidgetStack *playlistStack;
    TagEditor *editor;

    CollectionList *collection;

    bool showEditor;

private slots:
    // playlist box slots
    void changePlaylist(PlaylistBoxItem *item);

signals:
    void playlistDoubleClicked(QListViewItem *);
};

#endif
