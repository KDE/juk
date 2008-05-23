/***************************************************************************
    begin                : Mon Feb  4 23:40:41 EST 2002
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

#include <config.h>

#include <kcmdlineargs.h>
#include <kstatusbar.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>


#include "juk.h"
#include "slideraction.h"
#include "statuslabel.h"
#include "splashscreen.h"
#include "systemtray.h"
#include "keydialog.h"
#include "tagguesserconfigdlg.h"
#include "filerenamerconfigdlg.h"
#include "actioncollection.h"
#include "cache.h"
#include "playlistsplitter.h"
#include "collectionlist.h"
#include "covermanager.h"
#include "tagtransactionmanager.h"

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

JuK::JuK(QWidget *parent, const char *name) :
    KMainWindow(parent, name, WDestructiveClose),
    m_player(PlayerManager::instance()),
    m_shuttingDown(false)
{
    // Expect segfaults if you change this order.

    readSettings();

    if(m_showSplash && !m_startDocked && Cache::cacheFileExists()) {
	SplashScreen::instance()->show();
	kapp->processEvents();
    }

    setupActions();
    setupLayout();
	 
    if(QApplication::reverseLayout())
	setupGUI(ToolBar | Save | Create, "jukui-rtl.rc");
    else
	setupGUI(ToolBar | Save | Create);

    readConfig();
    setupSystemTray();
    setupGlobalAccels();
    createDirs();

    SplashScreen::finishedLoading();
    QTimer::singleShot(0, CollectionList::instance(), SLOT(slotCheckCache()));
    QTimer::singleShot(0, this, SLOT(slotProcessArgs()));

    m_sliderAction->slotUpdateOrientation();
}

JuK::~JuK()
{
    kdDebug(65432) << k_funcinfo << endl;
}

KActionCollection *JuK::actionCollection() const
{
    return ActionCollection::actions();
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void JuK::setupLayout()
{
    new TagTransactionManager(this);

    m_splitter = new PlaylistSplitter(this, "playlistSplitter");
    setCentralWidget(m_splitter);

    m_statusLabel = new StatusLabel(m_splitter->playlist(), statusBar());
    connect(CollectionList::instance(), SIGNAL(signalCollectionChanged()),
            m_statusLabel, SLOT(updateData()));
    statusBar()->addWidget(m_statusLabel, 1);
    PlayerManager::instance()->setStatusLabel(m_statusLabel);

    m_splitter->setFocus();
    resize(750, 500);
}

void JuK::setupActions()
{
    ActionCollection::actions()->setWidget(this);

    KStdAction::quit(this, SLOT(slotQuit()), actions());
    KStdAction::undo(this, SLOT(slotUndo()), actions());
    KStdAction::cut(kapp,   SLOT(cut()),   actions());
    KStdAction::copy(kapp,  SLOT(copy()),  actions());
    KStdAction::paste(kapp, SLOT(paste()), actions());
    KStdAction::clear(kapp, SLOT(clear()), actions());
    KStdAction::selectAll(kapp, SLOT(selectAll()), actions());

    new KAction(i18n("Remove From Playlist"), "edit_remove", 0, kapp, SLOT(clear()), actions(), "removeFromPlaylist");

    KActionMenu *actionMenu = new KActionMenu(i18n("&Random Play"), "roll", actions(), "actionMenu");
    actionMenu->setDelayed(false);

    KRadioAction *ka = new KRadioAction(i18n("&Disable Random Play"), "player_playlist", 0, actions(), "disableRandomPlay");
    ka->setExclusiveGroup("randomPlayGroup");
    actionMenu->insert(ka);

    m_randomPlayAction = new KRadioAction(i18n("Use &Random Play"), "roll", 0, actions(), "randomPlay");
    m_randomPlayAction->setExclusiveGroup("randomPlayGroup");
    actionMenu->insert(m_randomPlayAction);

    ka = new KRadioAction(i18n("Use &Album Random Play"), "roll", 0, actions(), "albumRandomPlay");
    ka->setExclusiveGroup("randomPlayGroup");
    connect(ka, SIGNAL(toggled(bool)), SLOT(slotCheckAlbumNextAction(bool)));
    actionMenu->insert(ka);

    new KAction(i18n("&Play"),  "player_play",  0, m_player, SLOT(play()),  actions(), "play");
    new KAction(i18n("P&ause"), "player_pause", 0, m_player, SLOT(pause()), actions(), "pause");
    new KAction(i18n("&Stop"),  "player_stop",  0, m_player, SLOT(stop()),  actions(), "stop");

    new KToolBarPopupAction(i18n("previous track", "Previous"), "player_start", KShortcut(), m_player, SLOT(back()), actions(), "back");
    new KAction(i18n("next track", "&Next"), "player_end", KShortcut(), m_player, SLOT(forward()), actions(), "forward");
    new KToggleAction(i18n("&Loop Playlist"), 0, KShortcut(), actions(), "loopPlaylist");
    KToggleAction *resizeColumnAction =
        new KToggleAction(i18n("&Resize Playlist Columns Manually"),
	                  KShortcut(), actions(), "resizeColumnsManually");
    resizeColumnAction->setCheckedState(i18n("&Resize Column Headers Automatically"));

    // the following are not visible by default

    new KAction(i18n("Mute"),         "mute",        0, m_player, SLOT(mute()),        actions(), "mute");
    new KAction(i18n("Volume Up"),    "volumeUp",    0, m_player, SLOT(volumeUp()),    actions(), "volumeUp");
    new KAction(i18n("Volume Down"),  "volumeDown",  0, m_player, SLOT(volumeDown()),  actions(), "volumeDown");
    new KAction(i18n("Play / Pause"), "playPause",   0, m_player, SLOT(playPause()),   actions(), "playPause");
    new KAction(i18n("Seek Forward"), "seekForward", 0, m_player, SLOT(seekForward()), actions(), "seekForward");
    new KAction(i18n("Seek Back"),    "seekBack",    0, m_player, SLOT(seekBack()),    actions(), "seekBack");

    //////////////////////////////////////////////////
    // settings menu
    //////////////////////////////////////////////////

    m_toggleSplashAction =
	new KToggleAction(i18n("Show Splash Screen on Startup"),
			  KShortcut(), actions(), "showSplashScreen");
    m_toggleSplashAction->setCheckedState(i18n("Hide Splash Screen on Startup"));
    m_toggleSystemTrayAction =
	new KToggleAction(i18n("&Dock in System Tray"),
			  KShortcut(), actions(), "toggleSystemTray");
    m_toggleDockOnCloseAction =
	new KToggleAction(i18n("&Stay in System Tray on Close"),
			  KShortcut(), actions(), "dockOnClose");
    m_togglePopupsAction =
	new KToggleAction(i18n("Popup &Track Announcement"),
			  KShortcut(), this, 0, actions(), "togglePopups");
    new KToggleAction(i18n("Save &Play Queue on Exit"),
                      KShortcut(), this, 0, actions(), "saveUpcomingTracks");

    connect(m_toggleSystemTrayAction, SIGNAL(toggled(bool)),
	    this, SLOT(slotToggleSystemTray(bool)));


    m_outputSelectAction = PlayerManager::playerSelectAction(actions());

    if(m_outputSelectAction)
        m_outputSelectAction->setCurrentItem(0);

    new KAction(i18n("&Tag Guesser..."), 0, 0, this, SLOT(slotConfigureTagGuesser()),
		actions(), "tagGuesserConfig");

    new KAction(i18n("&File Renamer..."), 0, 0, this, SLOT(slotConfigureFileRenamer()),
		actions(), "fileRenamerConfig");

    KStdAction::keyBindings(this, SLOT(slotEditKeys()), actions());

    //////////////////////////////////////////////////
    // just in the toolbar
    //////////////////////////////////////////////////

    m_sliderAction = new SliderAction(i18n("Track Position"), actions(),
				      "trackPositionAction");
}

void JuK::setupSystemTray()
{
    if(m_toggleSystemTrayAction && m_toggleSystemTrayAction->isChecked()) {
	m_systemTray = new SystemTray(this, "systemTray");
	m_systemTray->show();

	m_toggleDockOnCloseAction->setEnabled(true);
	m_togglePopupsAction->setEnabled(true);

	connect(m_systemTray, SIGNAL(quitSelected()), this, SLOT(slotAboutToQuit()));
    }
    else {
	m_systemTray = 0;
	m_toggleDockOnCloseAction->setEnabled(false);
	m_togglePopupsAction->setEnabled(false);
    }
}

void JuK::setupGlobalAccels()
{
    m_accel = new KGlobalAccel(this);

    KeyDialog::insert(m_accel, "Play",        i18n("Play"),         action("play"),        SLOT(activate()));
    KeyDialog::insert(m_accel, "PlayPause",   i18n("Play / Pause"), action("playPause"),   SLOT(activate()));
    KeyDialog::insert(m_accel, "Stop",        i18n("Stop Playing"), action("stop"),        SLOT(activate()));
    KeyDialog::insert(m_accel, "Back",        i18n("Back"),         action("back"),        SLOT(activate()));
    KeyDialog::insert(m_accel, "Forward",     i18n("Forward"),      action("forward"),     SLOT(activate()));
    KeyDialog::insert(m_accel, "SeekBack",    i18n("Seek Back"),    action("seekBack"),    SLOT(activate()));
    KeyDialog::insert(m_accel, "SeekForward", i18n("Seek Forward"), action("seekForward"), SLOT(activate()));
    KeyDialog::insert(m_accel, "VolumeUp",    i18n("Volume Up"),    action("volumeUp"),    SLOT(activate()));
    KeyDialog::insert(m_accel, "VolumeDown",  i18n("Volume Down"),  action("volumeDown"),  SLOT(activate()));
    KeyDialog::insert(m_accel, "Mute",        i18n("Mute"),         action("mute"),        SLOT(activate()));
    KeyDialog::insert(m_accel, "ShowHide",    i18n("Show / Hide"),  this,                  SLOT(slotShowHide()));
    KeyDialog::insert(m_accel, "ForwardAlbum", i18n("Play Next Album"), action("forwardAlbum"), SLOT(activate()));

    m_accel->setConfigGroup("Shortcuts");
    m_accel->readSettings();
    m_accel->updateConnections();
}

void JuK::slotProcessArgs()
{
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    QStringList files;

    for(int i = 0; i < args->count(); i++)
        files.append(args->arg(i));

    CollectionList::instance()->addFiles(files);
}

void JuK::createDirs()
{
    QDir dir(KGlobal::dirs()->saveLocation("data", kapp->instanceName() + '/'));
    if (!dir.exists("covers", false))
        dir.mkdir("covers", false);
    dir.cd("covers");
    if (!dir.exists("large", false))
        dir.mkdir("large", false);
}

void JuK::keyPressEvent(QKeyEvent *e)
{
    if (e->key() >= Qt::Key_Back && e->key() <= Qt::Key_MediaLast)
        e->accept();
    KMainWindow::keyPressEvent(e);
}

/**
 * These are settings that need to be know before setting up the GUI.
 */

void JuK::readSettings()
{
    KConfigGroup config(KGlobal::config(), "Settings");
    m_showSplash = config.readBoolEntry("ShowSplashScreen", true);
    m_startDocked = config.readBoolEntry("StartDocked", false);
}

void JuK::readConfig()
{
    // player settings

    KConfigGroup playerConfig(KGlobal::config(), "Player");

    if(m_sliderAction->volumeSlider()) {
	int maxVolume = m_sliderAction->volumeSlider()->maxValue();
	int volume = playerConfig.readNumEntry("Volume", maxVolume);
	m_sliderAction->volumeSlider()->setVolume(volume);
    }

    // Default to no random play

    ActionCollection::action<KToggleAction>("disableRandomPlay")->setChecked(true);

    QString randomPlayMode = playerConfig.readEntry("RandomPlay", "Disabled");
    if(randomPlayMode == "true" || randomPlayMode == "Normal")
	m_randomPlayAction->setChecked(true);
    else if(randomPlayMode == "AlbumRandomPlay")
	ActionCollection::action<KToggleAction>("albumRandomPlay")->setChecked(true);

    bool loopPlaylist = playerConfig.readBoolEntry("LoopPlaylist", false);
    ActionCollection::action<KToggleAction>("loopPlaylist")->setChecked(loopPlaylist);

    // general settings
    
    KConfigGroup settingsConfig(KGlobal::config(), "Settings");

    bool dockInSystemTray = settingsConfig.readBoolEntry("DockInSystemTray", true);
    m_toggleSystemTrayAction->setChecked(dockInSystemTray);

    bool dockOnClose = settingsConfig.readBoolEntry("DockOnClose", true);
    m_toggleDockOnCloseAction->setChecked(dockOnClose);

    bool showPopups = settingsConfig.readBoolEntry("TrackPopup", false);
    m_togglePopupsAction->setChecked(showPopups);

    if(m_outputSelectAction)
	m_outputSelectAction->setCurrentItem(settingsConfig.readNumEntry("MediaSystem", 0));

    m_toggleSplashAction->setChecked(m_showSplash);
}

void JuK::saveConfig()
{
    // player settings
    
    KConfigGroup playerConfig(KGlobal::config(), "Player");

    if (m_sliderAction->volumeSlider())
    {
        playerConfig.writeEntry("Volume", m_sliderAction->volumeSlider()->volume());
    }

    playerConfig.writeEntry("RandomPlay", m_randomPlayAction->isChecked());

    KToggleAction *a = ActionCollection::action<KToggleAction>("loopPlaylist");
    playerConfig.writeEntry("LoopPlaylist", a->isChecked());

    a = ActionCollection::action<KToggleAction>("albumRandomPlay");
    if(a->isChecked())
	playerConfig.writeEntry("RandomPlay", "AlbumRandomPlay");
    else if(m_randomPlayAction->isChecked())
	playerConfig.writeEntry("RandomPlay", "Normal");
    else
	playerConfig.writeEntry("RandomPlay", "Disabled");

    // general settings

    KConfigGroup settingsConfig(KGlobal::config(), "Settings");
    settingsConfig.writeEntry("ShowSplashScreen", m_toggleSplashAction->isChecked());
    settingsConfig.writeEntry("StartDocked", m_startDocked);
    settingsConfig.writeEntry("DockInSystemTray", m_toggleSystemTrayAction->isChecked());
    settingsConfig.writeEntry("DockOnClose", m_toggleDockOnCloseAction->isChecked());
    settingsConfig.writeEntry("TrackPopup", m_togglePopupsAction->isChecked());
    if(m_outputSelectAction)
	settingsConfig.writeEntry("MediaSystem", m_outputSelectAction->currentItem());

    KGlobal::config()->sync();
}

bool JuK::queryExit()
{
    m_startDocked = !isVisible();

    kdDebug(65432) << k_funcinfo << endl;

    hide();

    action("stop")->activate();
    delete m_systemTray;
    m_systemTray = 0;

    CoverManager::shutdown();
    Cache::instance()->save();
    saveConfig();

    delete m_splitter;
    m_splitter = 0;
    return true;
}

bool JuK::queryClose()
{
    kdDebug(65432) << k_funcinfo << endl;

    if(!m_shuttingDown &&
       !kapp->sessionSaving() &&
       m_systemTray &&
       m_toggleDockOnCloseAction->isChecked())
    {
	KMessageBox::information(this,
	    i18n("<qt>Closing the main window will keep JuK running in the system tray. "
		 "Use Quit from the File menu to quit the application.</qt>"),
	    i18n("Docking in System Tray"), "hideOnCloseInfo");
	hide();
	return false;
    }
    else
	return true;
}

////////////////////////////////////////////////////////////////////////////////
// private slot definitions
////////////////////////////////////////////////////////////////////////////////

void JuK::slotShowHide()
{
    setShown(!isShown());
}

void JuK::slotAboutToQuit()
{
    m_shuttingDown = true;
}

void JuK::slotQuit()
{
    kdDebug(65432) << k_funcinfo << endl;
    m_shuttingDown = true;

    kapp->quit();
}

////////////////////////////////////////////////////////////////////////////////
// settings menu
////////////////////////////////////////////////////////////////////////////////

void JuK::slotToggleSystemTray(bool enabled)
{
    if(enabled && !m_systemTray)
	setupSystemTray();
    else if(!enabled && m_systemTray) {
	delete m_systemTray;
	m_systemTray = 0;
	m_toggleDockOnCloseAction->setEnabled(false);
	m_togglePopupsAction->setEnabled(false);
    }
}

void JuK::slotEditKeys()
{
    KeyDialog::configure(m_accel, actions(), this);
}

void JuK::slotConfigureTagGuesser()
{
    TagGuesserConfigDlg(this).exec();
}

void JuK::slotConfigureFileRenamer()
{
    FileRenamerConfigDlg(this).exec();
}

void JuK::slotUndo()
{
    TagTransactionManager::instance()->undo();
}

void JuK::slotCheckAlbumNextAction(bool albumRandomEnabled)
{
    // If album random play is enabled, then enable the Play Next Album action
    // unless we're not playing right now.

    if(albumRandomEnabled && !m_player->playing())
	albumRandomEnabled = false;

    action("forwardAlbum")->setEnabled(albumRandomEnabled);
}

#include "juk.moc"
