/***************************************************************************
                          juk.cpp  -  description
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

#include <kcmdlineargs.h>
#include <kstatusbar.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <qmetaobject.h>
#include <qslider.h>

#include "juk.h"
#include "slideraction.h"
#include "statuslabel.h"
#include "splashscreen.h"
#include "genrelisteditor.h"
#include "systemtray.h"
#include "keydialog.h"
#include "tagguesserconfigdlg.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

JuK::JuK(QWidget *parent, const char *name) : KMainWindow(parent, name, WDestructiveClose),
					      DCOPObject("Player"),
					      m_shuttingDown(false)
{
    // Expect segfaults if you change this order.

    readSettings();

    if(m_showSplash) {
	SplashScreen::instance()->show();
	kapp->processEvents();
    }

    setupActions();
    setupLayout();
    setupSplitterConnections();
    slotPlaylistChanged();
    readConfig();
    setupPlayer();
    setupSystemTray();
    setupGlobalAccels();
    processArgs();

    SplashScreen::finishedLoading();
    QTimer::singleShot(0, CollectionList::instance(), SLOT(slotCheckCache()));
}

JuK::~JuK()
{

}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void JuK::setupLayout()
{
    m_splitter = new PlaylistSplitter(this, m_restore, "playlistSplitter");
    setCentralWidget(m_splitter);

    // playlist item activation connection
    connect(m_splitter, SIGNAL(signalDoubleClicked()), this, SLOT(slotPlaySelectedFile()));
    connect(m_splitter, SIGNAL(signalListBoxDoubleClicked()), this, SLOT(startPlayingPlaylist()));

    // create status bar
    m_statusLabel = new StatusLabel(statusBar());
    statusBar()->addWidget(m_statusLabel, 1);

    connect(m_splitter, SIGNAL(signalSelectedPlaylistCountChanged(int)), m_statusLabel, SLOT(setPlaylistCount(int)));
    connect(m_statusLabel, SIGNAL(jumpButtonClicked()), m_splitter, SLOT(slotSelectPlaying()));

    m_splitter->setFocus();

    resize(750, 500);
}

void JuK::setupActions()
{
    //////////////////////////////////////////////////
    // file menu
    //////////////////////////////////////////////////

    createSplitterAction(i18n("New Playlist..."),    SLOT(slotCreatePlaylist()),    "file_new",           "filenew");
    createSplitterAction(i18n("Open..."),            SLOT(slotOpen()),              "file_open",          "fileopen");
    createSplitterAction(i18n("Open &Directory..."), SLOT(slotOpenDirectory()),     "openDirectory",      "fileopen");
    createSplitterAction(i18n("&Rename..."),         SLOT(slotRenamePlaylist()),    "renamePlaylist");
    createSplitterAction(i18n("D&uplicate..."),      SLOT(slotDuplicatePlaylist()), "duplicatePlaylist");
    createSplitterAction(i18n("Save"),               SLOT(slotSavePlaylist()),      "file_save",          "filesave");
    createSplitterAction(i18n("Save As..."),         SLOT(slotSaveAsPlaylist()),    "file_save_as",       "filesaveas");
    createSplitterAction(i18n("R&emove"),            SLOT(slotDeletePlaylist()),    "deleteItemPlaylist", "edittrash");

    KStdAction::quit(this, SLOT(slotQuit()), actionCollection());

    //////////////////////////////////////////////////
    // edit menu
    //////////////////////////////////////////////////

    KStdAction::cut(this,   SLOT(cut()),   actionCollection());
    KStdAction::copy(this,  SLOT(copy()),  actionCollection());
    KStdAction::paste(this, SLOT(paste()), actionCollection());

    new KAction(i18n("C&lear"), "editclear", 0, this, SLOT(clear()), actionCollection(), "clear");

    KStdAction::selectAll(this, SLOT(selectAll()), actionCollection());

    //////////////////////////////////////////////////
    // view menu
    //////////////////////////////////////////////////

    m_showSearchAction = new KToggleAction(i18n("Show &Search Bar"), "filefind", 0, actionCollection(), "showSearch");
    m_showEditorAction = new KToggleAction(i18n("Show &Tag Editor"), "edit",     0, actionCollection(), "showEditor");

    createSplitterAction(i18n("Refresh Items"), SLOT(slotRefresh()), "refresh", "reload");

    //////////////////////////////////////////////////
    // play menu
    //////////////////////////////////////////////////

    m_randomPlayAction = new KToggleAction(i18n("&Random Play"), 0, actionCollection(), "randomPlay");

    new KAction(i18n("&Play"),  "player_play",  0, this, SLOT(play()),  actionCollection(), "play");
    new KAction(i18n("P&ause"), "player_pause", 0, this, SLOT(pause()), actionCollection(), "pause");
    new KAction(i18n("&Stop"),  "player_stop",  0, this, SLOT(stop()),  actionCollection(), "stop");

    m_backAction = new KToolBarPopupAction(i18n("Skip &Back"), "player_start", 0, this, SLOT(back()), actionCollection(), "back");
    connect(m_backAction->popupMenu(), SIGNAL(aboutToShow()),  this, SLOT(slotPopulateBackMenu()));
    connect(m_backAction->popupMenu(), SIGNAL(activated(int)), this, SLOT(back(int)));

    new KAction(i18n("Skip &Forward"), "player_end", 0, this, SLOT(forward()), actionCollection(), "forward");

    m_loopPlaylistAction = new KToggleAction(i18n("&Loop Playlist"), 0, 0, actionCollection(), "loopPlaylist");

    //////////////////////////////////////////////////
    // tagger menu
    //////////////////////////////////////////////////

    createSplitterAction(i18n("&Save"),   SLOT(slotSaveTag()),             "saveItem",   "filesave", "CTRL+t");
    createSplitterAction(i18n("&Delete"), SLOT(slotDeleteSelectedItems()), "removeItem", "editdelete");

    KActionMenu *guessMenu = new KActionMenu(i18n("&Guess Tag Information"), "", actionCollection(), "guessTag");

    guessMenu->insert(
        createSplitterAction(i18n("From &Filename"), SLOT(slotGuessTagInfoFile()),     "guessTagFile",     0, "CTRL+f"));
    guessMenu->insert(
        createSplitterAction(i18n("From &Internet"), SLOT(slotGuessTagInfoInternet()), "guessTagInternet", 0, "CTRL+i"));

    // new KAction(i18n("&Rename File"), 0, "CTRL+r", m_splitter, SLOT(slotRenameFile()), actionCollection(), "renameFile"); // 4

    //////////////////////////////////////////////////
    // settings menu
    //////////////////////////////////////////////////

    new KToggleAction(i18n("Show Menu Bar"), "CTRL+m", this, SLOT(slotToggleMenuBar()), actionCollection(), "toggleMenuBar");

    setStandardToolBarMenuEnabled(true);

    m_restoreOnLoadAction     = new KToggleAction(i18n("&Restore Playlists on Load"),    0, actionCollection(), "restoreOnLoad");
    m_toggleSplashAction      = new KToggleAction(i18n("Show Splash Screen on Startup"), 0, actionCollection(), "showSplashScreen");
    m_toggleSystemTrayAction  = new KToggleAction(i18n("&Dock in System Tray"),          0, actionCollection(), "toggleSystemTray");
    m_toggleDockOnCloseAction = new KToggleAction(i18n("&Stay in System Tray on Close"), 0, actionCollection(), "dockOnClose");
    m_togglePopupsAction      = new KToggleAction(i18n("&Popup Track Announcement"),     0, this, 0, actionCollection(), "togglePopups");

    connect(m_toggleSystemTrayAction, SIGNAL(toggled(bool)), this, SLOT(slotToggleSystemTray(bool)));


    new KAction(i18n("Genre List Editor..."), 0, this, SLOT(slotShowGenreListEditor()), actionCollection(), "showGenreListEditor");

    m_outputSelectAction = Player::playerSelectAction(actionCollection());

    if(m_outputSelectAction) {
        m_outputSelectAction->setCurrentItem(0);
        connect(m_outputSelectAction, SIGNAL(activated(int)), this, SLOT(slotSetOutput(int)));
    }

    new KAction(i18n("&Tag Guesser..."), 0, 0, this, SLOT(slotConfigureTagGuesser()), actionCollection(), "tagGuesserConfig");

    KStdAction::keyBindings(this, SLOT(slotEditKeys()), actionCollection());

    //////////////////////////////////////////////////
    // just in the toolbar
    //////////////////////////////////////////////////

    m_sliderAction = new SliderAction(i18n("Track Position"), actionCollection(), "trackPositionAction");

    createGUI();

    // set the slider to the proper orientation and make it stay that way
    m_sliderAction->slotUpdateOrientation();
}

void JuK::setupSplitterConnections()
{
    QValueListConstIterator<SplitterConnection> it = m_splitterConnections.begin();
    for(; it != m_splitterConnections.end(); ++it)
        connect((*it).first, SIGNAL(activated()), m_splitter, (*it).second);

    actionCollection()->insert(m_splitter->columnVisibleAction());

    connect(m_showSearchAction, SIGNAL(toggled(bool)), m_splitter, SLOT(slotSetSearchVisible(bool)));
    connect(m_showEditorAction, SIGNAL(toggled(bool)), m_splitter, SLOT(slotSetEditorVisible(bool)));
    connect(this, SIGNAL(dockWindowPositionChanged(QDockWindow *)), m_sliderAction, SLOT(slotUpdateOrientation(QDockWindow *)));
    connect(m_splitter, SIGNAL(signalPlaylistChanged()), this, SLOT(slotPlaylistChanged()));
}

void JuK::setupSystemTray()
{
    if(m_toggleSystemTrayAction && m_toggleSystemTrayAction->isChecked()) {
        m_systemTray = new SystemTray(this, "systemTray");
        m_systemTray->show();

        connect(this, SIGNAL(signalNewSong(const QString&)), m_systemTray, SLOT(slotNewSong(const QString&)));

        if(m_player && m_player->paused())
            m_systemTray->slotPause();
        else if(m_player && m_player->playing())
            m_systemTray->slotPlay();

        m_toggleDockOnCloseAction->setEnabled(true);

        connect(m_systemTray, SIGNAL(quitSelected()), this, SLOT(slotQuit()));
    }
    else {
        m_systemTray = 0;
        m_toggleDockOnCloseAction->setEnabled(false);
    }
}

void JuK::setupPlayer()
{
    m_trackPositionDragging = false;
    m_noSeek = false;
    m_muted = false;
    actionCollection()->action("pause")->setEnabled(false);
    actionCollection()->action("stop")->setEnabled(false);
    actionCollection()->action("back")->setEnabled(false);
    actionCollection()->action("forward")->setEnabled(false);

    m_playTimer = new QTimer(this);
    connect(m_playTimer, SIGNAL(timeout()), this, SLOT(slotPollPlay()));

    if(m_sliderAction && m_sliderAction->getTrackPositionSlider() && m_sliderAction->getVolumeSlider()) {
        connect(m_sliderAction->getTrackPositionSlider(), SIGNAL(valueChanged(int)), this, SLOT(slotTrackPositionSliderUpdate(int)));
        connect(m_sliderAction->getTrackPositionSlider(), SIGNAL(sliderPressed()), this, SLOT(slotTrackPositionSliderClicked()));
        connect(m_sliderAction->getTrackPositionSlider(), SIGNAL(sliderReleased()), this, SLOT(slotTrackPositionSliderReleased()));
        m_sliderAction->getTrackPositionSlider()->setEnabled(false);

        connect(m_sliderAction->getVolumeSlider(), SIGNAL(valueChanged(int)), this, SLOT(setVolume(int)));
    }

    int playerType = 0;
    if(m_outputSelectAction) {
        playerType = m_outputSelectAction->currentItem();
        connect(m_outputSelectAction, SIGNAL(activated(int)), this, SLOT(slotSetOutput(int)));
    }

    m_player = Player::createPlayer(playerType);
}


void JuK::setupGlobalAccels()
{
    m_accel = new KGlobalAccel(this);
    KeyDialog::insert(m_accel, "PlayPause",  i18n("Play/Pause"),        this, SLOT(playPause()));
    KeyDialog::insert(m_accel, "Stop",       i18n("Stop Playing"),      this, SLOT(stop()));
    KeyDialog::insert(m_accel, "Back",       i18n("Back"),              this, SLOT(back()));
    KeyDialog::insert(m_accel, "Forward",    i18n("Forward"),           this, SLOT(forward()));
    KeyDialog::insert(m_accel, "SeekBack",   i18n("Seek Back"),         this, SLOT(seekBack()));
    KeyDialog::insert(m_accel, "SeekForward",i18n("Seek Forward"),      this, SLOT(seekForward()));
    KeyDialog::insert(m_accel, "VolumeUp",   i18n("Volume Up"),         this, SLOT(volumeUp()));
    KeyDialog::insert(m_accel, "VolumeDown", i18n("Volume Down"),       this, SLOT(volumeDown()));
    KeyDialog::insert(m_accel, "Mute",       i18n("Mute"),              this, SLOT(volumeMute()));

    m_accel->setConfigGroup("Shortcuts");
    m_accel->readSettings();
    m_accel->updateConnections();
}

void JuK::processArgs()
{
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    QStringList files;

    for(int i = 0; i < args->count(); i++)
        files.append(args->arg(i));

    m_splitter->open(files);
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
    KConfig *config = KGlobal::config();
    { // general settings
        KConfigGroupSaver saver(config, "Settings");
        m_restore = config->readBoolEntry("RestoreOnLoad", true);
        m_showSplash = config->readBoolEntry("ShowSplashScreen", true);
    }
}

void JuK::readConfig()
{
    // Automagically save and m_restore many window settings.
    setAutoSaveSettings();

    KConfig *config = KGlobal::config();
    { // player settings
        KConfigGroupSaver saver(config, "Player");
        if(m_sliderAction->getVolumeSlider()) {
            int volume = config->readNumEntry("Volume", m_sliderAction->getVolumeSlider()->maxValue());
            m_sliderAction->getVolumeSlider()->setValue(volume);
        }
        bool randomPlay = config->readBoolEntry("RandomPlay", false);
        m_randomPlayAction->setChecked(randomPlay);

        bool loopPlaylist = config->readBoolEntry("LoopPlaylist", false);
        m_loopPlaylistAction->setChecked(loopPlaylist);
    }
    { // view settings
        KConfigGroupSaver saver(config, "View");

        bool showSearch = config->readBoolEntry("ShowSearch", true);
        m_showSearchAction->setChecked(showSearch);
        m_splitter->slotSetSearchVisible(showSearch);

        bool showEditor = config->readBoolEntry("ShowEditor", false);
        m_showEditorAction->setChecked(showEditor);
        m_splitter->slotSetEditorVisible(showEditor);
    }
    { // general settings
        KConfigGroupSaver saver(config, "Settings");

        bool dockInSystemTray = config->readBoolEntry("DockInSystemTray", true);
        m_toggleSystemTrayAction->setChecked(dockInSystemTray);

        bool dockOnClose = config->readBoolEntry("DockOnClose", true);
        m_toggleDockOnCloseAction->setChecked(dockOnClose);

        bool showPopups = config->readBoolEntry("TrackPopup", false);
        m_togglePopupsAction->setChecked(showPopups);

        if(m_outputSelectAction)
            m_outputSelectAction->setCurrentItem(config->readNumEntry("MediaSystem", 0));
    }

    m_restoreOnLoadAction->setChecked(m_restore);
    m_toggleSplashAction->setChecked(m_showSplash);
}

void JuK::saveConfig()
{
    KConfig *config = KGlobal::config();
    { // m_player settings
        KConfigGroupSaver saver(config, "Player");
        if(m_sliderAction && m_sliderAction->getVolumeSlider())
            config->writeEntry("Volume", m_sliderAction->getVolumeSlider()->value());
        if(m_randomPlayAction)
            config->writeEntry("RandomPlay", m_randomPlayAction->isChecked());
        if(m_loopPlaylistAction)
            config->writeEntry("LoopPlaylist", m_loopPlaylistAction->isChecked());
    }
    { // view settings
        KConfigGroupSaver saver(config, "View");

        config->writeEntry("ShowEditor", m_showEditorAction->isChecked());
        config->writeEntry("ShowSearch", m_showSearchAction->isChecked());
    }
    { // general settings
        KConfigGroupSaver saver(config, "Settings");
        config->writeEntry("RestoreOnLoad", m_restoreOnLoadAction->isChecked());
        config->writeEntry("ShowSplashScreen", m_toggleSplashAction->isChecked());
        config->writeEntry("DockInSystemTray", m_toggleSystemTrayAction->isChecked());
        config->writeEntry("DockOnClose", m_toggleDockOnCloseAction->isChecked());
        config->writeEntry("TrackPopup", m_togglePopupsAction->isChecked());
        if(m_outputSelectAction)
            config->writeEntry("MediaSystem", m_outputSelectAction->currentItem());
    }
    config->sync();
}

bool JuK::queryExit()
{
    stop();
    delete m_player;
    Cache::instance()->save();
    saveConfig();
    delete m_splitter;
    return true;
}

bool JuK::queryClose()
{
    if(!m_shuttingDown && m_systemTray && m_toggleDockOnCloseAction->isChecked()) {
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

void JuK::invokeEditSlot( const char *slotName, const char *slot )
{
    QObject *object = focusWidget();

    if(!object || !slotName || !slot)
        return;

    QMetaObject *meta = object->metaObject();
    QStrList l = meta->slotNames(true);

    if(l.find(slotName) == -1)
        return;

    connect(this, SIGNAL(signalEdit()), object, slot);
    emit signalEdit();
    disconnect(this, SIGNAL(signalEdit()), object, slot);
}

QString JuK::playingString() const
{
    QString s;

    if(m_splitter->playingArtist().isEmpty())
        s = m_splitter->playingTrack().simplifyWhiteSpace();
    else
        s = m_splitter->playingArtist().simplifyWhiteSpace() + " - " + m_splitter->playingTrack().simplifyWhiteSpace();

    return s;
}

void JuK::updatePlaylistInfo()
{
    m_statusLabel->setPlaylistInfo(m_splitter->visiblePlaylistName(), m_splitter->selectedPlaylistCount());
}

void JuK::play(const QString &file)
{
    if(!m_player || !m_sliderAction || !m_sliderAction->getVolumeSlider())
        return;

    float volume = float(m_sliderAction->getVolumeSlider()->value()) / float(m_sliderAction->getVolumeSlider()->maxValue());

    if(m_player->paused())
        m_player->stop();

    m_player->play(file, volume);

    // Make sure that the m_player actually starts before doing anything.

    if(m_player->playing()) {
        actionCollection()->action("pause")->setEnabled(true);
        actionCollection()->action("stop")->setEnabled(true);
        actionCollection()->action("forward")->setEnabled(true);

        m_backAction->setEnabled(true);

        m_sliderAction->getTrackPositionSlider()->setValue(0);
        m_sliderAction->getTrackPositionSlider()->setEnabled(true);
        m_playTimer->start(m_pollInterval);

        m_statusLabel->setPlayingItemInfo(playingString(), m_splitter->playingList());

        emit signalNewSong(playingString());

        if(m_systemTray)
            m_systemTray->slotPlay();
    }
    else
        stop();
}

////////////////////////////////////////////////////////////////////////////////
// private slot definitions
////////////////////////////////////////////////////////////////////////////////

void JuK::slotPlaylistChanged()
{
    if(m_splitter->collectionListSelected() || !m_splitter->hasListSelected()) {
        actionCollection()->action("file_save")->setEnabled(false);
        actionCollection()->action("file_save_as")->setEnabled(false);
        actionCollection()->action("renamePlaylist")->setEnabled(false);
        actionCollection()->action("deleteItemPlaylist")->setEnabled(false);
    }
    else {
        actionCollection()->action("file_save")->setEnabled(true);
        actionCollection()->action("file_save_as")->setEnabled(true);
        actionCollection()->action("renamePlaylist")->setEnabled(true);
        actionCollection()->action("deleteItemPlaylist")->setEnabled(true);
    }
    if(m_splitter->hasListSelected())
        actionCollection()->action("duplicatePlaylist")->setEnabled(true);
    else
        actionCollection()->action("duplicatePlaylist")->setEnabled(false);
    

    updatePlaylistInfo();
}

void JuK::startPlayingPlaylist()
{
    if(m_randomPlayAction->isChecked())
        play(m_splitter->playRandomFile());
    else
        play(m_splitter->playFirstFile());
}

////////////////////////////////////////////////////////////////////////////////
// edit menu
////////////////////////////////////////////////////////////////////////////////

void JuK::cut()
{
    invokeEditSlot("cut()", SLOT(cut()));
}

void JuK::copy()
{
    invokeEditSlot("copy()", SLOT(copy()));
}

void JuK::paste()
{
    invokeEditSlot("paste()", SLOT(paste()));
}

void JuK::clear()
{
    invokeEditSlot("clear()", SLOT(clear()));
}

void JuK::selectAll()
{
    invokeEditSlot("selectAll()", SLOT(selectAll()));
}

////////////////////////////////////////////////////////////////////////////////
// player menu
////////////////////////////////////////////////////////////////////////////////

void JuK::play()
{
    if(!m_player)
    return;

    if(m_player->paused()) {
        m_player->play();

	// Here, before doing anything, we want to make sure that the m_player did
	// in fact start.

        if(m_player->playing()) {
            actionCollection()->action("pause")->setEnabled(true);
            actionCollection()->action("stop")->setEnabled(true);
            m_playTimer->start(m_pollInterval);
	    if(m_systemTray)
		m_systemTray->slotPlay();
        }
    }
    else if(m_player->playing())
	m_player->seekPosition(0);
    else
	play(m_splitter->playNextFile(m_randomPlayAction->isChecked(), m_loopPlaylistAction->isChecked()));
}

void JuK::pause()
{
    if(!m_player)
	return;

    m_playTimer->stop();
    m_player->pause();
    actionCollection()->action("pause")->setEnabled(false);
    if(m_systemTray)
	m_systemTray->slotPause();
}

void JuK::stop()
{
    if(!m_player || !m_sliderAction || !m_sliderAction->getVolumeSlider())
	return;

    m_playTimer->stop();
    m_player->stop();

    actionCollection()->action("pause")->setEnabled(false);
    actionCollection()->action("stop")->setEnabled(false);
    actionCollection()->action("back")->setEnabled(false);
    actionCollection()->action("forward")->setEnabled(false);

    m_sliderAction->getTrackPositionSlider()->setValue(0);
    m_sliderAction->getTrackPositionSlider()->setEnabled(false);

    m_splitter->stop();

    m_statusLabel->clear();

    if(m_systemTray)
	m_systemTray->slotStop();
}

void JuK::back()
{
    play(m_splitter->playPreviousFile(m_randomPlayAction->isChecked()));
}

void JuK::back(int howMany)
{
    for(--howMany; howMany > 0; --howMany)
        m_splitter->playPreviousFile(m_randomPlayAction->isChecked());

    play(m_splitter->playPreviousFile(m_randomPlayAction->isChecked()));
}

void JuK::slotPopulateBackMenu()
{
    m_splitter->populatePlayHistoryMenu(m_backAction->popupMenu(), m_randomPlayAction->isChecked());
}

void JuK::forward()
{
    play(m_splitter->playNextFile(m_randomPlayAction->isChecked(), m_loopPlaylistAction->isChecked()));
}

void JuK::seekBack()
{
    int position = m_sliderAction->getTrackPositionSlider()->value();
    position = QMAX(m_sliderAction->getTrackPositionSlider()->minValue(), position - 10);
    emit m_sliderAction->getTrackPositionSlider()->setValue(position);
}

void JuK::seekForward()
{
    int position = m_sliderAction->getTrackPositionSlider()->value();
    position = QMIN(m_sliderAction->getTrackPositionSlider()->maxValue(), position + 10);
    emit m_sliderAction->getTrackPositionSlider()->setValue(position);
}

////////////////////////////////////////////////////////////////////////////////
// settings menu
////////////////////////////////////////////////////////////////////////////////

void JuK::slotShowGenreListEditor()
{
    GenreListEditor * editor = new GenreListEditor();
    editor->exec();
}

void JuK::slotToggleSystemTray(bool enabled)
{
    if(enabled && !m_systemTray)
	setupSystemTray();
    else if(!enabled && m_systemTray) {
	delete m_systemTray;
	m_systemTray = 0;
	m_toggleDockOnCloseAction->setEnabled(false);
    }
}

void JuK::slotSetOutput(int output)
{
    stop();
    delete m_player;
    m_player = Player::createPlayer(output);
}

void JuK::slotEditKeys()
{
    KeyDialog::configure(m_accel, actionCollection(), this);
}

////////////////////////////////////////////////////////////////////////////////
// additional player slots
////////////////////////////////////////////////////////////////////////////////

void JuK::slotTrackPositionSliderClicked()
{
    m_trackPositionDragging = true;
}

void JuK::slotTrackPositionSliderReleased()
{
    if(!m_player)
	return;

    m_trackPositionDragging = false;
    m_player->seekPosition(m_sliderAction->getTrackPositionSlider()->value());
}

void JuK::slotTrackPositionSliderUpdate(int position)
{
    if(!m_player)
	return;

    if(m_player->playing() && !m_trackPositionDragging && !m_noSeek)
        m_player->seekPosition(position);

    // The dragging flag is set, so just update the status label, rather than seeking
    if(m_player->playing() && m_trackPositionDragging && !m_noSeek) {
	// position from 0 to 1
	float positionFraction = float(position) / m_sliderAction->getTrackPositionSlider()->maxValue();
	float totalTime = float(m_player->totalTime());
	long seekTime = long(positionFraction * totalTime + 0.5); // "+0.5" for rounding

	m_statusLabel->setItemCurrentTime(seekTime);
    }
}

void JuK::playPause()
{
    if(!m_player)
	return;

    if(m_player->playing())
	pause();
    else
	play();
}

void JuK::volumeUp()
{
    if(m_sliderAction && m_sliderAction->getVolumeSlider()) {
	int volume = m_sliderAction->getVolumeSlider()->value() +
	  m_sliderAction->getVolumeSlider()->maxValue() / 25; // 4% up
	setVolume(volume);
	m_sliderAction->getVolumeSlider()->setValue(volume);
    }
}

void JuK::volumeDown()
{
    if(m_sliderAction && m_sliderAction->getVolumeSlider()) {
	int volume = m_sliderAction->getVolumeSlider()->value() -
	  m_sliderAction->getVolumeSlider()->maxValue() / 25; // 4% down
	setVolume(volume);
	m_sliderAction->getVolumeSlider()->setValue(volume);
    }
}

void JuK::volumeMute()
{
    if(m_sliderAction && m_sliderAction->getVolumeSlider()) {
	if(m_muted)
	    setVolume(m_sliderAction->getVolumeSlider()->value());
	else
	    setVolume(0);
	    m_muted = !m_muted;
    }
}

// This method is called when the play timer has expired.

void JuK::slotPollPlay()
{
    if(!m_player)
	return;

    // Our locking mechanism.  Since this method adjusts the play slider, we
    // want to make sure that our adjustments
    m_noSeek = true;

    if(!m_player->playing()) {

        m_playTimer->stop();

	play(m_splitter->playNextFile(m_randomPlayAction->isChecked(), m_loopPlaylistAction->isChecked()));
    }
    else if(!m_trackPositionDragging) {
        m_sliderAction->getTrackPositionSlider()->setValue(m_player->position());
	m_statusLabel->setItemTotalTime(m_player->totalTime());
	m_statusLabel->setItemCurrentTime(m_player->currentTime());
    }

    // Ok, this is weird stuff, but it works pretty well.  Ordinarily we don't
    // need to check up on our playing time very often, but in the span of the
    // last interval, we want to check a lot -- to figure out that we've hit the
    // end of the song as soon as possible.

    if(m_player->playing() && m_player->totalTime() > 0 && float(m_player->totalTime() - m_player->currentTime()) < m_pollInterval * 2)
        m_playTimer->changeInterval(50);

    m_noSeek = false;
}

void JuK::setVolume(int volume)
{
    if(m_player && m_sliderAction && m_sliderAction->getVolumeSlider() &&
       m_sliderAction->getVolumeSlider()->maxValue() > 0 &&
       volume >= 0 && m_sliderAction->getVolumeSlider()->maxValue() >= volume)
    {
        m_player->setVolume(float(volume) / float(m_sliderAction->getVolumeSlider()->maxValue()));
    }
}

void JuK::slotConfigureTagGuesser()
{
    TagGuesserConfigDlg dlg(this);
    dlg.exec();
}

void JuK::openFile(const QString &file)
{
    m_splitter->open(file);
}

KAction *JuK::createSplitterAction(const QString &text, const char *slot, const char *name, 
                               const QString &pix, const KShortcut &shortcut)
{
    KAction *action;

    if(pix.isNull())
	action = new KAction(text, shortcut, actionCollection(), name);
    else
	action = new KAction(text, pix, shortcut, actionCollection(), name);

    m_splitterConnections.append(SplitterConnection(action, slot));
    
    return action;
}

#include "juk.moc"
