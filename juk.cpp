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
#include <kactioncollection.h>
#include <kstdaction.h>
#include <ktoggleaction.h>
#include <kactionmenu.h>
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

#include <QKeyEvent>
#include <kaction.h>
#include <ktoggleaction.h>
#include <kactionmenu.h>

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

JuK::JuK(QWidget *parent) :
    KMainWindow(parent, Qt::WDestructiveClose),
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

    if(QApplication::isRightToLeft())
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
    kDebug(65432) << k_funcinfo << endl;
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
    ActionCollection::actions()->setAssociatedWidget(this);
    KActionCollection *collection = ActionCollection::actions();

    // Setup KDE standard actions that JuK uses.

    KStdAction::quit(this, SLOT(slotQuit()), collection);
    KStdAction::undo(this, SLOT(slotUndo()), collection);
    KStdAction::cut(collection);
    KStdAction::copy(collection);
    KStdAction::paste(collection);
    KAction *clear = KStdAction::clear(collection);
    KStdAction::selectAll(collection);
    KStdAction::keyBindings(this, SLOT(slotEditKeys()), collection);


    // Setup the menu which handles the random play options.
    KActionMenu *actionMenu = new KActionMenu(KIcon("roll"), i18n("&Random Play"), collection, "actionMenu");
    actionMenu->setDelayed(false);

    // ### KDE4: Investigate how QActionGroups integrate into menus now.
    QActionGroup* randomPlayGroup = new QActionGroup(this);

    KAction *act = new KToggleAction(KIcon("player_playlist"), i18n("&Disable Random Play"), collection, "disableRandomPlay");
    act->setActionGroup(randomPlayGroup);
    actionMenu->addAction(act);

    m_randomPlayAction = new KToggleAction(KIcon("roll"), i18n("Use &Random Play"), collection, "randomPlay");
    m_randomPlayAction->setActionGroup(randomPlayGroup);
    actionMenu->addAction(m_randomPlayAction);

    act = new KToggleAction(KIcon("roll"), i18n("Use &Album Random Play"), collection, "albumRandomPlay");
    act->setActionGroup(randomPlayGroup);
    connect(act, SIGNAL(triggered(bool)), SLOT(slotCheckAlbumNextAction(bool)));
    actionMenu->addAction(act);

    act = new KAction(KIcon("edit_remove"), i18n("Remove From Playlist"), collection, "removeFromPlaylist");
    connect(act, SIGNAL(triggered(bool)), clear, SLOT(clear()));

    act = new KAction(KIcon("player_play"), i18n("&Play"), collection, "play");
    connect(act, SIGNAL(triggered(bool)), m_player, SLOT(play()));

    act = new KAction(KIcon("player_pause"), i18n("P&ause"), collection, "pause");
    connect(act, SIGNAL(triggered(bool)), m_player, SLOT(pause()));

    act = new KAction(KIcon("player_stop"), i18n("&Stop"), collection, "stop");
    connect(act, SIGNAL(triggered(bool)), m_player, SLOT(stop()));

    act = new KToolBarPopupAction(KIcon("player_start"), i18nc("previous track", "Previous"), collection, "back");
    connect(act, SIGNAL(triggered(bool)), m_player, SLOT(back()));

    act = new KAction(KIcon("player_end"), i18nc("next track", "&Next"), collection, "forward");
    connect(act, SIGNAL(triggered(bool)), m_player, SLOT(forward()));

    act = new KAction(i18n("&Loop Playlist"), collection, "loopPlaylist");
    act->setCheckable(true);

    KToggleAction *resizeColumnAction =
        new KToggleAction(i18n("&Resize Playlist Columns Manually"),
                          collection, "resizeColumnsManually");
    resizeColumnAction->setCheckedState(i18n("&Resize Column Headers Automatically"));

    // the following are not visible by default

    act = new KAction(KIcon("mute"),        i18n("Mute"),         collection, "mute");
    connect(act, SIGNAL(triggered(bool)), m_player, SLOT(mute()));

    act = new KAction(KIcon("volumeUp"),    i18n("Volume Up"),    collection, "volumeUp");
    connect(act, SIGNAL(triggered(bool)), m_player, SLOT(volumeUp()));

    act = new KAction(KIcon("volumeDown"),  i18n("Volume Down"),  collection, "volumeDown");
    connect(act, SIGNAL(triggered(bool)), m_player, SLOT(volumeDown()));

    act = new KAction(KIcon("playPause"),   i18n("Play / Pause"), collection, "playPause");
    connect(act, SIGNAL(triggered(bool)), m_player, SLOT(playPause()));

    act = new KAction(KIcon("seekForward"), i18n("Seek Forward"), collection, "seekForward");
    connect(act, SIGNAL(triggered(bool)), m_player, SLOT(seekForward()));

    act = new KAction(KIcon("seekBack"),    i18n("Seek Back"),    collection, "seekBack");
    connect(act, SIGNAL(triggered(bool)), m_player, SLOT(seekBack()));

    act = new KAction(i18n("Show / Hide"), collection, "showHide");
    connect(act, SIGNAL(triggered(bool)), this,     SLOT(slotShowHide()));

    //////////////////////////////////////////////////
    // settings menu
    //////////////////////////////////////////////////

    m_toggleSplashAction =
        new KToggleAction(i18n("Show Splash Screen on Startup"),
                          collection, "showSplashScreen");
    m_toggleSplashAction->setCheckedState(i18n("Hide Splash Screen on Startup"));
    m_toggleSystemTrayAction =
        new KToggleAction(i18n("&Dock in System Tray"),
                          collection, "toggleSystemTray");
    m_toggleDockOnCloseAction =
        new KToggleAction(i18n("&Stay in System Tray on Close"),
                          collection, "dockOnClose");
    m_togglePopupsAction =
        new KToggleAction(i18n("Popup &Track Announcement"),
                          collection, "togglePopups");
    new KToggleAction(i18n("Save &Play Queue on Exit"),
                      collection, "saveUpcomingTracks");

    connect(m_toggleSystemTrayAction, SIGNAL(toggled(bool)),
            this, SLOT(slotToggleSystemTray(bool)));


    act = new KAction(i18n("&Tag Guesser..."), collection, "tagGuesserConfig");
    connect(act, SIGNAL(triggered(bool)), SLOT(slotConfigureTagGuesser()));

    act = new KAction(i18n("&File Renamer..."), collection, "fileRenamerConfig");
    connect(act, SIGNAL(triggered(bool)), SLOT(slotConfigureFileRenamer()));

    //////////////////////////////////////////////////
    // just in the toolbar
    //////////////////////////////////////////////////

    m_sliderAction = new SliderAction(i18n("Track Position"), collection,
                                      "trackPositionAction");
}

void JuK::setupSystemTray()
{
    if(m_toggleSystemTrayAction && m_toggleSystemTrayAction->isChecked()) {
        m_systemTray = new SystemTray(this);
        m_systemTray->setObjectName( "systemTray" );
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
    m_accel = KGlobalAccel::self();

    KeyDialog::setupActionShortcut("play");
    KeyDialog::setupActionShortcut("playPause");
    KeyDialog::setupActionShortcut("stop");
    KeyDialog::setupActionShortcut("back");
    KeyDialog::setupActionShortcut("forward");
    KeyDialog::setupActionShortcut("seekBack");
    KeyDialog::setupActionShortcut("seekForward");
    KeyDialog::setupActionShortcut("volumeUp");
    KeyDialog::setupActionShortcut("volumeDown");
    KeyDialog::setupActionShortcut("mute");
    KeyDialog::setupActionShortcut("showHide");
    KeyDialog::setupActionShortcut("forwardAlbum");

    m_accel->setConfigGroup("Shortcuts");
    m_accel->readSettings();
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

    if(!dir.exists("covers"))
       dir.mkdir("covers");

    dir.cd("covers");

    if(!dir.exists("large"))
        dir.mkdir("large");
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
    m_showSplash = config.readEntry("ShowSplashScreen", true);
    m_startDocked = config.readEntry("StartDocked", false);
}

void JuK::readConfig()
{
    // player settings

    KConfigGroup playerConfig(KGlobal::config(), "Player");

    if(m_sliderAction->volumeSlider()) {
        int maxVolume = m_sliderAction->volumeSlider()->maximum();
        int volume = playerConfig.readEntry("Volume", maxVolume);
        m_sliderAction->volumeSlider()->setVolume(volume);
    }

    // Default to no random play

    ActionCollection::action<KToggleAction>("disableRandomPlay")->setChecked(true);

    QString randomPlayMode = playerConfig.readEntry("RandomPlay", "Disabled");
    if(randomPlayMode == "true" || randomPlayMode == "Normal")
        m_randomPlayAction->setChecked(true);
    else if(randomPlayMode == "AlbumRandomPlay")
        ActionCollection::action<KAction>("albumRandomPlay")->setChecked(true);

    bool loopPlaylist = playerConfig.readEntry("LoopPlaylist", false);
    ActionCollection::action<KAction>("loopPlaylist")->setChecked(loopPlaylist);

    // general settings

    KConfigGroup settingsConfig(KGlobal::config(), "Settings");

    bool dockInSystemTray = settingsConfig.readEntry("DockInSystemTray", true);
    m_toggleSystemTrayAction->setChecked(dockInSystemTray);

    bool dockOnClose = settingsConfig.readEntry("DockOnClose", true);
    m_toggleDockOnCloseAction->setChecked(dockOnClose);

    bool showPopups = settingsConfig.readEntry("TrackPopup", false);
    m_togglePopupsAction->setChecked(showPopups);

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

    KAction *a = ActionCollection::action<KAction>("loopPlaylist");
    playerConfig.writeEntry("LoopPlaylist", a->isChecked());

    a = ActionCollection::action<KAction>("albumRandomPlay");
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

    KGlobal::config()->sync();
}

bool JuK::queryExit()
{
    m_startDocked = !isVisible();

    kDebug(65432) << k_funcinfo << endl;

    hide();

    action("stop")->trigger();
    delete m_systemTray;
    m_systemTray = 0;

    CoverManager::shutdown();
    Cache::instance()->save();
    saveConfig();

    delete m_splitter;
    return true;
}

bool JuK::queryClose()
{
    kDebug(65432) << k_funcinfo << endl;

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
    setHidden(!isHidden());
}

void JuK::slotAboutToQuit()
{
    m_shuttingDown = true;
}

void JuK::slotQuit()
{
    kDebug(65432) << k_funcinfo << endl;
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
    KeyDialog::configure(ActionCollection::actions(), this);
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

// vim: set et sw=4 tw=0 sta:
