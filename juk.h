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
#include <kmenubar.h>

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

signals:
    void signalEdit();
    void signalNewSong(const QString& songTitle);

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

    virtual bool queryExit();
    virtual bool queryClose();

    void invokeEditSlot(const char *slotName, const char *slot);
    QString playingString() const;
    void updatePlaylistInfo();

    /**
     * This is the main method to play stuff.  Everything else is just a wrapper
     * around this.
     */
    void play(const QString &file);

private slots:
    void slotPlaylistChanged();

    // edit menu
    void cut();
    void copy();
    void paste();
    void clear();
    void selectAll();

    // player menu
    void slotPlay();
    void slotPause();
    void slotStop();
    void slotBack();
    void slotForward();

    // settings menu
    void slotShowGenreListEditor();
    void slotToggleSystemTray(bool enabled);
    void slotSetOutput(int output);

    // additional player slots
    void slotTrackPositionSliderClicked();
    void slotTrackPositionSliderReleased();
    void slotTrackPositionSliderUpdate(int position);

    /**
     * This method is called to check our progress in the playing file.  It uses
     * m_playTimer to know when to call itself again.
     */
    void slotPollPlay();

    /**
     * This method is called by the slider to set the volume of the player.  Its
     * value is relative to the maxValue() of the volume slider.
     */
    void slotSetVolume(int volume);

    void slotPlaySelectedFile() { play(m_splitter->playSelectedFile()); }
    void slotPlayFirstFile() { play(m_splitter->playFirstFile()); }
    void slotToggleMenuBar() { menuBar()->isVisible() ? menuBar()->hide() : menuBar()->show(); }
    void slotToggleToolBar() { toolBar()->isVisible() ? toolBar()->hide() : toolBar()->show(); }

private:
    // layout objects
    PlaylistSplitter *m_splitter;
    StatusLabel *m_statusLabel;
    SystemTray *m_systemTray;

    // actions
    KToggleAction *m_showEditorAction;
    KToggleAction *m_restoreOnLoadAction;
    SliderAction *m_sliderAction;
    KToggleAction *m_randomPlayAction;
    KToggleAction *m_toggleSystemTrayAction;
    KSelectAction *m_outputSelectAction;

    KAction *m_playAction;
    KAction *m_pauseAction;
    KAction *m_stopAction;
    KAction *m_backAction;
    KAction *m_forwardAction;

    KAction *m_savePlaylistAction;
    KAction *m_saveAsPlaylistAction;
    KAction *m_renamePlaylistAction;
    KAction *m_deleteItemPlaylistAction;

    QTimer *m_playTimer;
    Player *m_player;

    bool m_trackPositionDragging;
    bool m_noSeek;
    bool m_restore;

    static const int m_pollInterval = 800;
};

#endif
