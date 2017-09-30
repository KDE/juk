/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2008, 2009 Michael Pyne <mpyne@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "juk.h"

#include <kstatusbar.h>
#include <kmessagebox.h>
#include <kactioncollection.h>
#include <kstandardaction.h>
#include <ktoggleaction.h>
#include <kactionmenu.h>
#include <kconfiggroup.h>
#include <kglobal.h>
#include <kglobalaccel.h>
#include <ktoolbarpopupaction.h>
#include <knotification.h>
#include <kdeversion.h>

#include <QIcon>
#include <QAction>
#include <QCoreApplication>
#include <QKeyEvent>
#include <QDir>
#include <QDirIterator>
#include <QTime>
#include <QTimer>
#include <QDesktopWidget>

#include "slideraction.h"
#include "statuslabel.h"
#include "systemtray.h"
#include "keydialog.h"
#include "tagguesserconfigdlg.h"
#include "filerenamerconfigdlg.h"
#include "scrobbler.h"
#include "scrobbleconfigdlg.h"
#include "actioncollection.h"
#include "cache.h"
#include "playlistsplitter.h"
#include "collectionlist.h"
#include "covermanager.h"
#include "tagtransactionmanager.h"
#include "juk_debug.h"

using namespace ActionCollection;

JuK* JuK::m_instance;

template<class T>
void deleteAndClear(T *&ptr)
{
    delete ptr;
    ptr = 0;
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

JuK::JuK(const QStringList &filesToOpen, QWidget *parent) :
    KXmlGuiWindow(parent, Qt::WindowFlags(Qt::WA_DeleteOnClose)),
    m_splitter(nullptr),
    m_statusLabel(nullptr),
    m_systemTray(nullptr),
    m_player(new PlayerManager),
    m_scrobbler(nullptr),
    m_filesToOpen(filesToOpen),
    m_shuttingDown(false)
{
    // Expect segfaults if you change this order.

    m_instance = this;

    readSettings();

    Cache::ensureAppDataStorageExists();

    setupActions();
    setupLayout();

    bool firstRun = !KSharedConfig::openConfig()->hasGroup("MainWindow");

    if(firstRun) {
        KConfigGroup mainWindowConfig(KSharedConfig::openConfig(), "MainWindow");
        KConfigGroup playToolBarConfig(&mainWindowConfig, "Toolbar playToolBar");
        playToolBarConfig.writeEntry("ToolButtonStyle", "IconOnly");
    }

    QSize defaultSize(800, 480);

    if(QApplication::isRightToLeft())
        setupGUI(defaultSize, ToolBar | Save | Create, "jukui-rtl.rc");
    else
        setupGUI(defaultSize, ToolBar | Save | Create);

    // Center the GUI if this is our first run ever.

    if(firstRun) {
        QRect r = rect();
        r.moveCenter(QApplication::desktop()->screenGeometry().center());
        move(r.topLeft());
    }

    connect(m_splitter, SIGNAL(guiReady()), SLOT(slotSetupSystemTray()));
    readConfig();
    setupGlobalAccels();
    activateScrobblerIfEnabled();

    connect(QCoreApplication::instance(), SIGNAL(aboutToQuit()), SLOT(slotAboutToQuit()));

    // slotCheckCache loads the cached entries first to populate the collection list

    QTimer::singleShot(0, this, SLOT(slotClearOldCovers()));
    QTimer::singleShot(0, CollectionList::instance(), SLOT(startLoadingCachedItems()));
    QTimer::singleShot(0, this, SLOT(slotProcessArgs()));
}

JuK::~JuK()
{
}

JuK* JuK::JuKInstance()
{
    return m_instance;
}

PlayerManager *JuK::playerManager() const
{
    return m_player;
}

void JuK::coverDownloaded(const QPixmap &cover)
{
    QString event(cover.isNull() ? "coverFailed" : "coverDownloaded");
    KNotification *notification = new KNotification(event, this);
    notification->setPixmap(cover);
    notification->setFlags(KNotification::CloseOnTimeout);

    if(cover.isNull())
        notification->setText(i18n("Your album art failed to download."));
    else
        notification->setText(i18n("Your album art has finished downloading."));

    notification->sendEvent();
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void JuK::setupLayout()
{
    new TagTransactionManager(this);

    qCDebug(JUK_LOG) << "Creating GUI";
    QTime stopwatch;
    stopwatch.start();

    m_splitter = new PlaylistSplitter(m_player, this);
    setCentralWidget(m_splitter);

    m_statusLabel = new StatusLabel(m_splitter->playlist(), statusBar());
    connect(CollectionList::instance(), SIGNAL(signalCollectionChanged()),
            m_statusLabel, SLOT(updateData()));
    statusBar()->addWidget(m_statusLabel, 1);
    m_player->setStatusLabel(m_statusLabel);

    m_splitter->setFocus();

    qCDebug(JUK_LOG) << "GUI created in" << stopwatch.elapsed() << "ms";
}

void JuK::setupActions()
{
    KActionCollection *collection = ActionCollection::actions();

    // Setup KDE standard actions that JuK uses.

    KStandardAction::quit(this, SLOT(slotQuit()), collection);
    KStandardAction::undo(this, SLOT(slotUndo()), collection);
    KStandardAction::cut(collection);
    KStandardAction::copy(collection);
    KStandardAction::paste(collection);
    QAction *clear = KStandardAction::clear(collection);
    KStandardAction::selectAll(collection);
    KStandardAction::keyBindings(this, SLOT(slotEditKeys()), collection);

    // Setup the menu which handles the random play options.
    KActionMenu *actionMenu = collection->add<KActionMenu>("actionMenu");
    actionMenu->setText(i18n("&Random Play"));
    actionMenu->setIcon(QIcon::fromTheme( QLatin1String( "media-playlist-shuffle" )));
    actionMenu->setDelayed(false);

    QActionGroup* randomPlayGroup = new QActionGroup(this);

    QAction *act = collection->add<KToggleAction>("disableRandomPlay");
    act->setText(i18n("&Disable Random Play"));
    act->setIcon(QIcon::fromTheme( QLatin1String( "go-down" )));
    act->setActionGroup(randomPlayGroup);
    actionMenu->addAction(act);

    m_randomPlayAction = collection->add<KToggleAction>("randomPlay");
    m_randomPlayAction->setText(i18n("Use &Random Play"));
    m_randomPlayAction->setIcon(QIcon::fromTheme( QLatin1String( "media-playlist-shuffle" )));
    m_randomPlayAction->setActionGroup(randomPlayGroup);
    actionMenu->addAction(m_randomPlayAction);

    act = collection->add<KToggleAction>("albumRandomPlay");
    act->setText(i18n("Use &Album Random Play"));
    act->setIcon(QIcon::fromTheme( QLatin1String( "media-playlist-shuffle" )));
    act->setActionGroup(randomPlayGroup);
    connect(act, SIGNAL(triggered(bool)), SLOT(slotCheckAlbumNextAction(bool)));
    actionMenu->addAction(act);

    act = collection->addAction("removeFromPlaylist", clear, SLOT(clear()));
    act->setText(i18n("Remove From Playlist"));
    act->setIcon(QIcon::fromTheme( QLatin1String( "list-remove" )));

    act = collection->addAction("play", m_player, SLOT(play()));
    act->setText(i18n("&Play"));
    act->setIcon(QIcon::fromTheme( QLatin1String( "media-playback-start" )));

    act = collection->addAction("pause", m_player, SLOT(pause()));
    act->setText(i18n("P&ause"));
    act->setIcon(QIcon::fromTheme( QLatin1String( "media-playback-pause" )));

    act = collection->addAction("stop", m_player, SLOT(stop()));
    act->setText(i18n("&Stop"));
    act->setIcon(QIcon::fromTheme( QLatin1String( "media-playback-stop" )));

    act = new KToolBarPopupAction(QIcon::fromTheme( QLatin1String( "media-skip-backward") ), i18nc("previous track", "Previous" ), collection);
    collection->addAction("back", act);
    connect(act, SIGNAL(triggered(bool)), m_player, SLOT(back()));

    act = collection->addAction("forward", m_player, SLOT(forward()));
    act->setText(i18nc("next track", "&Next"));
    act->setIcon(QIcon::fromTheme( QLatin1String( "media-skip-forward" )));

    act = collection->addAction("loopPlaylist");
    act->setText(i18n("&Loop Playlist"));
    act->setCheckable(true);

    act = collection->add<KToggleAction>("resizeColumnsManually");
    act->setText(i18n("&Resize Playlist Columns Manually"));

    // the following are not visible by default

    act = collection->addAction("mute", m_player, SLOT(mute()));
    act->setText(i18nc("silence playback", "Mute"));
    act->setIcon(QIcon::fromTheme( QLatin1String( "audio-volume-muted" )));

    act = collection->addAction("volumeUp", m_player, SLOT(volumeUp()));
    act->setText(i18n("Volume Up"));
    act->setIcon(QIcon::fromTheme( QLatin1String( "audio-volume-high" )));

    act = collection->addAction("volumeDown", m_player, SLOT(volumeDown()));
    act->setText(i18n("Volume Down"));
    act->setIcon(QIcon::fromTheme( QLatin1String( "audio-volume-low" )));

    act = collection->addAction("playPause", m_player, SLOT(playPause()));
    act->setText(i18n("Play / Pause"));
    act->setIcon(QIcon::fromTheme( QLatin1String( "media-playback-start" )));

    act = collection->addAction("seekForward", m_player, SLOT(seekForward()));
    act->setText(i18n("Seek Forward"));
    act->setIcon(QIcon::fromTheme( QLatin1String( "media-seek-forward" )));

    act = collection->addAction("seekBack", m_player, SLOT(seekBack()));
    act->setText(i18n("Seek Back"));
    act->setIcon(QIcon::fromTheme( QLatin1String( "media-seek-backward" )));

    act = collection->addAction("showHide", this, SLOT(slotShowHide()));
    act->setText(i18n("Show / Hide"));

    //////////////////////////////////////////////////
    // settings menu
    //////////////////////////////////////////////////

    m_toggleSystemTrayAction = collection->add<KToggleAction>("toggleSystemTray");
    m_toggleSystemTrayAction->setText(i18n("&Dock in System Tray"));
    connect(m_toggleSystemTrayAction, SIGNAL(triggered(bool)), SLOT(slotToggleSystemTray(bool)));

    m_toggleDockOnCloseAction = collection->add<KToggleAction>("dockOnClose");
    m_toggleDockOnCloseAction->setText(i18n("&Stay in System Tray on Close"));

    m_togglePopupsAction = collection->add<KToggleAction>("togglePopups");
    m_togglePopupsAction->setText(i18n("Popup &Track Announcement"));

    act = collection->add<KToggleAction>("saveUpcomingTracks");
    act->setText(i18n("Save &Play Queue on Exit"));

    act = collection->addAction("tagGuesserConfig", this, SLOT(slotConfigureTagGuesser()));
    act->setText(i18n("&Tag Guesser..."));

    act = collection->addAction("fileRenamerConfig", this, SLOT(slotConfigureFileRenamer()));
    act->setText(i18n("&File Renamer..."));

    act = collection->addAction("scrobblerConfig", this, SLOT(slotConfigureScrobbling()));
    act->setText(i18n("&Configure scrobbling..."));

    //////////////////////////////////////////////////
    // just in the toolbar
    //////////////////////////////////////////////////

    collection->addAction("trackPositionAction",
                          new TrackPositionAction(i18n("Track Position"), this));
    collection->addAction("volumeAction",
                          new VolumeAction(i18n("Volume"), this));

    ActionCollection::actions()->addAssociatedWidget(this);
    foreach (QAction* action, ActionCollection::actions()->actions())
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
}

void JuK::slotSetupSystemTray()
{
    if(m_toggleSystemTrayAction && m_toggleSystemTrayAction->isChecked()) {
        qCDebug(JUK_LOG) << "Setting up systray";
        QTime stopwatch; stopwatch.start();
        m_systemTray = new SystemTray(m_player, this);
        m_systemTray->setObjectName( QLatin1String("systemTray" ));

        m_toggleDockOnCloseAction->setEnabled(true);
        m_togglePopupsAction->setEnabled(true);
        qCDebug(JUK_LOG) << "Finished setting up systray, took" << stopwatch.elapsed() << "ms";
    }
    else {
        m_systemTray = 0;
        m_toggleDockOnCloseAction->setEnabled(false);
        m_togglePopupsAction->setEnabled(false);
    }
}

void JuK::setupGlobalAccels()
{
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
}

void JuK::slotProcessArgs()
{
    CollectionList::instance()->addFiles(m_filesToOpen);
}

void JuK::slotClearOldCovers()
{
    // Find all saved covers from the previous run of JuK and clear them out, in case
    // we find our tracks in a different order this run, which would cause old saved
    // covers to be wrong.
    // See mpris2/mediaplayer2player.cpp
    QString tmpDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QStringList nameFilters;

    nameFilters << QStringLiteral("juk-cover-*.png");
    QDirIterator jukCoverIter(tmpDir, nameFilters);
    while (jukCoverIter.hasNext()) {
        QFile::remove(jukCoverIter.next());
    }
}

void JuK::keyPressEvent(QKeyEvent *e)
{
    if (e->key() >= Qt::Key_Back && e->key() <= Qt::Key_MediaLast)
        e->accept();
    KXmlGuiWindow::keyPressEvent(e);
}

/**
 * These are settings that need to be know before setting up the GUI.
 */

void JuK::readSettings()
{
    KConfigGroup config(KSharedConfig::openConfig(), "Settings");
    m_startDocked = config.readEntry("StartDocked", false);
}

void JuK::readConfig()
{
    // player settings

    KConfigGroup playerConfig(KSharedConfig::openConfig(), "Player");

    if(m_player)
    {
        const int maxVolume = 100;
        const int volume = playerConfig.readEntry("Volume", maxVolume);
        m_player->setVolume(volume * 0.01);

        //bool enableCrossfade = playerConfig.readEntry("CrossfadeTracks", true);
        //m_player->setCrossfadeEnabled(enableCrossfade);
        //ActionCollection::action<QAction>("crossfadeTracks")->setChecked(enableCrossfade);
    }

    // Default to no random play

    ActionCollection::action<KToggleAction>("disableRandomPlay")->setChecked(true);

    QString randomPlayMode = playerConfig.readEntry("RandomPlay", "Disabled");
    if(randomPlayMode == "true" || randomPlayMode == "Normal")
        m_randomPlayAction->setChecked(true);
    else if(randomPlayMode == "AlbumRandomPlay")
        ActionCollection::action<QAction>("albumRandomPlay")->setChecked(true);

    bool loopPlaylist = playerConfig.readEntry("LoopPlaylist", false);
    ActionCollection::action<QAction>("loopPlaylist")->setChecked(loopPlaylist);

    // general settings

    KConfigGroup settingsConfig(KSharedConfig::openConfig(), "Settings");

    bool dockInSystemTray = settingsConfig.readEntry("DockInSystemTray", true);
    m_toggleSystemTrayAction->setChecked(dockInSystemTray);

    bool dockOnClose = settingsConfig.readEntry("DockOnClose", true);
    m_toggleDockOnCloseAction->setChecked(dockOnClose);

    bool showPopups = settingsConfig.readEntry("TrackPopup", false);
    m_togglePopupsAction->setChecked(showPopups);
}

void JuK::saveConfig()
{
    // player settings

    KConfigGroup playerConfig(KSharedConfig::openConfig(), "Player");

    if (m_player)
    {
        playerConfig.writeEntry("Volume", static_cast<int>(100.0 * m_player->volume()));
    }

    playerConfig.writeEntry("RandomPlay", m_randomPlayAction->isChecked());

    QAction *a = ActionCollection::action<QAction>("loopPlaylist");
    playerConfig.writeEntry("LoopPlaylist", a->isChecked());

    playerConfig.writeEntry("CrossfadeTracks", false); // TODO bring back

    a = ActionCollection::action<QAction>("albumRandomPlay");
    if(a->isChecked())
        playerConfig.writeEntry("RandomPlay", "AlbumRandomPlay");
    else if(m_randomPlayAction->isChecked())
        playerConfig.writeEntry("RandomPlay", "Normal");
    else
        playerConfig.writeEntry("RandomPlay", "Disabled");

    // general settings

    KConfigGroup settingsConfig(KSharedConfig::openConfig(), "Settings");
    settingsConfig.writeEntry("StartDocked", m_startDocked);
    settingsConfig.writeEntry("DockInSystemTray", m_toggleSystemTrayAction->isChecked());
    settingsConfig.writeEntry("DockOnClose", m_toggleDockOnCloseAction->isChecked());
    settingsConfig.writeEntry("TrackPopup", m_togglePopupsAction->isChecked());

    KSharedConfig::openConfig()->sync();
}

bool JuK::queryClose()
{
    if(!m_shuttingDown &&
       !qApp->isSavingSession() &&
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
    {
        // Some phonon backends will crash on shutdown unless we've stopped
        // playback.
        if(m_player->playing())
            m_player->stop();

        // Save configuration data.
        m_startDocked = !isVisible();
        saveConfig();
        return true;
    }
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

    deleteAndClear(m_systemTray);
    deleteAndClear(m_splitter);
    deleteAndClear(m_player);
    deleteAndClear(m_statusLabel);
}

void JuK::slotQuit()
{
    m_shuttingDown = true;

    qApp->quit();
}

////////////////////////////////////////////////////////////////////////////////
// settings menu
////////////////////////////////////////////////////////////////////////////////

void JuK::slotToggleSystemTray(bool enabled)
{
    if(enabled && !m_systemTray)
        slotSetupSystemTray();
    else if(!enabled && m_systemTray) {
        delete m_systemTray;
        m_systemTray = 0;
        m_toggleDockOnCloseAction->setEnabled(false);
        m_togglePopupsAction->setEnabled(false);
    }
}

void JuK::slotEditKeys()
{
    KeyDialog(ActionCollection::actions(), this).configure();
}

void JuK::slotConfigureTagGuesser()
{
    TagGuesserConfigDlg(this).exec();
}

void JuK::slotConfigureFileRenamer()
{
    FileRenamerConfigDlg(this).exec();
}

void JuK::slotConfigureScrobbling()
{
    ScrobbleConfigDlg(this).exec();
    activateScrobblerIfEnabled();
}

void JuK::activateScrobblerIfEnabled()
{
    bool isScrobbling = Scrobbler::isScrobblingEnabled();

    if (!m_scrobbler && isScrobbling) {
        m_scrobbler = new Scrobbler(this);
        connect (m_player,    SIGNAL(signalItemChanged(FileHandle)),
                 m_scrobbler, SLOT(nowPlaying(FileHandle)));
    }
    else if (m_scrobbler && !isScrobbling) {
        delete m_scrobbler;
        m_scrobbler = 0;
    }
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

// vim: set et sw=4 tw=0 sta:
