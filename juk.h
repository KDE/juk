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

#include <kaction.h>
#include <kglobalaccel.h>
#include <kmainwindow.h>
#include <kpopupmenu.h>

#include "playermanager.h"
#include "playlistsplitter.h"
#include "jukIface.h"

class QTimer;
class QListViewItem;

class SliderAction;
class StatusLabel;
class SystemTray;

class JuK : public KMainWindow, public CollectionIface
{
    Q_OBJECT

public:
    JuK(QWidget* parent = 0, const char *name = 0);
    virtual ~JuK();
    virtual KActionCollection *actionCollection() const;

    /**
     * This forwards on the request to enable or disable directory scanning for
     * new files being added or removed.
     */
    void setDirWatchEnabled(bool enabled) { m_splitter->setDirWatchEnabled(enabled); }

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

    void updatePlaylistInfo();

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
    void slotShowHide();

    void slotPlaylistChanged();

    void slotQuit();

    void slotToggleSystemTray(bool enabled);
    void slotEditKeys();
    void slotConfigureTagGuesser();
    void slotConfigureFileRenamer();
    void slotGuessTagInfoFromFile();
    void slotGuessTagInfoFromInternet();

private:
    PlaylistSplitter *m_splitter;
    StatusLabel *m_statusLabel;
    SystemTray *m_systemTray;

    typedef QPair<KAction *, const char *> SplitterConnection;
    QValueList<SplitterConnection> m_splitterConnections;

    SliderAction *m_sliderAction;
    KToggleAction *m_showSearchAction;
    KToggleAction *m_showEditorAction;
    KToggleAction *m_showHistoryAction;
    KToggleAction *m_randomPlayAction;
    KToggleAction *m_toggleSystemTrayAction;
    KToggleAction *m_toggleDockOnCloseAction;
    KToggleAction *m_togglePopupsAction;
    KToggleAction *m_toggleSplashAction;
    KToggleAction *m_loopPlaylistAction;
    KSelectAction *m_outputSelectAction;
    KToolBarPopupAction *m_backAction;

    PlayerManager *m_player;
    KGlobalAccel *m_accel;

    bool m_startDocked;
    bool m_showSplash;
    bool m_shuttingDown;
};

#endif
