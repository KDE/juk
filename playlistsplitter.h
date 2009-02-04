/***************************************************************************
    begin                : Fri Sep 13 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
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

#ifndef PLAYLISTSPLITTER_H
#define PLAYLISTSPLITTER_H

#include <QSplitter>

class QStackedWidget;

class Playlist;
class SearchWidget;
class PlaylistInterface;
class TagEditor;
class PlaylistBox;
class NowPlaying;

/**
 * This is the main layout class of JuK.  It should contain a PlaylistBox and
 * a QStackedWidget of the Playlists.
 *
 * This class serves as a "mediator" (see "Design Patterns") between the JuK
 * class and the playlist classes.  Thus all access to the playlist classes from
 * non-Playlist related classes should be through the public API of this class.
 */

class PlaylistSplitter : public QSplitter
{
    Q_OBJECT

public:
    explicit PlaylistSplitter(QWidget *parent, const char *name = 0);
    virtual ~PlaylistSplitter();

    PlaylistInterface *playlist() const;

    virtual bool eventFilter(QObject *watched, QEvent *event);

signals:
    /**
     * Emitted when GUI is created and the cache is loaded.  Is kind of a hack
     * until we move the time-intensive parts to a separate thread but then
     * again at least this works.
     */
    void guiReady();

public slots:
    virtual void setFocus();
    virtual void slotFocusCurrentPlaylist();
    void slotEnable();

private:

    /**
     * This returns a pointer to the first item in the playlist on the top
     * of the QStackedWidget of playlists.
     */
    Playlist *visiblePlaylist() const;

    void setupActions();
    void setupLayout();
    void readConfig();
    void saveConfig();

private slots:

    /**
     * Updates the visible search results based on the result of the search
     * associated with the currently visible playlist.
     */
    void slotShowSearchResults();
    void slotPlaylistSelectionChanged();
    void slotPlaylistChanged(int i);

private:
    Playlist *m_newVisible;
    PlaylistBox *m_playlistBox;
    SearchWidget *m_searchWidget;
    QStackedWidget *m_playlistStack;
    TagEditor *m_editor;
    NowPlaying *m_nowPlaying;
};

#endif

// vim: set et sw=4 tw=0 sta:
