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

#include <kaction.h>
#include <kcmdlineargs.h>
#include <kstatusbar.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <qmetaobject.h>
#include <qslider.h>
#include <qmime.h>

#include "juk.h"
#include "slideraction.h"
#include "statuslabel.h"
#include "splashscreen.h"
#include "systemtray.h"
#include "keydialog.h"
#include "tagguesserconfigdlg.h"
#include "filerenamerconfigdlg.h"
#include "playermanager.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

JuK::JuK(QWidget *parent, const char *name) :
    DCOPObject("Player"),
    KMainWindow(parent, name, WDestructiveClose),
    m_shuttingDown(false)
{
    // Expect segfaults if you change this order.

    readSettings();

    if(m_showSplash && !m_startDocked) {
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
// public slots
////////////////////////////////////////////////////////////////////////////////

void JuK::setVolume(float volume)
{
    if(m_sliderAction->volumeSlider()->maxValue() > 0 &&
       volume >= 0 && m_sliderAction->volumeSlider()->maxValue() >= volume)
    {
	int v = int(volume / 100 * m_sliderAction->volumeSlider()->maxValue());
	slotSetVolume(v);
	m_sliderAction->volumeSlider()->setValue(v);
    }
}

void JuK::startPlayingPlaylist()
{
    if(m_randomPlayAction->isChecked())
        play(m_splitter->playRandomFile());
    else
        play(m_splitter->playFirstFile());
}

void JuK::slotGuessTagInfoFromFile()
{
    m_splitter->slotGuessTagInfo(TagGuesser::FileName);
}

void JuK::slotGuessTagInfoFromInternet()
{
    m_splitter->slotGuessTagInfo(TagGuesser::MusicBrainz);
}

void JuK::play()
{
    m_player->play();
}

void JuK::pause()
{
    m_player->pause();
}

void JuK::stop()
{
    m_player->stop();
    m_splitter->stop();
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
    m_splitter->populatePlayHistoryMenu(m_backAction->popupMenu(),
					m_randomPlayAction->isChecked());
}

void JuK::forward()
{
    play(m_splitter->playNextFile(m_randomPlayAction->isChecked(),
				  m_loopPlaylistAction->isChecked()));
}

void JuK::seekBack()
{
    int position = m_sliderAction->trackPositionSlider()->value();
    position = QMAX(m_sliderAction->trackPositionSlider()->minValue(), position - 10);
    emit m_sliderAction->trackPositionSlider()->setValue(position);
}

void JuK::seekForward()
{
    int position = m_sliderAction->trackPositionSlider()->value();
    position = QMIN(m_sliderAction->trackPositionSlider()->maxValue(), position + 10);
    emit m_sliderAction->trackPositionSlider()->setValue(position);
}

void JuK::playPause()
{
    if(m_player->playing())
	pause();
    else
	play();
}

void JuK::volumeUp()
{
    if(m_sliderAction && m_sliderAction->volumeSlider()) {
	int volume = m_sliderAction->volumeSlider()->value() +
	    m_sliderAction->volumeSlider()->maxValue() / 25; // 4% up
	slotSetVolume(volume);
	m_sliderAction->volumeSlider()->setValue(volume);
    }
}

void JuK::volumeDown()
{
    if(m_sliderAction && m_sliderAction->volumeSlider()) {
	int volume = m_sliderAction->volumeSlider()->value() -
	    m_sliderAction->volumeSlider()->maxValue() / 25; // 4% down
	slotSetVolume(volume);
	m_sliderAction->volumeSlider()->setValue(volume);
    }
}

void JuK::volumeMute()
{
    if(m_sliderAction && m_sliderAction->volumeSlider()) {
	if(m_muted)
	    slotSetVolume(m_sliderAction->volumeSlider()->value());
	else
	    slotSetVolume(0);
	    m_muted = !m_muted;
    }
}

void JuK::openFile(const QString &file)
{
    m_splitter->open(file);
}

void JuK::openFile(const QStringList &files)
{
    for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it)
	openFile(*it);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void JuK::setupLayout()
{
    m_splitter = new PlaylistSplitter(this, "playlistSplitter");
    setCentralWidget(m_splitter);

    // playlist item activation connection
    connect(m_splitter, SIGNAL(signalActivated()),
	    this, SLOT(slotPlaySelectedFile()));
    connect(m_splitter, SIGNAL(signalListBoxDoubleClicked()),
	    this, SLOT(startPlayingPlaylist()));

    // create status bar

    m_statusLabel = new StatusLabel(statusBar());

    statusBar()->addWidget(m_statusLabel, 1);

    connect(m_splitter, SIGNAL(signalSelectedPlaylistCountChanged(int)),
	    m_statusLabel, SLOT(setPlaylistCount(int)));
    connect(m_splitter, SIGNAL(signalSelectedPlaylistTimeChanged(int)),
	    m_statusLabel, SLOT(setPlaylistTime(int)));
    connect(m_statusLabel, SIGNAL(jumpButtonClicked()),
	    m_splitter, SLOT(slotSelectPlaying()));

    PlayerManager::instance()->setStatusLabel(m_statusLabel);

    // Needs to be here because m_splitter is not called before setupActions
    // (because PlaylistSplitter itself accesses the actionCollection)

    new KAction(i18n("&Rename File"), 0, "CTRL+r", m_splitter, SLOT(slotRenameFile()),
                actionCollection(), "renameFile"); // 4

    m_splitter->setFocus();

    resize(750, 500);
}

void JuK::setupActions()
{
    //////////////////////////////////////////////////
    // file menu
    //////////////////////////////////////////////////

    KActionMenu *newMenu = new KActionMenu(i18n("&New"), "filenew",
					   actionCollection(), "file_new");

    // This connection will call the
    // PlaylistSplitter::slotCreatePlaylist(const QString &) slot - this is
    // possible because the QString parameter has a default value, so the
    // slot can be called without arguments (as required by the signal's
    // signature).

    newMenu->insert(createSplitterAction(i18n("Empty Playlist..."),
					 SLOT(slotCreatePlaylist()), 0, 0, 0));

    newMenu->insert(createSplitterAction(i18n("Playlist From Folder..."),
					 SLOT(slotCreatePlaylistFromDir()), 0, 0, 0));

    createSplitterAction(
	i18n("Open..."), SLOT(slotOpen()), "file_open", "fileopen", "CTRL+o");
    createSplitterAction(
	i18n("Open &Folder..."), SLOT(slotOpenDirectory()), "openDirectory", "fileopen");
    createSplitterAction(
	i18n("&Rename..."), SLOT(slotRenamePlaylist()), "renamePlaylist");
    createSplitterAction(
	i18n("D&uplicate..."), SLOT(slotDuplicatePlaylist()), "duplicatePlaylist");
    createSplitterAction(
	i18n("Save"), SLOT(slotSavePlaylist()), "file_save", "filesave", "CTRL+s");
    createSplitterAction(
	i18n("Save As..."), SLOT(slotSaveAsPlaylist()), "file_save_as", "filesaveas");
    createSplitterAction(
	i18n("R&emove"), SLOT(slotDeletePlaylist()), "deleteItemPlaylist", "edittrash");
    createSplitterAction(
	i18n("Reload"), SLOT(slotReloadPlaylist()), "reloadPlaylist", "reload");

    KStdAction::quit(this, SLOT(slotQuit()), actionCollection());

    //////////////////////////////////////////////////
    // edit menu
    //////////////////////////////////////////////////

    KStdAction::cut(kapp,   SLOT(cut()),   actionCollection());
    KStdAction::copy(kapp,  SLOT(copy()),  actionCollection());
    KStdAction::paste(kapp, SLOT(paste()), actionCollection());
    KStdAction::clear(kapp, SLOT(clear()), actionCollection());

    KStdAction::selectAll(kapp, SLOT(selectAll()), actionCollection());

    createSplitterAction(
	i18n("Advanced Search"), SLOT(slotAdvancedSearch()), "advancedSearch", "find", "s");

    //////////////////////////////////////////////////
    // view menu
    //////////////////////////////////////////////////

    m_showSearchAction = new KToggleAction(i18n("Show &Search Bar"), "filefind",
					   0, actionCollection(), "showSearch");
    m_showEditorAction = new KToggleAction(i18n("Show &Tag Editor"), "edit",
					   0, actionCollection(), "showEditor");
    m_showHistoryAction = new KToggleAction(i18n("Show &History"), "history",
					   0, actionCollection(), "showHistory");

    createSplitterAction(i18n("Refresh Items"), SLOT(slotRefresh()), "refresh", "reload");

    //////////////////////////////////////////////////
    // play menu
    //////////////////////////////////////////////////

    m_randomPlayAction = new KToggleAction(i18n("&Random Play"), 0,
					   actionCollection(), "randomPlay");

    new KAction(i18n("&Play"),  "player_play",  0, this, SLOT(play()),
		actionCollection(), "play");

    new KAction(i18n("P&ause"), "player_pause", 0, this, SLOT(pause()),
		actionCollection(), "pause");

    new KAction(i18n("&Stop"),  "player_stop",  0, this, SLOT(stop()),
		actionCollection(), "stop");

    m_backAction = new KToolBarPopupAction(i18n("Previous &Track"), "player_start", 0,
					   this, SLOT(back()), actionCollection(), "back");

    connect(m_backAction->popupMenu(), SIGNAL(aboutToShow()),
	    this, SLOT(slotPopulateBackMenu()));

    connect(m_backAction->popupMenu(), SIGNAL(activated(int)),
	    this, SLOT(back(int)));

    new KAction(i18n("&Next Track"), "player_end", 0, this, SLOT(forward()),
		actionCollection(), "forward");

    m_loopPlaylistAction = new KToggleAction(i18n("&Loop Playlist"), 0, 0,
					     actionCollection(), "loopPlaylist");

    //////////////////////////////////////////////////
    // tagger menu
    //////////////////////////////////////////////////

    createSplitterAction(i18n("&Save"), SLOT(slotSaveTag()),
			 "saveItem",   "filesave", "CTRL+t");
    createSplitterAction(i18n("&Delete"), SLOT(slotDeleteSelectedItems()),
			 "removeItem", "editdelete");

    m_guessMenu = new KActionMenu(i18n("&Guess Tag Information"),
					     QString::null, actionCollection(),
					     "guessTag");

    KAction *a;

    a = new KAction(i18n("From &Filename"), 0, "CTRL+f", this, SLOT(slotGuessTagInfoFromFile()),
                actionCollection(), "guessTagFile");
    m_guessMenu->insert(a);
#if HAVE_MUSICBRAINZ
    a = new KAction(i18n("From &Internet"), 0, "CTRL+i", this, SLOT(slotGuessTagInfoFromInternet()),
                actionCollection(), "guessTagInternet");
    m_guessMenu->insert(a);
#endif

    //////////////////////////////////////////////////
    // settings menu
    //////////////////////////////////////////////////

    new KToggleAction(i18n("Show Menu Bar"), "CTRL+m", this,
		      SLOT(slotToggleMenuBar()), actionCollection(), "toggleMenuBar");

    setStandardToolBarMenuEnabled(true);

    m_toggleSplashAction = new KToggleAction(i18n("Show Splash Screen on Startup"),
					     0, actionCollection(), "showSplashScreen");

    m_toggleSystemTrayAction = new KToggleAction(i18n("&Dock in System Tray"),
						 0, actionCollection(), "toggleSystemTray");

    m_toggleDockOnCloseAction = new KToggleAction(i18n("&Stay in System Tray on Close"),
						  0, actionCollection(), "dockOnClose");

    m_togglePopupsAction = new KToggleAction(i18n("Popup &Track Announcement"),
					     0, this, 0, actionCollection(), "togglePopups");

    connect(m_toggleSystemTrayAction, SIGNAL(toggled(bool)),
	    this, SLOT(slotToggleSystemTray(bool)));


    m_outputSelectAction = Player::playerSelectAction(actionCollection());

    if(m_outputSelectAction)
        m_outputSelectAction->setCurrentItem(0);

    new KAction(i18n("&Tag Guesser..."), 0, 0, this, SLOT(slotConfigureTagGuesser()),
		actionCollection(), "tagGuesserConfig");

    new KAction(i18n("&File Renamer..."), 0, 0, this, SLOT(slotConfigureFileRenamer()),
		actionCollection(), "fileRenamerConfig");

    KStdAction::keyBindings(this, SLOT(slotEditKeys()), actionCollection());

    //////////////////////////////////////////////////
    // just in the toolbar
    //////////////////////////////////////////////////

    m_sliderAction = new SliderAction(i18n("Track Position"), actionCollection(),
				      "trackPositionAction");

    createGUI();

    // set the slider to the proper orientation and make it stay that way
    m_sliderAction->slotUpdateOrientation();
}

void JuK::setupSplitterConnections()
{
    QValueListConstIterator<SplitterConnection> it = m_splitterConnections.begin();
    for(; it != m_splitterConnections.end(); ++it)
        connect((*it).first, SIGNAL(activated()), m_splitter, (*it).second);

    connect(m_showSearchAction, SIGNAL(toggled(bool)),
	    m_splitter, SLOT(slotSetSearchVisible(bool)));
    connect(m_showEditorAction, SIGNAL(toggled(bool)),
	    m_splitter, SLOT(slotSetEditorVisible(bool)));
    connect(m_showHistoryAction, SIGNAL(toggled(bool)),
	    m_splitter, SLOT(slotSetHistoryVisible(bool)));
    connect(this, SIGNAL(dockWindowPositionChanged(QDockWindow *)),
	    m_sliderAction, SLOT(slotUpdateOrientation(QDockWindow *)));
    connect(m_splitter, SIGNAL(signalPlaylistChanged()),
	    this, SLOT(slotPlaylistChanged()));
}

void JuK::setupSystemTray()
{
    if(m_toggleSystemTrayAction && m_toggleSystemTrayAction->isChecked()) {
        m_systemTray = new SystemTray(this, "systemTray");
        m_systemTray->show();

        connect(this, SIGNAL(signalNewSong(const QString&)),
		m_systemTray, SLOT(slotNewSong(const QString&)));

        m_toggleDockOnCloseAction->setEnabled(true);
	m_togglePopupsAction->setEnabled(true);

        connect(m_systemTray, SIGNAL(quitSelected()), this, SLOT(slotQuit()));
    }
    else {
        m_systemTray = 0;
        m_toggleDockOnCloseAction->setEnabled(false);
	m_togglePopupsAction->setEnabled(false);
    }
}

void JuK::setupPlayer()
{
    m_muted = false;

    if(m_sliderAction) {
    }

    m_player = PlayerManager::instance();
    PlayerManager::instance()->setPlaylistInterface(m_splitter);
}


void JuK::setupGlobalAccels()
{
    m_accel = new KGlobalAccel(this);
    KeyDialog::insert(m_accel, "PlayPause",   i18n("Play/Pause"),   this, SLOT(playPause()));
    KeyDialog::insert(m_accel, "Stop",        i18n("Stop Playing"), this, SLOT(stop()));
    KeyDialog::insert(m_accel, "Back",        i18n("Back"),         this, SLOT(back()));
    KeyDialog::insert(m_accel, "Forward",     i18n("Forward"),      this, SLOT(forward()));
    KeyDialog::insert(m_accel, "SeekBack",    i18n("Seek Back"),    this, SLOT(seekBack()));
    KeyDialog::insert(m_accel, "SeekForward", i18n("Seek Forward"), this, SLOT(seekForward()));
    KeyDialog::insert(m_accel, "VolumeUp",    i18n("Volume Up"),    this, SLOT(volumeUp()));
    KeyDialog::insert(m_accel, "VolumeDown",  i18n("Volume Down"),  this, SLOT(volumeDown()));
    KeyDialog::insert(m_accel, "Mute",        i18n("Mute"),         this, SLOT(volumeMute()));

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
        m_showSplash = config->readBoolEntry("ShowSplashScreen", true);
        m_startDocked = config->readBoolEntry("StartDocked", false);
    }
}

void JuK::readConfig()
{
    // Automagically save and m_restore many window settings.
    setAutoSaveSettings();

    KConfig *config = KGlobal::config();
    { // player settings
        KConfigGroupSaver saver(config, "Player");
        if(m_sliderAction->volumeSlider()) {
	    int maxVolume = m_sliderAction->volumeSlider()->maxValue();
	    int volume = config->readNumEntry("Volume", maxVolume);
            m_sliderAction->volumeSlider()->setValue(volume);
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

	// The history list will actually be created by the playlist restoration
	// code, but we want to remember the checkbox's setting and hope that
	// it's in synch with the code that does the real work.

	bool showHistory = config->readBoolEntry("ShowHistory", false);
	m_showHistoryAction->setChecked(showHistory);
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

    m_toggleSplashAction->setChecked(m_showSplash);
}

void JuK::saveConfig()
{
    KConfig *config = KGlobal::config();
    { // m_player settings
        KConfigGroupSaver saver(config, "Player");
        if(m_sliderAction && m_sliderAction->volumeSlider())
            config->writeEntry("Volume", m_sliderAction->volumeSlider()->value());
        if(m_randomPlayAction)
            config->writeEntry("RandomPlay", m_randomPlayAction->isChecked());
        if(m_loopPlaylistAction)
            config->writeEntry("LoopPlaylist", m_loopPlaylistAction->isChecked());
    }
    { // view settings
        KConfigGroupSaver saver(config, "View");

        config->writeEntry("ShowEditor", m_showEditorAction->isChecked());
        config->writeEntry("ShowSearch", m_showSearchAction->isChecked());
	config->writeEntry("ShowHistory", m_showHistoryAction->isChecked());
    }
    { // general settings
        KConfigGroupSaver saver(config, "Settings");
        config->writeEntry("ShowSplashScreen", m_toggleSplashAction->isChecked());
        config->writeEntry("StartDocked", m_startDocked);
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
    m_startDocked = !isVisible();

    hide();
    delete m_systemTray;
    m_systemTray = 0;

    stop();

    Cache::instance()->save();
    saveConfig();
    delete m_splitter;
    return true;
}

bool JuK::queryClose()
{
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

QString JuK::playingString() const
{
    QString s;
    if(!m_player->playing() && !m_player->paused())
	return i18n("No song playing");

    if(!m_splitter->playingArtist().isEmpty())
	s = m_splitter->playingArtist().simplifyWhiteSpace() + " - ";

    s += m_splitter->playingTrack().simplifyWhiteSpace();

    return s;
}

void JuK::updatePlaylistInfo()
{
    m_statusLabel->setPlaylistInfo(m_splitter->visiblePlaylistName(),
				   m_splitter->selectedPlaylistCount(),
				   m_splitter->selectedPlaylistTotalTime());
}

void JuK::play(const QString &file)
{
    m_player->play(file);
    emit signalNewSong(playingString());
}

KAction *JuK::createSplitterAction(const QString &text, const char *slot,
				   const char *name, const QString &pix,
				   const KShortcut &shortcut)
{
    KAction *action;

    if(pix.isNull())
	action = new KAction(text, shortcut, actionCollection(), name);
    else
	action = new KAction(text, pix, shortcut, actionCollection(), name);

    m_splitterConnections.append(SplitterConnection(action, slot));

    return action;
}

////////////////////////////////////////////////////////////////////////////////
// private slot definitions
////////////////////////////////////////////////////////////////////////////////

void JuK::slotPlaylistChanged()
{
    if(m_splitter->collectionListSelected() ||
       !m_splitter->hasListSelected() ||
       m_splitter->readOnlyListSelected())
    {
        actionCollection()->action("file_save")->setEnabled(false);
        actionCollection()->action("file_save_as")->setEnabled(false);
        actionCollection()->action("renamePlaylist")->setEnabled(false);
        actionCollection()->action("reloadPlaylist")->setEnabled(false);
        actionCollection()->action("deleteItemPlaylist")->setEnabled(false);
    }
    else {
        actionCollection()->action("file_save")->setEnabled(true);
        actionCollection()->action("file_save_as")->setEnabled(true);
        actionCollection()->action("renamePlaylist")->setEnabled(true);
        actionCollection()->action("deleteItemPlaylist")->setEnabled(true);

	if(m_splitter->fileBasedListSelected() || m_splitter->dynamicListSelected())
	    actionCollection()->action("reloadPlaylist")->setEnabled(true);
	else
	    actionCollection()->action("reloadPlaylist")->setEnabled(false);
    }
    if(m_splitter->hasListSelected())
        actionCollection()->action("duplicatePlaylist")->setEnabled(true);
    else
        actionCollection()->action("duplicatePlaylist")->setEnabled(false);

    updatePlaylistInfo();
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
    KeyDialog::configure(m_accel, actionCollection(), this);
}

////////////////////////////////////////////////////////////////////////////////
// additional player slots
////////////////////////////////////////////////////////////////////////////////

void JuK::slotPlaySelectedFile()
{
    QString file = m_splitter->playSelectedFile();
    if(!file.isNull())
	play(m_splitter->playSelectedFile());
}

void JuK::slotSetVolume(int volume)
{
    m_player->slotSetVolume(volume);
}

void JuK::slotConfigureTagGuesser()
{
    TagGuesserConfigDlg dlg(this);
    dlg.exec();
}

void JuK::slotConfigureFileRenamer()
{
    FileRenamerConfigDlg dlg(this);
    dlg.exec();
}

#include "juk.moc"
