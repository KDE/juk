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
    splitter = new PlaylistSplitter(this, restore, "playlistSplitter");
    setCentralWidget(splitter);

    // playlist item activation connection
    connect(splitter, SIGNAL(doubleClicked()), this, SLOT(playSelectedFile()));
    connect(splitter, SIGNAL(listBoxDoubleClicked()), this, SLOT(playFirstFile()));

    // create status bar
    statusLabel = new StatusLabel(statusBar());
    statusBar()->addWidget(statusLabel, 1);

    connect(splitter, SIGNAL(selectedPlaylistCountChanged(int)), statusLabel, SLOT(setPlaylistCount(int)));
    connect(statusLabel, SIGNAL(jumpButtonClicked()), splitter, SLOT(selectPlaying()));

    updatePlaylistInfo();

    splitter->setFocus();

    resize(750, 500);
}

void JuK::setupActions()
{
    // file menu
    KStdAction::openNew(splitter, SLOT(createPlaylist()), actionCollection());
    KStdAction::open(splitter, SLOT(open()), actionCollection());
    new KAction(i18n("Open &Directory..."), "fileopen", 0, splitter, SLOT(openDirectory()), actionCollection(), "openDirectory");

    renamePlaylistAction = new KAction(i18n("Rename..."), 0, splitter, SLOT(renamePlaylist()), 
				       actionCollection(), "renamePlaylist");
    new KAction(i18n("Duplicate..."), "editcopy", 0, splitter, SLOT(duplicatePlaylist()), actionCollection(), "duplicatePlaylist");
    
    savePlaylistAction = KStdAction::save(splitter, SLOT(savePlaylist()), actionCollection());
    saveAsPlaylistAction = KStdAction::saveAs(splitter, SLOT(saveAsPlaylist()), actionCollection());
    deleteItemPlaylistAction = new KAction(i18n("Remove"), "edittrash", 0, splitter, SLOT(deleteItemPlaylist()), 
					   actionCollection(), "deleteItemPlaylist");

    KStdAction::quit(this, SLOT(close()), actionCollection());

    // edit menu
    KStdAction::cut(this, SLOT(cut()), actionCollection());
    KStdAction::copy(this, SLOT(copy()), actionCollection());
    KStdAction::paste(this, SLOT(paste()), actionCollection());
    new KAction(i18n("Clear"), "editclear", 0, this, SLOT(clear()), actionCollection(), "clear");
    KStdAction::selectAll(this, SLOT(selectAll()), actionCollection());

    // view menu
    showEditorAction = new KToggleAction(i18n("Show Tag Editor"), "edit", 0, actionCollection(), "showEditor");
    connect(showEditorAction, SIGNAL(toggled(bool)), splitter, SLOT(setEditorVisible(bool)));
    KStdAction::redisplay(splitter, SLOT(refresh()), actionCollection());
    actionCollection()->insert(splitter->columnVisibleAction());
    
    // play menu
    randomPlayAction = new KToggleAction(i18n("Random Play"), 0, actionCollection(), "randomPlay");
    playAction = new KAction(i18n("&Play"), "player_play", 0, this, SLOT(play()), actionCollection(), "play");
    pauseAction = new KAction(i18n("P&ause"), "player_pause", 0, this, SLOT(pause()), actionCollection(), "pause");
    stopAction = new KAction(i18n("&Stop"), "player_stop", 0, this, SLOT(stop()), actionCollection(), "stop");
    backAction = new KAction(i18n("Skip &Back"), "player_start", 0, this, SLOT(back()), actionCollection(), "back");
    forwardAction = new KAction(i18n("Skip &Forward"), "player_end", 0, this, SLOT(forward()), actionCollection(), "forward");

    // tagger menu
    new KAction(i18n("Save"), "filesave", "CTRL+t", splitter, SLOT(saveItem()), actionCollection(), "saveItem");
    new KAction(i18n("Delete"), "editdelete", 0, splitter, SLOT(removeSelectedItems()), actionCollection(), "removeItem");
    
    // settings menu

    new KToggleAction(i18n("Show Menu Bar"), "CTRL+m", this, SLOT(slotToggleMenuBar()), actionCollection(), "toggleMenuBar");
    new KToggleAction(i18n("Show Tool Bar"), "CTRL+b", this, SLOT(slotToggleToolBar()), actionCollection(), "toggleToolBar");

    restoreOnLoadAction = new KToggleAction(i18n("Restore Playlists on Load"),  0, actionCollection(), "restoreOnLoad");

    toggleSystemTrayAction = new KToggleAction(i18n("Dock in System Tray"), 0, actionCollection(), "toggleSystemTray");
    connect(toggleSystemTrayAction, SIGNAL(toggled(bool)), this, SLOT(toggleSystemTray(bool)));

    new KAction(i18n("Genre List Editor..."), 0, this, SLOT(showGenreListEditor()), actionCollection(), "showGenreListEditor");


    outputSelectAction = Player::playerSelectAction(actionCollection());
    if(outputSelectAction) {
	outputSelectAction->setCurrentItem(0);
	connect(outputSelectAction, SIGNAL(activated(int)), this, SLOT(setOutput(int)));
    }

    // just in the toolbar
    sliderAction = new SliderAction(i18n("Track Position"), actionCollection(), "trackPositionAction");

    createGUI();

    // set the slider to the proper orientation and make it stay that way
    sliderAction->updateOrientation();
    connect(this, SIGNAL(dockWindowPositionChanged(QDockWindow *)), sliderAction, SLOT(updateOrientation(QDockWindow *)));

    connect(splitter, SIGNAL(playlistChanged()), this, SLOT(playlistChanged()));
}

void JuK::setupSystemTray()
{
    if(toggleSystemTrayAction && toggleSystemTrayAction->isChecked()) {
	systemTray = new SystemTray(this, "systemTray");
	systemTray->show();
	
	connect(systemTray, SIGNAL(play()),    this, SLOT(play()));
	connect(systemTray, SIGNAL(stop()),    this, SLOT(stop()));
	connect(systemTray, SIGNAL(pause()),   this, SLOT(pause()));
	connect(systemTray, SIGNAL(back()),    this, SLOT(back()));
	connect(systemTray, SIGNAL(forward()), this, SLOT(forward()));

	connect(this, SIGNAL(newSongSignal(const QString&)), systemTray, SLOT(slotNewSong(const QString&)));
	
	if(player && player->paused())
	    systemTray->slotPause();
	else if(player && player->playing())
	    systemTray->slotPlay();
    }
    else
	systemTray = 0;
}

void JuK::setupPlayer()
{
    trackPositionDragging = false;
    noSeek = false;
    pauseAction->setEnabled(false);
    stopAction->setEnabled(false);
    backAction->setEnabled(false);
    forwardAction->setEnabled(false);

    playTimer = new QTimer(this);
    connect(playTimer, SIGNAL(timeout()), this, SLOT(pollPlay()));

    if(sliderAction && sliderAction->getTrackPositionSlider() && sliderAction->getVolumeSlider()) {
        connect(sliderAction->getTrackPositionSlider(), SIGNAL(valueChanged(int)), this, SLOT(trackPositionSliderUpdate(int)));
        connect(sliderAction->getTrackPositionSlider(), SIGNAL(sliderPressed()), this, SLOT(trackPositionSliderClick()));
        connect(sliderAction->getTrackPositionSlider(), SIGNAL(sliderReleased()), this, SLOT(trackPositionSliderRelease()));
        sliderAction->getTrackPositionSlider()->setEnabled(false);

        connect(sliderAction->getVolumeSlider(), SIGNAL(valueChanged(int)), this, SLOT(setVolume(int)));
    }
    
    int playerType = 0;
    if(outputSelectAction) {
	playerType = outputSelectAction->currentItem();
	connect(outputSelectAction, SIGNAL(activated(int)), this, SLOT(setOutput(int)));
    }

    player = Player::createPlayer(playerType);
}


void JuK::processArgs()
{
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    QStringList files;
    
    for(int i = 0; i < args->count(); i++)
	files.append(args->arg(i));

    splitter->open(files);
}

/**
 * These are settings that need to be know before setting up the GUI.
 */

void JuK::readSettings()
{
    KConfig *config = KGlobal::config();
    { // general settings
        KConfigGroupSaver saver(config, "Settings");
	restore = config->readBoolEntry("RestoreOnLoad", true);
    }
}

void JuK::readConfig()
{
    // Automagically save and restore many window settings.
    setAutoSaveSettings();

    KConfig *config = KGlobal::config();
    { // player settings
        KConfigGroupSaver saver(config, "Player");
        if(sliderAction && sliderAction->getVolumeSlider()) {
            int volume = config->readNumEntry("Volume", sliderAction->getVolumeSlider()->maxValue());
            sliderAction->getVolumeSlider()->setValue(volume);
        }
	if(randomPlayAction) {
	    bool randomPlay = config->readBoolEntry("RandomPlay", false);
	    randomPlayAction->setChecked(randomPlay);
	}
    }
    { // view settings
        KConfigGroupSaver saver(config, "View");
	bool showEditor = config->readBoolEntry("ShowEditor", false);
	showEditorAction->setChecked(showEditor);
	splitter->setEditorVisible(showEditor);
    }
    { // general settings
        KConfigGroupSaver saver(config, "Settings");
	bool dockInSystemTray = config->readBoolEntry("DockInSystemTray", true);
	toggleSystemTrayAction->setChecked(dockInSystemTray);
	
	if(outputSelectAction)
	    outputSelectAction->setCurrentItem(config->readNumEntry("MediaSystem", 0));
	
    }

    if(restoreOnLoadAction)
	restoreOnLoadAction->setChecked(restore);
    
}

void JuK::saveConfig()
{
    KConfig *config = KGlobal::config();
    { // player settings
        KConfigGroupSaver saver(config, "Player");
        if(sliderAction && sliderAction->getVolumeSlider())
            config->writeEntry("Volume", sliderAction->getVolumeSlider()->value());
	if(randomPlayAction)
	    config->writeEntry("RandomPlay", randomPlayAction->isChecked());
    }
    { // view settings
        KConfigGroupSaver saver(config, "View");
	config->writeEntry("ShowEditor", showEditorAction->isChecked());
    }
    { // general settings
        KConfigGroupSaver saver(config, "Settings");
	if(restoreOnLoadAction)
	    config->writeEntry("RestoreOnLoad", restoreOnLoadAction->isChecked());
	if(toggleSystemTrayAction)
	    config->writeEntry("DockInSystemTray", toggleSystemTrayAction->isChecked());
	if(outputSelectAction)
	    config->writeEntry("MediaSystem", outputSelectAction->currentItem());
    }
}

bool JuK::queryClose()
{
    stop();
    delete player;
    Cache::instance()->save();
    saveConfig();
    delete splitter;
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
    
    if(splitter->playingArtist().isEmpty())
	s = splitter->playingTrack().simplifyWhiteSpace();
    else
	s = splitter->playingArtist().simplifyWhiteSpace() + " - " + splitter->playingTrack().simplifyWhiteSpace();

    return s;
}

////////////////////////////////////////////////////////////////////////////////
// private slot definitions
////////////////////////////////////////////////////////////////////////////////

void JuK::playlistChanged()
{
    if(splitter->collectionListSelected()) {
	savePlaylistAction->setEnabled(false);
	saveAsPlaylistAction->setEnabled(false);
	renamePlaylistAction->setEnabled(false);
	deleteItemPlaylistAction->setEnabled(false);	
    }
    else {
	savePlaylistAction->setEnabled(true);
	saveAsPlaylistAction->setEnabled(true);
	renamePlaylistAction->setEnabled(true);
	deleteItemPlaylistAction->setEnabled(true);
    }

    updatePlaylistInfo();
}

void JuK::updatePlaylistInfo()
{
    statusLabel->setPlaylistInfo(splitter->selectedPlaylistName(), splitter->selectedPlaylistCount());
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
    if(!player)
	return;

    if(player->paused()) {
        player->play();

	// Here, before doing anything, we want to make sure that the player did
	// in fact start.

        if(player->playing()) {
            pauseAction->setEnabled(true);
            stopAction->setEnabled(true);
            playTimer->start(pollInterval);
	    if(systemTray)
		systemTray->slotPlay();
        }
    }
    else if(player->playing())
	player->seekPosition(0);
    else
	play(splitter->playNextFile(randomPlayAction->isChecked()));
}

void JuK::pause()
{
    if(!player)
	return;

    playTimer->stop();
    player->pause();
    pauseAction->setEnabled(false);
    if(systemTray)
	systemTray->slotPause();
}

void JuK::stop()
{
    if(!player)
	return;

    playTimer->stop();
    player->stop();

    pauseAction->setEnabled(false);
    stopAction->setEnabled(false);
    backAction->setEnabled(false);
    forwardAction->setEnabled(false);

    sliderAction->getTrackPositionSlider()->setValue(0);
    sliderAction->getTrackPositionSlider()->setEnabled(false);

    splitter->stop();

    statusLabel->clear();
    
    if(systemTray)
	systemTray->slotStop();
}

void JuK::back()
{
    play(splitter->playPreviousFile(randomPlayAction->isChecked()));
}

void JuK::forward()
{
    play(splitter->playNextFile(randomPlayAction->isChecked()));
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
    if(enabled && !systemTray)
	setupSystemTray();
    else if(!enabled && systemTray) {
	delete systemTray;
	systemTray = 0;
    }
}

void JuK::setOutput(int output)
{
    stop();
    delete player;
    player = Player::createPlayer(output);
}

////////////////////////////////////////////////////////////////////////////////
// additional player slots
////////////////////////////////////////////////////////////////////////////////

void JuK::trackPositionSliderClick()
{
    trackPositionDragging = true;
}

void JuK::trackPositionSliderRelease()
{
    if(!player)
	return;

    trackPositionDragging = false;
    player->seekPosition(sliderAction->getTrackPositionSlider()->value());
}

void JuK::trackPositionSliderUpdate(int position)
{
    if(!player)
	return;

    if(player->playing() && !trackPositionDragging && !noSeek)
        player->seekPosition(position);
}

// This method is called when the play timer has expired.

void JuK::pollPlay()
{
    if(!player)
	return;

    // Our locking mechanism.  Since this method adjusts the play slider, we 
    // want to make sure that our adjustments
    noSeek = true;

    if(!player->playing()) {

        playTimer->stop();

	if(!player->paused())
	    play(splitter->playNextFile(randomPlayAction->isChecked()));

    }
    else if(!trackPositionDragging) {
        sliderAction->getTrackPositionSlider()->setValue(player->position());
	statusLabel->setItemTotalTime(player->totalTime());
	statusLabel->setItemCurrentTime(player->currentTime());
    }

    // Ok, this is weird stuff, but it works pretty well.  Ordinarily we don't
    // need to check up on our playing time very often, but in the span of the 
    // last interval, we want to check a lot -- to figure out that we've hit the
    // end of the song as soon as possible.

    if(player->playing() && player->totalTime() > 0 && float(player->totalTime() - player->currentTime()) < pollInterval * 2)
        playTimer->changeInterval(50);

    noSeek = false;
}

void JuK::setVolume(int volume)
{
    if(player && sliderAction && sliderAction->getVolumeSlider() &&
       sliderAction->getVolumeSlider()->maxValue() > 0 &&
       volume >= 0 && sliderAction->getVolumeSlider()->maxValue() >= volume)
    {
        player->setVolume(float(volume) / float(sliderAction->getVolumeSlider()->maxValue()));
    }
}

void JuK::play(const QString &file)
{
    if(!player)
	return;

    float volume = float(sliderAction->getVolumeSlider()->value()) / float(sliderAction->getVolumeSlider()->maxValue());

    if(player->paused())
	player->stop();
    
    player->play(file, volume);

    // Make sure that the player actually starts before doing anything.

    if(player->playing()) {
	pauseAction->setEnabled(true);
	stopAction->setEnabled(true);
	
	backAction->setEnabled(true);
	forwardAction->setEnabled(true);
	
	sliderAction->getTrackPositionSlider()->setValue(0);
	sliderAction->getTrackPositionSlider()->setEnabled(true);
	playTimer->start(pollInterval);

	statusLabel->setPlayingItemInfo(playingString(), splitter->playingList());

	emit newSongSignal(playingString());

	if(systemTray)
	    systemTray->slotPlay();
    }
    else
	stop();
}

#include "juk.moc"
