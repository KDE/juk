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

#include <kapplication.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kcmdlineargs.h>
#include <kstatusbar.h>
#include <kconfig.h>
#include <kdebug.h>

#include <qtimer.h>
#include <qlistview.h>
#include <qinputdialog.h>
#include <qslider.h>
#include <qstrlist.h>
#include <qmetaobject.h>

#include "juk.h"
#include "slideraction.h"
#include "cache.h"
#include "statuslabel.h"
#include "splashscreen.h"
#include "genrelisteditor.h"
#include "systemtray.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

JuK::JuK(QWidget *parent, const char *name) : KMainWindow(parent, name, WDestructiveClose)
{
    SplashScreen::instance()->show();
    kapp->processEvents();
 
    // Expect segfaults if you change this order.

    readSettings();
    setupLayout();
    setupActions();
    playlistChanged();
    readConfig();
    setupPlayer();
    setupSystemTray();
    processArgs();

    SplashScreen::finishedLoading();
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
    connect(m_splitter, SIGNAL(signalDoubleClicked()), this, SLOT(playSelectedFile()));
    connect(m_splitter, SIGNAL(signalListBoxDoubleClicked()), this, SLOT(playFirstFile()));

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
    // file menu
    KStdAction::openNew(m_splitter, SLOT(slotCreatePlaylist()), actionCollection());
    KStdAction::open(m_splitter, SLOT(slotOpen()), actionCollection());
    new KAction(i18n("Open &Directory..."), "fileopen", 0, m_splitter, SLOT(slotOpenDirectory()), actionCollection(), "openDirectory");

    m_renamePlaylistAction = new KAction(i18n("Rename..."), 0, m_splitter, SLOT(slotRenamePlaylist()), 
				       actionCollection(), "renamePlaylist");
    new KAction(i18n("Duplicate..."), "editcopy", 0, m_splitter, SLOT(slotDuplicatePlaylist()), actionCollection(), "duplicatePlaylist");
    
    m_savePlaylistAction = KStdAction::save(m_splitter, SLOT(slotSavePlaylist()), actionCollection());
    m_saveAsPlaylistAction = KStdAction::saveAs(m_splitter, SLOT(slotSaveAsPlaylist()), actionCollection());
    m_deleteItemPlaylistAction = new KAction(i18n("Remove"), "edittrash", 0, m_splitter, SLOT(slotDeletePlaylist()), 
					   actionCollection(), "deleteItemPlaylist");

    KStdAction::quit(this, SLOT(close()), actionCollection());

    // edit menu
    KStdAction::cut(this, SLOT(cut()), actionCollection());
    KStdAction::copy(this, SLOT(copy()), actionCollection());
    KStdAction::paste(this, SLOT(paste()), actionCollection());
    new KAction(i18n("Clear"), "editclear", 0, this, SLOT(clear()), actionCollection(), "clear");
    KStdAction::selectAll(this, SLOT(selectAll()), actionCollection());

    // view menu
    m_showEditorAction = new KToggleAction(i18n("Show Tag Editor"), "edit", 0, actionCollection(), "showEditor");
    connect(m_showEditorAction, SIGNAL(toggled(bool)), m_splitter, SLOT(slotSetEditorVisible(bool)));
    KStdAction::redisplay(m_splitter, SLOT(slotRefresh()), actionCollection());
    actionCollection()->insert(m_splitter->columnVisibleAction());
    
    // play menu
    m_randomPlayAction = new KToggleAction(i18n("Random Play"), 0, actionCollection(), "randomPlay");
    m_playAction = new KAction(i18n("&Play"), "player_play", 0, this, SLOT(play()), actionCollection(), "play");
    m_pauseAction = new KAction(i18n("P&ause"), "player_pause", 0, this, SLOT(pause()), actionCollection(), "pause");
    m_stopAction = new KAction(i18n("&Stop"), "player_stop", 0, this, SLOT(stop()), actionCollection(), "stop");
    m_backAction = new KAction(i18n("Skip &Back"), "player_start", 0, this, SLOT(back()), actionCollection(), "back");
    m_forwardAction = new KAction(i18n("Skip &Forward"), "player_end", 0, this, SLOT(forward()), actionCollection(), "forward");

    // tagger menu
    new KAction(i18n("Save"), "filesave", "CTRL+t", m_splitter, SLOT(slotSaveTag()), actionCollection(), "saveItem");
    new KAction(i18n("Delete"), "editdelete", 0, m_splitter, SLOT(slotDeleteSelectedItems()), actionCollection(), "removeItem");
    
    // settings menu

    new KToggleAction(i18n("Show Menu Bar"), "CTRL+m", this, SLOT(slotToggleMenuBar()), actionCollection(), "toggleMenuBar");
    new KToggleAction(i18n("Show Tool Bar"), "CTRL+b", this, SLOT(slotToggleToolBar()), actionCollection(), "toggleToolBar");

    m_restoreOnLoadAction = new KToggleAction(i18n("Restore Playlists on Load"),  0, actionCollection(), "restoreOnLoad");

    m_toggleSystemTrayAction = new KToggleAction(i18n("Dock in System Tray"), 0, actionCollection(), "toggleSystemTray");
    connect(m_toggleSystemTrayAction, SIGNAL(toggled(bool)), this, SLOT(toggleSystemTray(bool)));

    new KAction(i18n("Genre List Editor..."), 0, this, SLOT(showGenreListEditor()), actionCollection(), "showGenreListEditor");


    m_outputSelectAction = Player::playerSelectAction(actionCollection());
    if(m_outputSelectAction) {
	m_outputSelectAction->setCurrentItem(0);
	connect(m_outputSelectAction, SIGNAL(activated(int)), this, SLOT(setOutput(int)));
    }

    // just in the toolbar
    m_sliderAction = new SliderAction(i18n("Track Position"), actionCollection(), "trackPositionAction");

    createGUI();

    // set the slider to the proper orientation and make it stay that way
    m_sliderAction->updateOrientation();
    connect(this, SIGNAL(dockWindowPositionChanged(QDockWindow *)), m_sliderAction, SLOT(updateOrientation(QDockWindow *)));

    connect(m_splitter, SIGNAL(signalPlaylistChanged()), this, SLOT(playlistChanged()));
}

void JuK::setupSystemTray()
{
    if(m_toggleSystemTrayAction && m_toggleSystemTrayAction->isChecked()) {
	m_systemTray = new SystemTray(this, "systemTray");
	m_systemTray->show();
	
	connect(m_systemTray, SIGNAL(play()),    this, SLOT(play()));
	connect(m_systemTray, SIGNAL(stop()),    this, SLOT(stop()));
	connect(m_systemTray, SIGNAL(pause()),   this, SLOT(pause()));
	connect(m_systemTray, SIGNAL(back()),    this, SLOT(back()));
	connect(m_systemTray, SIGNAL(forward()), this, SLOT(forward()));

	connect(this, SIGNAL(newSongSignal(const QString&)), m_systemTray, SLOT(slotNewSong(const QString&)));
	
	if(m_player && m_player->paused())
	    m_systemTray->slotPause();
	else if(m_player && m_player->playing())
	    m_systemTray->slotPlay();
    }
    else
	m_systemTray = 0;
}

void JuK::setupPlayer()
{
    m_trackPositionDragging = false;
    m_noSeek = false;
    m_pauseAction->setEnabled(false);
    m_stopAction->setEnabled(false);
    m_backAction->setEnabled(false);
    m_forwardAction->setEnabled(false);

    m_playTimer = new QTimer(this);
    connect(m_playTimer, SIGNAL(timeout()), this, SLOT(pollPlay()));

    if(m_sliderAction && m_sliderAction->getTrackPositionSlider() && m_sliderAction->getVolumeSlider()) {
        connect(m_sliderAction->getTrackPositionSlider(), SIGNAL(valueChanged(int)), this, SLOT(trackPositionSliderUpdate(int)));
        connect(m_sliderAction->getTrackPositionSlider(), SIGNAL(sliderPressed()), this, SLOT(trackPositionSliderClick()));
        connect(m_sliderAction->getTrackPositionSlider(), SIGNAL(sliderReleased()), this, SLOT(trackPositionSliderRelease()));
        m_sliderAction->getTrackPositionSlider()->setEnabled(false);

        connect(m_sliderAction->getVolumeSlider(), SIGNAL(valueChanged(int)), this, SLOT(setVolume(int)));
    }
    
    int playerType = 0;
    if(m_outputSelectAction) {
	playerType = m_outputSelectAction->currentItem();
	connect(m_outputSelectAction, SIGNAL(activated(int)), this, SLOT(setOutput(int)));
    }

    m_player = Player::createPlayer(playerType);
}


void JuK::processArgs()
{
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    QStringList files;
    
    for(int i = 0; i < args->count(); i++)
	files.append(args->arg(i));

    m_splitter->open(files);
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
    }
}

void JuK::readConfig()
{
    // Automagically save and m_restore many window settings.
    setAutoSaveSettings();

    KConfig *config = KGlobal::config();
    { // m_player settings
        KConfigGroupSaver saver(config, "Player");
        if(m_sliderAction && m_sliderAction->getVolumeSlider()) {
            int volume = config->readNumEntry("Volume", m_sliderAction->getVolumeSlider()->maxValue());
            m_sliderAction->getVolumeSlider()->setValue(volume);
        }
	if(m_randomPlayAction) {
	    bool randomPlay = config->readBoolEntry("RandomPlay", false);
	    m_randomPlayAction->setChecked(randomPlay);
	}
    }
    { // view settings
        KConfigGroupSaver saver(config, "View");
	bool showEditor = config->readBoolEntry("ShowEditor", false);
	m_showEditorAction->setChecked(showEditor);
	m_splitter->slotSetEditorVisible(showEditor);
    }
    { // general settings
        KConfigGroupSaver saver(config, "Settings");
	bool dockInSystemTray = config->readBoolEntry("DockInSystemTray", true);
	m_toggleSystemTrayAction->setChecked(dockInSystemTray);
	
	if(m_outputSelectAction)
	    m_outputSelectAction->setCurrentItem(config->readNumEntry("MediaSystem", 0));
	
    }

    if(m_restoreOnLoadAction)
	m_restoreOnLoadAction->setChecked(m_restore);
    
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
    }
    { // view settings
        KConfigGroupSaver saver(config, "View");
	config->writeEntry("ShowEditor", m_showEditorAction->isChecked());
    }
    { // general settings
        KConfigGroupSaver saver(config, "Settings");
	if(m_restoreOnLoadAction)
	    config->writeEntry("RestoreOnLoad", m_restoreOnLoadAction->isChecked());
	if(m_toggleSystemTrayAction)
	    config->writeEntry("DockInSystemTray", m_toggleSystemTrayAction->isChecked());
	if(m_outputSelectAction)
	    config->writeEntry("MediaSystem", m_outputSelectAction->currentItem());
    }
}

bool JuK::queryClose()
{
    stop();
    delete m_player;
    Cache::instance()->save();
    saveConfig();
    delete m_splitter;
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
    
    connect(this, SIGNAL( editSignal() ), object, slot);
    emit editSignal();
    disconnect(this, SIGNAL(editSignal()), object, slot);
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

////////////////////////////////////////////////////////////////////////////////
// private slot definitions
////////////////////////////////////////////////////////////////////////////////

void JuK::playlistChanged()
{
    if(m_splitter->collectionListSelected()) {
	m_savePlaylistAction->setEnabled(false);
	m_saveAsPlaylistAction->setEnabled(false);
	m_renamePlaylistAction->setEnabled(false);
	m_deleteItemPlaylistAction->setEnabled(false);	
    }
    else {
	m_savePlaylistAction->setEnabled(true);
	m_saveAsPlaylistAction->setEnabled(true);
	m_renamePlaylistAction->setEnabled(true);
	m_deleteItemPlaylistAction->setEnabled(true);
    }

    updatePlaylistInfo();
}

void JuK::updatePlaylistInfo()
{
    m_statusLabel->setPlaylistInfo(m_splitter->selectedPlaylistName(), m_splitter->selectedPlaylistCount());
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
// m_player menu
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
            m_pauseAction->setEnabled(true);
            m_stopAction->setEnabled(true);
            m_playTimer->start(pollInterval);
	    if(m_systemTray)
		m_systemTray->slotPlay();
        }
    }
    else if(m_player->playing())
	m_player->seekPosition(0);
    else
	play(m_splitter->playNextFile(m_randomPlayAction->isChecked()));
}

void JuK::pause()
{
    if(!m_player)
	return;

    m_playTimer->stop();
    m_player->pause();
    m_pauseAction->setEnabled(false);
    if(m_systemTray)
	m_systemTray->slotPause();
}

void JuK::stop()
{
    if(!m_player)
	return;

    m_playTimer->stop();
    m_player->stop();

    m_pauseAction->setEnabled(false);
    m_stopAction->setEnabled(false);
    m_backAction->setEnabled(false);
    m_forwardAction->setEnabled(false);

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

void JuK::forward()
{
    play(m_splitter->playNextFile(m_randomPlayAction->isChecked()));
}

////////////////////////////////////////////////////////////////////////////////
// settings menu
////////////////////////////////////////////////////////////////////////////////

void JuK::showGenreListEditor()
{
    GenreListEditor * editor = new GenreListEditor();
    editor->exec();
}

void JuK::toggleSystemTray(bool enabled)
{
    if(enabled && !m_systemTray)
	setupSystemTray();
    else if(!enabled && m_systemTray) {
	delete m_systemTray;
	m_systemTray = 0;
    }
}

void JuK::setOutput(int output)
{
    stop();
    delete m_player;
    m_player = Player::createPlayer(output);
}

////////////////////////////////////////////////////////////////////////////////
// additional m_player slots
////////////////////////////////////////////////////////////////////////////////

void JuK::trackPositionSliderClick()
{
    m_trackPositionDragging = true;
}

void JuK::trackPositionSliderRelease()
{
    if(!m_player)
	return;

    m_trackPositionDragging = false;
    m_player->seekPosition(m_sliderAction->getTrackPositionSlider()->value());
}

void JuK::trackPositionSliderUpdate(int position)
{
    if(!m_player)
	return;

    if(m_player->playing() && !m_trackPositionDragging && !m_noSeek)
        m_player->seekPosition(position);
}

// This method is called when the play timer has expired.

void JuK::pollPlay()
{
    if(!m_player)
	return;

    // Our locking mechanism.  Since this method adjusts the play slider, we 
    // want to make sure that our adjustments
    m_noSeek = true;

    if(!m_player->playing()) {

        m_playTimer->stop();

	if(!m_player->paused())
	    play(m_splitter->playNextFile(m_randomPlayAction->isChecked()));

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

    if(m_player->playing() && m_player->totalTime() > 0 && float(m_player->totalTime() - m_player->currentTime()) < pollInterval * 2)
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

void JuK::play(const QString &file)
{
    if(!m_player)
	return;

    float volume = float(m_sliderAction->getVolumeSlider()->value()) / float(m_sliderAction->getVolumeSlider()->maxValue());

    if(m_player->paused())
	m_player->stop();
    
    m_player->play(file, volume);

    // Make sure that the m_player actually starts before doing anything.

    if(m_player->playing()) {
	m_pauseAction->setEnabled(true);
	m_stopAction->setEnabled(true);
	
	m_backAction->setEnabled(true);
	m_forwardAction->setEnabled(true);
	
	m_sliderAction->getTrackPositionSlider()->setValue(0);
	m_sliderAction->getTrackPositionSlider()->setEnabled(true);
	m_playTimer->start(pollInterval);

	m_statusLabel->setPlayingItemInfo(playingString(), m_splitter->playingList());

	emit newSongSignal(playingString());

	if(m_systemTray)
	    m_systemTray->slotPlay();
    }
    else
	stop();
}

#include "juk.moc"
