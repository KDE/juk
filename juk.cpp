/***************************************************************************
                          juk.cpp  -  description
                             -------------------
    begin                : Mon Feb  4 23:40:41 EST 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <klocale.h>
#include <keditcl.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <kdebug.h>

#include "juk.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

JuK::JuK(QWidget *parent, const char *name) : KMainWindow(parent, name)
{
  setupActions();
  setupLayout();
  setupPlayer();
  readConfig();
}

JuK::~JuK()
{
  saveConfig();
  delete(playTimer);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void JuK::setupActions()
{
  // file menu
  KStdAction::open(this, SLOT(openFile()), actionCollection());
  (void) new KAction(i18n("Open &Directory..."), "fileopen", 0, this, SLOT(openDirectory()), actionCollection(), "openDirectory");
  KStdAction::save(this, SLOT(saveFile()), actionCollection());
  (void) new KAction(i18n("Delete"), "edittrash", 0, this, SLOT(deleteFile()), actionCollection(), "deleteFile");
  KStdAction::quit(this, SLOT(quit()), actionCollection());
  
  // edit menu
  KStdAction::cut(this, SLOT(cut()), actionCollection());
  KStdAction::copy(this, SLOT(copy()), actionCollection());
  KStdAction::paste(this, SLOT(paste()), actionCollection());
  KStdAction::selectAll(this, SLOT(selectAll()), actionCollection());
 
  // play menu
  (void) new KAction(i18n("&Add to Playlist"), "enqueue", 0, this, SLOT(addToPlaylist()), actionCollection(), "addToPlaylist");
  (void) new KAction(i18n("&Remove from Playlist"), "dequeue", 0, this, SLOT(removeFromPlaylist()), actionCollection(), "removeFromPlaylist");
  playAction = new KAction(i18n("&Play"), "1rightarrow", 0, this, SLOT(playFile()), actionCollection(), "playFile");
  pauseAction = new KAction(i18n("P&ause"), "player_pause", 0, this, SLOT(pauseFile()), actionCollection(), "pauseFile");
  stopAction = new KAction(i18n("&Stop"), "player_stop", 0, this, SLOT(stopFile()), actionCollection(), "stopFile");

  // function menu

  (void) new KAction(i18n("Tagger"), "tag", 0, this, SLOT(showTagger()), actionCollection(), "showTagger");
  (void) new KAction(i18n("Playlist Editor"), "edit", 0, this, SLOT(showPlaylist()), actionCollection(), "showPlaylist");
  

  // just in the toolbar
  sliderAction = new SliderAction(i18n("Track Position"), actionCollection(), "trackPositionAction");

  createGUI();
}

void JuK::setupLayout()
{
  // automagically save and restore settings
  this->setAutoSaveSettings();

  // create the main widgets
  tagger = new TaggerWidget(this);
  playlist = new PlaylistWidget(this);
  
  showTagger();

  // set the slider to the proper orientation and make it stay that way
  sliderAction->updateOrientation();
  connect(this, SIGNAL(dockWindowPositionChanged(QDockWindow *)), sliderAction, SLOT(updateOrientation(QDockWindow *)));

  // playlist item activation connection
  connect(playlist->getPlaylistList(), SIGNAL(doubleClicked(QListViewItem *)), this, SLOT(playItem(QListViewItem *)));
}

void JuK::setupPlayer()
{
  trackPositionDragging=false;
  noSeek=false;
  pauseAction->setEnabled(false);
  stopAction->setEnabled(false);

  playTimer=new QTimer(this);
  connect(playTimer, SIGNAL(timeout()), this, SLOT(pollPlay()));

  if(sliderAction && sliderAction->getTrackPositionSlider() && sliderAction->getVolumeSlider()) {
    connect(sliderAction->getTrackPositionSlider(), SIGNAL(valueChanged(int)), this, SLOT(trackPositionSliderUpdate(int)));
    connect(sliderAction->getTrackPositionSlider(), SIGNAL(sliderPressed()), this, SLOT(trackPositionSliderClick()));
    connect(sliderAction->getTrackPositionSlider(), SIGNAL(sliderReleased()), this, SLOT(trackPositionSliderRelease()));    
    sliderAction->getTrackPositionSlider()->setEnabled(false);

    connect(sliderAction->getVolumeSlider(), SIGNAL(valueChanged(int)), this, SLOT(setVolume(int)));
  }
}

void JuK::readConfig()
{
  KConfig *config = KGlobal::config();
  { // player settings
    KConfigGroupSaver saver(config, "Player");
    if(sliderAction && sliderAction->getVolumeSlider()) {
      int volume = config->readNumEntry("Volume", sliderAction->getVolumeSlider()->maxValue());
      sliderAction->getVolumeSlider()->setValue(volume);
    }
  }
}

void JuK::saveConfig()
{
  KConfig *config = KGlobal::config();
  { // player settings
    KConfigGroupSaver saver(config, "Player");
    if(sliderAction && sliderAction->getVolumeSlider())
      config->writeEntry("Volume", sliderAction->getVolumeSlider()->value());
  }  
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// private slot definitions
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// private action slot implementations - file menu
////////////////////////////////////////////////////////////////////////////////

void JuK::openFile()
{
  QStringList files = KFileDialog::getOpenFileNames(QString::null, "*.mp3|MPEG Audio (*.mp3)");
  tagger->add(&files);
}

void JuK::openDirectory()
{
  tagger->add(KFileDialog::getExistingDirectory());
}

void JuK::saveFile()
{
  if(tagger && tagger->isVisible())
    tagger->save();
}

void JuK::quit()
{
  delete(this);
}

////////////////////////////////////////////////////////////////////////////////
// function menu
////////////////////////////////////////////////////////////////////////////////

void JuK::showTagger()
{
  playlist->hide();
  tagger->show();
  setCentralWidget(tagger);
}

void JuK::showPlaylist()
{
  tagger->hide();
  playlist->show();
  setCentralWidget(playlist);
}

////////////////////////////////////////////////////////////////////////////////
// player menu
////////////////////////////////////////////////////////////////////////////////

void JuK::addToPlaylist()
{
  playlist->add(tagger->getSelectedItems());
}

void JuK::removeFromPlaylist()
{

}

void JuK::playFile()
{
  if(playlist) {
    if(playlist->getSelectedItems()->count() > 0)
      playItem(dynamic_cast<FileListItem *>(playlist->getSelectedItems()->at(0)));
    else
      playItem(playlist->firstItem());
  }
}

void JuK::pauseFile()
{
  playTimer->stop();
  player.pause();
  playAction->setEnabled(true);
  pauseAction->setEnabled(false);
}

void JuK::stopFile()
{
  playTimer->stop();
  player.stop();
  playAction->setEnabled(true);
  pauseAction->setEnabled(false);
  stopAction->setEnabled(false);
  sliderAction->getTrackPositionSlider()->setValue(0);
  sliderAction->getTrackPositionSlider()->setEnabled(false);
  playingItem->setPixmap(0, 0);
}

void JuK::trackPositionSliderClick()
{
  trackPositionDragging=true;
}

void JuK::trackPositionSliderRelease()
{
  trackPositionDragging=false;
  player.seekPosition(sliderAction->getTrackPositionSlider()->value());
}

void JuK::trackPositionSliderUpdate(int position)
{
  if(player.playing() && !trackPositionDragging && !noSeek) {
    player.seekPosition(position);
  }
}

void JuK::pollPlay()
{
  noSeek = true;
  if(!player.playing()) {
    playTimer->stop();
    if(player.paused()) {
      pauseFile();
    }
    else {
      if(playingItem && dynamic_cast<FileListItem*>(playingItem->itemBelow())) {
	playingItem->setPixmap(0, 0);
        playingItem = dynamic_cast<FileListItem *>(playingItem->itemBelow());
	sliderAction->getTrackPositionSlider()->setValue(0);
	player.play(playingItem->absFilePath(), player.getVolume());
	if(player.playing()) {
	  playTimer->start(pollInterval);
	  playingItem->setPixmap(0, QPixmap(UserIcon("playing")));
	}
      }
      else
	stopFile();
    }
  }
  else if(!trackPositionDragging) {
    //    kdDebug() << player.position() << " - " << sliderAction->getTrackPositionSlider()->maxValue() << endl;
    sliderAction->getTrackPositionSlider()->setValue(player.position());
  }

  if(player.playing() && float(player.totalTime() - player.currentTime()) < pollInterval * 2)
    playTimer->changeInterval(50);

  noSeek=false;
}

void JuK::setVolume(int volume)
{
  if(sliderAction && sliderAction->getVolumeSlider() && 
     sliderAction->getVolumeSlider()->maxValue() > 0 &&
     volume >= 0 && sliderAction->getVolumeSlider()->maxValue() >= volume)
    {
      player.setVolume(float(volume) / float(sliderAction->getVolumeSlider()->maxValue()));
    }
}

void JuK::playItem(QListViewItem *item)
{
  FileListItem *fileListItem = dynamic_cast<FileListItem *>(item);
  if(fileListItem)
    playItem(fileListItem);
}

void JuK::playItem(FileListItem *item)
{
  if(player.playing() || player.paused())
    stopFile();
  
  if(item) {
    playingItem = item;
    float volume = float(sliderAction->getVolumeSlider()->value()) / float(sliderAction->getVolumeSlider()->maxValue());  
    player.play(playingItem->absFilePath(), volume);
    if(player.playing()) {
      playAction->setEnabled(false);
      pauseAction->setEnabled(true);
      stopAction->setEnabled(true);
      sliderAction->getTrackPositionSlider()->setEnabled(true);
      playingItem->setPixmap(0, QPixmap(UserIcon("playing")));
      playTimer->start(pollInterval);
    }
  }
}
