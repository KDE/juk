/***************************************************************************
                          juk.h  -  description
                             -------------------
    begin                : Mon Feb  4 23:40:41 EST 2002
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

#include "slideraction.h"
#include "player.h"
#include "playlistsplitter.h"

class JuK : public KMainWindow
{
    Q_OBJECT
public:
    JuK(QWidget* parent = 0, const char *name = 0);
    virtual ~JuK();

private:
    // private methods
    void setupActions();
    void setupLayout();
    void setupPlayer();
    void processArgs();
    void readConfig();
    void saveConfig();

    // layout objects
    PlaylistSplitter *splitter;

    // actions
    KToggleAction *showEditorAction;
    SliderAction *sliderAction;
    KAction *playAction;
    KAction *pauseAction;
    KAction *stopAction;

    QTimer *playTimer;
    Player player;
    PlaylistItem *playingItem;
    bool trackPositionDragging;
    bool noSeek;

    const static int pollInterval = 800;

private slots:
    // file menu
    void openFile();
    void openDirectory();
    void saveFile();
    void remove();
    void quit();

    // edit menu
    void cut();
    void copy() {};
    void paste() {};
    void selectAll(bool select = true);

    void playFile();
    void pauseFile();
    void stopFile();

    // additional player slots
    void trackPositionSliderClick();
    void trackPositionSliderRelease();
    void trackPositionSliderUpdate(int position);
    void pollPlay();
    void setVolume(int volume);
    void playItem(QListViewItem *item);
    void playItem(PlaylistItem *item);
    void playTaggerItem(QListViewItem *item);
    void playTaggerItem(PlaylistItem *item);
};

#endif
