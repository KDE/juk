/***************************************************************************
                          juk.h  -  description
                             -------------------
    begin                : Mon Feb  4 23:40:41 EST 2002
    copyright            : (C) 2002 by Scott Wheeler
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

#ifndef JUK_H
#define JUK_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kapp.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kmainwindow.h>

#include <qtimer.h>
#include <qlistview.h>
#include <qlabel.h>

#include "player.h"

class Playlist;
class PlaylistSplitter;
class PlaylistItem;
class SliderAction;
class StatusLabel;

class JuK : public KMainWindow
{
    Q_OBJECT
public:
    JuK(QWidget* parent = 0, const char *name = 0);
    virtual ~JuK();

private:

    // private methods
    void setupLayout();
    void setupActions();
    void setupPlayer();
    void processArgs();
    void readConfig();
    /**
     * This is only separate from readConfig() because it is useful to call it
     * before we construct the splitter.
     */
    void readSettings();
    void saveConfig();

    virtual bool queryClose();

    // layout objects
    PlaylistSplitter *splitter;
    StatusLabel *statusLabel;

    // actions
    KToggleAction *showEditorAction;
    KToggleAction *restoreOnLoadAction;
    SliderAction *sliderAction;
    KToggleAction *randomPlayAction;

    KAction *playAction;
    KAction *pauseAction;
    KAction *stopAction;
    KAction *backAction;
    KAction *forwardAction;

    KAction *savePlaylistAction;
    KAction *saveAsPlaylistAction;
    KAction *renamePlaylistAction;
    KAction *deleteItemPlaylistAction;

    QTimer *playTimer;
    Player player;
    PlaylistItem *playingItem;

    bool trackPositionDragging;
    bool noSeek;
    bool restore;

    static const int pollInterval = 800;

private slots:
    void playlistChanged(Playlist *list);

    // file menu
    void remove();
    void quit();

    // edit menu
    void cut();
    void copy() {};
    void paste() {};

    // player menu
    void playFile();
    void pauseFile();
    void stopFile();
    void backFile();
    void forwardFile();

    // additional player slots
    void trackPositionSliderClick();
    void trackPositionSliderRelease();
    void trackPositionSliderUpdate(int position);
    void pollPlay();
    void setVolume(int volume);
    /**
     * This is just a wrapper around the below method to take a QListViewItem.
     */
    void playItem(QListViewItem *item);
    /**
     * This is the main method to play stuff.  All of the other play related 
     * members in this class ultimately call this method.
     */
    void playItem(PlaylistItem *item);
};

#endif
