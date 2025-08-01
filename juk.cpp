/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2008, 2009, 2017 Michael Pyne <mpyne@kde.org>
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

#include <KSharedConfig>
#include <KAboutData>
#include <KActionCollection>
#include <kactionmenu.h>
#include <kconfiggroup.h>
#include <kglobalaccel.h>
#include <kmessagebox.h>
#include <knotification.h>
#include <kstandardaction.h>
#include <ktoggleaction.h>
#include <ktoolbarpopupaction.h>

#include <QAction>
#include <QActionGroup>
#include <QCoreApplication>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDir>
#include <QDirIterator>
#include <QKeyEvent>
#include <QMenuBar>
#include <QScreen>
#include <QStatusBar>
#include <QTime>
#include <QTimer>

#include "actioncollection.h"
#include "cache.h"
#include "collectionlist.h"
#include "filerenamerconfigdlg.h"
#include "iconsupport.h"
#include "keydialog.h"
#include "playermanager.h"
#include "playlistsplitter.h"
#include "scrobbleconfigdlg.h"
#include "scrobbler.h"
#include "slideraction.h"
#include "statuslabel.h"
#include "systemtray.h"
#include "tagguesserconfigdlg.h"
#include "tagtransactionmanager.h"

#include "juk_debug.h"

using namespace ActionCollection; // ""_act and others
using namespace IconSupport;      // ""_icon
using namespace Qt::Literals::StringLiterals;

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

JuK::JuK(const QStringList &filesToOpen, QWidget *parent)
  : KXmlGuiWindow(parent)
  , m_player(new PlayerManager)
  , m_filesToOpen(filesToOpen)
{
    // Expect segfaults if you change this order.

    // Allow to be passed across threads
    qRegisterMetaType<FileHandle>();
    qRegisterMetaType<FileHandleList>();

    m_instance = this;

    Cache::ensureAppDataStorageExists();

    setupActions();
    setupLayout(); // Creates PlaylistSplitter and therefore CollectionList

    bool firstRun = !KSharedConfig::openConfig()->hasGroup(u"MainWindow"_s);

    if(firstRun) {
        KConfigGroup mainWindowConfig(KSharedConfig::openConfig(), u"MainWindow"_s);
        KConfigGroup playToolBarConfig(&mainWindowConfig, u"Toolbar playToolBar"_s);
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
        const QRect screenCenter = QApplication::primaryScreen()->availableGeometry();
        r.moveCenter(screenCenter.center());
        move(r.topLeft());
    }

    connect(m_splitter, SIGNAL(guiReady()), SLOT(slotSetupSystemTray()));
    readConfig();
    setupGlobalAccels();
    activateScrobblerIfEnabled();

    QDBusInterface *pmInterface = new QDBusInterface(QStringLiteral("org.kde.Solid.PowerManagement"),
                                       QStringLiteral("/org/freedesktop/PowerManagement/Inhibit"),
                                       QStringLiteral("org.freedesktop.PowerManagement.Inhibit"),
                                       QDBusConnection::sessionBus());

    connect(m_player, &PlayerManager::signalPlay, pmInterface, [=] () {
        QDBusReply<uint> reply;
        if(pmInterface->isValid() && (m_pmToken == 0)) {
            reply = pmInterface->call(QStringLiteral("Inhibit"),
                               KAboutData::applicationData().componentName(),
                               QStringLiteral("playing audio"));
            if(reply.isValid()) {
                m_pmToken = reply.value();
            }
        }
    });

    auto uninhibitPowerManagement = [=] () {
        QDBusMessage reply;
        if(pmInterface->isValid() && (m_pmToken != 0)) {
            reply = pmInterface->call(QStringLiteral("UnInhibit"), m_pmToken);
            if(reply.errorName().isEmpty()) {
                m_pmToken = 0;
            }
        }
    };

    connect(m_player, &PlayerManager::signalPause, uninhibitPowerManagement);
    connect(m_player, &PlayerManager::signalStop, uninhibitPowerManagement);

    // KMainWindow will close the window on this signal, but we need the visible state
    // of it in saveConfig()
    disconnect(qApp, &QGuiApplication::commitDataRequest, nullptr, nullptr);

    connect(qApp, &QGuiApplication::commitDataRequest, this, [this]() { saveConfig(); },
            Qt::DirectConnection);

    // slotCheckCache loads the cached entries first to populate the collection list

    QTimer::singleShot(0, CollectionList::instance(), SLOT(startLoadingCachedItems()));
    QTimer::singleShot(0, this, SLOT(slotProcessArgs()));
}

JuK::~JuK()
{
    if(!m_shuttingDown)
        slotQuit();

    // Some items need to be deleted before others, though I haven't looked
    // at this in some time so refinement is probably possible.
    delete m_systemTray;
    delete m_splitter;
    delete m_player;
    delete m_statusLabel;

    // Sometimes KMainWindow doesn't actually call QCoreApplication::quit
    // after queryClose, even if not in a session shutdown, so make sure to
    // do so ourselves when closing the main window.
    QTimer::singleShot(0, qApp, &QCoreApplication::quit);
}

JuK* JuK::JuKInstance()
{
    return m_instance;
}

PlayerManager *JuK::playerManager() const
{
    return m_player;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void JuK::setupLayout()
{
    m_splitter = new PlaylistSplitter(m_player, this);
    setCentralWidget(m_splitter);

    m_statusLabel = new StatusLabel(*m_splitter->playlist(), statusBar());
    statusBar()->addWidget(m_statusLabel, 1);
    connect(m_player, &PlayerManager::tick, m_statusLabel,
            &StatusLabel::setItemCurrentTime);
    connect(m_player, &PlayerManager::totalTimeChanged,
            m_statusLabel, &StatusLabel::setItemTotalTime);
    connect(m_splitter, &PlaylistSplitter::currentPlaylistChanged,
            m_statusLabel, &StatusLabel::slotCurrentPlaylistHasChanged);

    m_splitter->setFocus();
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
    KStandardAction::showMenubar(menuBar(), SLOT(setVisible(bool)), collection);

    // Setup the menu which handles the random play options.
    KActionMenu *actionMenu = collection->add<KActionMenu>("actionMenu");
    actionMenu->setText(i18nc("@action:inmenu", "&Random Play"));
    actionMenu->setIcon("media-playlist-shuffle"_icon);
    actionMenu->setPopupMode(QToolButton::InstantPopup);

    QActionGroup* randomPlayGroup = new QActionGroup(this);

    QAction *act = collection->add<KToggleAction>("disableRandomPlay");
    act->setText(i18nc("@action:inmenu", "&Disable Random Play"));
    act->setIcon("go-down"_icon);
    act->setActionGroup(randomPlayGroup);
    actionMenu->addAction(act);

    m_randomPlayAction = collection->add<KToggleAction>("randomPlay");
    m_randomPlayAction->setText(i18nc("@action:inmenu", "Use &Random Play"));
    m_randomPlayAction->setIcon("media-playlist-shuffle"_icon);
    m_randomPlayAction->setActionGroup(randomPlayGroup);
    actionMenu->addAction(m_randomPlayAction);

    act = collection->add<KToggleAction>("albumRandomPlay");
    act->setText(i18nc("@action:inmenu", "Use &Album Random Play"));
    act->setIcon("media-playlist-shuffle"_icon);
    act->setActionGroup(randomPlayGroup);
    connect(act, &QAction::toggled,
            this, &JuK::slotCheckAlbumNextAction);
    actionMenu->addAction(act);

    act = collection->addAction("removeFromPlaylist", clear, SLOT(clear()));
    act->setText(i18nc("@action:inmenu", "Remove From Playlist"));
    act->setIcon("list-remove"_icon);

    act = collection->addAction("play", m_player, SLOT(play()));
    act->setText(i18nc("@action:inmenu", "&Play"));
    act->setIcon("media-playback-start"_icon);

    act = collection->addAction("pause", m_player, SLOT(pause()));
    act->setEnabled(false);
    act->setText(i18nc("@action:inmenu", "P&ause"));
    act->setIcon("media-playback-pause"_icon);

    act = collection->addAction("stop", m_player, SLOT(stop()));
    act->setEnabled(false);
    act->setText(i18nc("@action:inmenu", "&Stop"));
    act->setIcon("media-playback-stop"_icon);

    auto *popup = new KToolBarPopupAction("media-skip-backward"_icon, i18nc("@action:inmenu, previous track", "Previous" ), collection);
    popup->setEnabled(false);
    collection->addAction("back", popup);
    connect(popup, SIGNAL(triggered(bool)), m_player, SLOT(back()));

    act = collection->addAction("forward", m_player, SLOT(forward()));
    act->setEnabled(false);
    act->setText(i18nc("@action:inmenu, next track", "&Next"));
    act->setIcon("media-skip-forward"_icon);

    act = collection->addAction("loopPlaylist");
    act->setText(i18nc("@action:inmenu", "&Loop Playlist"));
    act->setCheckable(true);

    act = collection->add<KToggleAction>("resizeColumnsManually");
    act->setText(i18nc("@action:inmenu", "&Resize Playlist Columns Manually"));

    // the following are not visible by default

    act = collection->addAction("mute", m_player, SLOT(mute()));
    act->setText(i18nc("@action:inmenu, silence playback", "Mute"));
    act->setIcon("audio-volume-muted"_icon);

    act = collection->addAction("volumeUp", m_player, SLOT(volumeUp()));
    act->setText(i18nc("@action:inmenu", "Volume Up"));
    act->setIcon("audio-volume-high"_icon);

    act = collection->addAction("volumeDown", m_player, SLOT(volumeDown()));
    act->setText(i18nc("@action:inmenu", "Volume Down"));
    act->setIcon("audio-volume-low"_icon);

    act = collection->addAction("playPause", m_player, SLOT(playPause()));
    act->setText(i18nc("@action:inmenu", "Play / Pause"));
    act->setIcon("media-playback-start"_icon);

    act = collection->addAction("seekForward", m_player, SLOT(seekForward()));
    act->setText(i18nc("@action:inmenu", "Seek Forward"));
    act->setIcon("media-seek-forward"_icon);

    act = collection->addAction("seekBack", m_player, SLOT(seekBack()));
    act->setText(i18nc("@action:inmenu", "Seek Back"));
    act->setIcon("media-seek-backward"_icon);

    act = collection->addAction("showHide", this, SLOT(slotShowHide()));
    act->setText(i18nc("@action:inmenu", "Show / Hide"));

    //////////////////////////////////////////////////
    // settings menu
    //////////////////////////////////////////////////

    m_toggleSystemTrayAction = collection->add<KToggleAction>("toggleSystemTray");
    m_toggleSystemTrayAction->setText(i18nc("@action:inmenu", "&Dock in System Tray"));
    connect(m_toggleSystemTrayAction, SIGNAL(triggered(bool)), SLOT(slotToggleSystemTray(bool)));

    m_toggleDockOnCloseAction = collection->add<KToggleAction>("dockOnClose");
    m_toggleDockOnCloseAction->setText(i18nc("@action:inmenu", "&Stay in System Tray on Close"));

    m_togglePopupsAction = collection->add<KToggleAction>("togglePopups");
    m_togglePopupsAction->setText(i18nc("@action:inmenu", "Popup &Track Announcement"));

    act = collection->add<KToggleAction>("saveUpcomingTracks");
    act->setText(i18nc("@action:inmenu", "Save &Play Queue on Exit"));

    act = collection->addAction("tagGuesserConfig", this, SLOT(slotConfigureTagGuesser()));
    act->setText(i18nc("@action:inmenu", "&Tag Guesser…"));

    act = collection->addAction("fileRenamerConfig", this, SLOT(slotConfigureFileRenamer()));
    act->setText(i18nc("@action:inmenu", "&File Renamer…"));

    act = collection->addAction("scrobblerConfig", this, SLOT(slotConfigureScrobbling()));
    act->setText(i18nc("@action:inmenu", "&Configure Scrobbling…"));

    //////////////////////////////////////////////////
    // just in the toolbar
    //////////////////////////////////////////////////

    collection->addAction("trackPositionAction",
                          new TrackPositionAction(i18nc("@action:button", "Track Position"), this));
    collection->addAction("volumeAction",
                          new VolumeAction(i18nc("@action:button", "Volume"), this));

    ActionCollection::actions()->addAssociatedWidget(this);

    const auto actList = ActionCollection::actions()->actions();
    for(QAction* action : actList) {
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    }
}

void JuK::slotSetupSystemTray()
{
    if(m_toggleSystemTrayAction && m_toggleSystemTrayAction->isChecked()) {
        m_systemTray = new SystemTray(m_player, this);
        m_systemTray->setObjectName(QStringLiteral("systemTray"));

        m_toggleDockOnCloseAction->setEnabled(true);
        m_togglePopupsAction->setEnabled(true);

        // since we need the information if the main window is visible
        // when quit was selected, we need to reroute the signal to our slot
        QAction *quitAction = m_systemTray->action(QStringLiteral("quit"));
        if (quitAction) {
            quitAction->disconnect();
            connect(quitAction, &QAction::triggered, this, &JuK::slotQuit);
        }

        // If this flag gets set then JuK will quit if you click the cover on
        // the track announcement popup when JuK is only in the system tray
        // (the systray has no widget).

        qGuiApp->setQuitOnLastWindowClosed(false);
    }
    else {
        m_systemTray = nullptr;
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

void JuK::keyPressEvent(QKeyEvent *e)
{
    if (e->key() >= Qt::Key_Back && e->key() <= Qt::Key_MediaLast)
        e->accept();
    KXmlGuiWindow::keyPressEvent(e);
}

void JuK::readConfig()
{
    // player settings

    KConfigGroup playerConfig(KSharedConfig::openConfig(), u"Player"_s);

    const int maxVolume = 100;
    const int volume = playerConfig.readEntry("Volume", maxVolume);
    m_player->setVolume(volume * 0.01);

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

    KConfigGroup settingsConfig(KSharedConfig::openConfig(), u"Settings"_s);

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

    KConfigGroup playerConfig(KSharedConfig::openConfig(), u"Player"_s);

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

    KConfigGroup settingsConfig(KSharedConfig::openConfig(), u"Settings"_s);
    settingsConfig.writeEntry("StartDocked", !isVisible() && m_toggleDockOnCloseAction->isChecked());
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
            i18nc("@title:window", "Dock in System Tray"), "hideOnCloseInfo");

        showMinimized(); // Avoid confusing Qt / KWin
        hide();          // Remove icon in taskbar
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////
// private slot definitions
////////////////////////////////////////////////////////////////////////////////

void JuK::slotShowHide()
{
    setHidden(!isHidden());
}

void JuK::slotQuit()
{
    m_shuttingDown = true;

    // Some phonon backends will crash on shutdown unless we've stopped
    // playback.
    if(m_player->playing()) {
        m_player->stop();
    }

    saveConfig();

    // this will start chain of events causing PlaylistCollection (in
    // guise of PlaylistBox) and CollectionList (as first Playlist child)
    // to save themselves and then quit the application when this widget
    // closes.
    delete m_statusLabel;
    m_statusLabel = nullptr;

    delete m_splitter;
    m_splitter = nullptr;

    close();
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
        m_systemTray = nullptr;
        m_toggleDockOnCloseAction->setEnabled(false);
        m_togglePopupsAction->setEnabled(false);

        qGuiApp->setQuitOnLastWindowClosed(true);
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
    action(u"forwardAlbum"_s)->setEnabled(m_player->playing() && albumRandomEnabled);
}

// vim: set et sw=4 tw=0 sta:
