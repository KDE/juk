
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
#include <kglobalaccel.h>
#include <kstdaction.h>
#include <kmainwindow.h>
#include <kmenubar.h>
#include <kpopupmenu.h>

#include <qlabel.h>

#include "player.h"
#include "playlistsplitter.h"
#include "jukIface.h"

class QTimer;
class QListViewItem;

class SliderAction;
class StatusLabel;
class SystemTray;

class JuK : public KMainWindow, virtual public JuKIface
{
    Q_OBJECT

public:
    JuK(QWidget* parent = 0, const char *name = 0);
    virtual ~JuK();

public slots:
    void play();
    void pause();
    void stop();
    void back();
    void back(int howMany);
    void slotPopulateBackMenu();
    void forward();
    void seekBack();
    void seekForward();

    void playPause();

    void volumeUp();
    void volumeDown();
    void volumeMute();

signals:
    void signalEdit();
    void signalNewSong(const QString& songTitle);

private:
    void setupLayout();
    void setupActions();
    /**
     * Solves the problem of the splitter needing to use some of the actions from
     * this class and as such them needing to be created before the
     * PlaylistSplitter, but also needing to connect to the playlist splitter.
     *
     * @see createSplitterAction();
     */
    void setupSplitterConnections();
    void setupPlayer();
    void setupSystemTray();
    void setupGlobalAccels();

    void processArgs();

    void keyPressEvent(QKeyEvent *);

    /**
     * readSettings() is separate from readConfig() in that it contains settings
     * that need to be read before the GUI is setup.
     */
    void readSettings();
    void readConfig();
    void saveConfig();

    virtual bool queryExit();
    virtual bool queryClose();

    QString playingString() const;

    int currentTime() const { return m_player->currentTime(); }
    int totalTime() const { return m_player->totalTime(); }

    /**
     * Set the volume.  100 is the maximum.
     */
    void setVolume(float volume);

    /**
     * Set the position in the currently playing track (in seconds).
     */
    void setTime(int time) { m_player->seek(time); }

    void updatePlaylistInfo();

    /**
     * This is the main method to play stuff.  Everything else is just a wrapper
     * around this.
     */
    void play(const QString &file);

    void openFile(const QString &file);
    void openFile(const QStringList &files);

    /**
     * Because we want to be able to reuse these actions in the main GUI classes,
     * which are created by the PlaylistSplitter, it is useful to create them
     * before creating the splitter.  This however creates a problem in that we
     * also need to connect them to the splitter.  This method builds creates
     * actions and builds a list of connections that can be set up after the
     * splitter is created.
     */
    KAction *createSplitterAction(const QString &text, 
				  const char *slot, 
				  const char *name,
				  const QString &pix = QString::null,
				  const KShortcut &shortcut = KShortcut());

private slots:
    void slotPlaylistChanged();

    // file menu
    void slotQuit() { m_shuttingDown = true; kapp->quit(); }

    // settings menu
    void slotShowGenreListEditor();
    void slotToggleSystemTray(bool enabled);
    void slotSetOutput(int output);
    void slotEditKeys();
    void slotConfigureTagGuesser();

    // additional player slots
    void slotTrackPositionSliderClicked();
    void slotTrackPositionSliderReleased();
    void slotTrackPositionSliderUpdate(int position);

    /**
     * This method is called by the slider to set the volume of the player.  Its
     * value is relative to the maxValue() of the volume slider.
     */
    void slotSetVolume(int volume);

    /**
     * This method is called to check our progress in the playing file.  It uses
     * m_playTimer to know when to call itself again.
     */
    void slotPollPlay();
    void slotPlaySelectedFile() { play(m_splitter->playSelectedFile()); }
    void startPlayingPlaylist();
    void slotToggleMenuBar() { menuBar()->isVisible() ? menuBar()->hide() : menuBar()->show(); }
    void slotGuessTagInfo(int i);

private:
    // layout objects
    PlaylistSplitter *m_splitter;
    StatusLabel *m_statusLabel;
    SystemTray *m_systemTray;

    typedef QPair<KAction *, const char *> SplitterConnection;
    QValueList<SplitterConnection> m_splitterConnections;

    // actions
    KToggleAction *m_showSearchAction;
    KToggleAction *m_showEditorAction;
    KToggleAction *m_showHistoryAction;
    SliderAction *m_sliderAction;
    KToggleAction *m_randomPlayAction;
    KToggleAction *m_toggleSystemTrayAction;
    KToggleAction *m_toggleDockOnCloseAction;
    KToggleAction *m_togglePopupsAction;
    KToggleAction *m_toggleSplashAction;
    KSelectAction *m_outputSelectAction;
    KActionMenu *m_guessMenu;

    KToolBarPopupAction *m_backAction;
    KToggleAction *m_loopPlaylistAction;

    QTimer *m_playTimer;
    Player *m_player;
    KGlobalAccel *m_accel;

    bool m_trackPositionDragging;
    bool m_noSeek;
    bool m_showSplash;
    bool m_shuttingDown;
    bool m_muted;

    static const int m_pollInterval = 800;
};

#endif
