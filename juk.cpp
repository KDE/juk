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
#include <kcmdlineargs.h>
#include <kdebug.h>

#include <qsplitter.h>

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
    processArgs();
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
    (void) new KAction(i18n("Delete"), "edittrash", 0, this, SLOT(remove()), actionCollection(), "remove");
    KStdAction::quit(this, SLOT(quit()), actionCollection());

    // edit menu
    KStdAction::cut(this, SLOT(cut()), actionCollection());
    KStdAction::copy(this, SLOT(copy()), actionCollection());
    KStdAction::paste(this, SLOT(paste()), actionCollection());
    KStdAction::selectAll(this, SLOT(selectAll()), actionCollection());

    // play menu
    playAction = new KAction(i18n("&Play"), "1rightarrow", 0, this, SLOT(playFile()), actionCollection(), "playFile");
    pauseAction = new KAction(i18n("P&ause"), "player_pause", 0, this, SLOT(pauseFile()), actionCollection(), "pauseFile");
    stopAction = new KAction(i18n("&Stop"), "player_stop", 0, this, SLOT(stopFile()), actionCollection(), "stopFile");

    // just in the toolbar
    sliderAction = new SliderAction(i18n("Track Position"), actionCollection(), "trackPositionAction");

    createGUI();
}

void JuK::setupLayout()
{
    // automagically save and restore settings
    setAutoSaveSettings();

    splitter = new PlaylistSplitter(this, "playlistSplitter");
    setCentralWidget(splitter);

    // set the slider to the proper orientation and make it stay that way
    sliderAction->updateOrientation();
    connect(this, SIGNAL(dockWindowPositionChanged(QDockWindow *)), sliderAction, SLOT(updateOrientation(QDockWindow *)));

    // playlist item activation connection
    connect(splitter, SIGNAL(playlistDoubleClicked(QListViewItem *)), this, SLOT(playItem(QListViewItem *)));
}

void JuK::setupPlayer()
{
    trackPositionDragging = false;
    noSeek = false;
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

void JuK::processArgs()
{
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    QStringList files;
    
    for(int i = 0; i < args->count(); i++)
	files.append(args->arg(i));

    splitter->open(files);
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
    splitter->open(files);
}

void JuK::openDirectory()
{
    splitter->open(KFileDialog::getExistingDirectory());
}

void JuK::saveFile()
{
    splitter->save();
}

void JuK::remove()
{
    QPtrList<PlaylistItem> items(splitter->playlistSelection());
    PlaylistItem *item = items.first();
    while(item) {
	if(item == playingItem)
	    playingItem = 0;
	item = items.next();
    }
    
    splitter->remove();
}

void JuK::quit()
{
    delete(this);
}

////////////////////////////////////////////////////////////////////////////////
// edit menu
////////////////////////////////////////////////////////////////////////////////

void JuK::cut()
{
    QPtrList<PlaylistItem> items = splitter->playlistSelection();

    PlaylistItem *item = items.first();
    while(item) {
	if(item == playingItem)
	    playingItem = 0;
	item = items.next();
    }

    splitter->clearSelectedItems();
}

void JuK::selectAll(bool select)
{
    splitter->selectAll(select);
}

////////////////////////////////////////////////////////////////////////////////
// player menu
////////////////////////////////////////////////////////////////////////////////

void JuK::playFile()
{
    if(player.paused()) {
        player.play();
        if(player.playing()) {
            playAction->setEnabled(false);
            pauseAction->setEnabled(true);
            stopAction->setEnabled(true);
            playTimer->start(pollInterval);
        }
    }
    else if(splitter) {
        QPtrList<PlaylistItem> items = splitter->playlistSelection();
        if(items.count() > 0)
            playItem(items.at(0));
        else
            playItem(splitter->playlistFirstItem());
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
    if(playingItem)
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
    if(player.playing() && !trackPositionDragging && !noSeek)
        player.seekPosition(position);
}

void JuK::pollPlay()
{
    noSeek = true;
    if(!player.playing()) {
        playTimer->stop();
        if(player.paused())
            pauseFile();
        else {
            if(playingItem && dynamic_cast<PlaylistItem *>(playingItem->itemBelow())) {
                playingItem->setPixmap(0, 0);
                playingItem = dynamic_cast<PlaylistItem *>(playingItem->itemBelow());
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
    else if(!trackPositionDragging)
        sliderAction->getTrackPositionSlider()->setValue(player.position());

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
    PlaylistItem *fileListItem = dynamic_cast<PlaylistItem *>(item);
    if(fileListItem)
        playItem(fileListItem);
}

void JuK::playItem(PlaylistItem *item)
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

void JuK::playTaggerItem(QListViewItem *item)
{

}

void JuK::playTaggerItem(PlaylistItem *item)
{

}

#include "juk.moc"
