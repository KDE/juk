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

#include <kaction.h>
#include <kstdaction.h>
#include <kmainwindow.h>

#include <qlabel.h>

#include "player.h"
#include "playlistsplitter.h"

class QTimer;
class QListViewItem;

class SliderAction;
class StatusLabel;
class SystemTray;

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
    void setupSystemTray();

    void processArgs();

    /**
     * readSettings() is separate from readConfig() in that it contains settings
     * that need to be read before the GUI is setup.
     */
    void readSettings();
    void readConfig();
    void saveConfig();

    virtual bool queryClose();

private slots:
    void playlistChanged();
    void updatePlaylistInfo();

    // edit menu
    void cut() { splitter->copy(); splitter->clear(); }

    // player menu
    void play();
    void pause();
    void stop();
    void back();
    void forward();

    // settings menu
    void showGenreListEditor();
    void toggleSystemTray(bool enabled);
    void setOutput(int output);

    // additional player slots
    void trackPositionSliderClick();
    void trackPositionSliderRelease();
    void trackPositionSliderUpdate(int position);

    /**
     * This method is called to check our progress in the playing file.  It uses
     * playTimer to know when to call itself again.
     */
    void pollPlay();

    /**
     * This method is called by the slider to set the volume of the player.  Its
     * value is relative to the maxValue() of the volume slider.
     */
    void setVolume(int volume);

    /**
     * This is the main method to play stuff.  Everything else is just a wrapper
     * around this.
     */
    void play(const QString &file);

    void playSelectedFile() { play(splitter->playSelectedFile()); }
    void playFirstFile() { play(splitter->playFirstFile()); }

private:
    // layout objects
    PlaylistSplitter *splitter;
    StatusLabel *statusLabel;
    SystemTray *systemTray;

    // actions
    KToggleAction *showEditorAction;
    KToggleAction *restoreOnLoadAction;
    SliderAction *sliderAction;
    KToggleAction *randomPlayAction;
    KToggleAction *toggleSystemTrayAction;
    KSelectAction *outputSelectAction;

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
    Player *player;

    bool trackPositionDragging;
    bool noSeek;
    bool restore;

    static const int pollInterval = 800;
};

#endif
