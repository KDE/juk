/***************************************************************************
                          playermanager.cpp
                             -------------------
    begin                : Sat Feb 14 2004
    copyright            : (C) 2004 by Scott Wheeler
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

#include <kactioncollection.h>
#include <kmainwindow.h>
#include <kapplication.h>
#include <kdebug.h>

#include <qslider.h>
#include <qtimer.h>

#include "playermanager.h"
#include "slideraction.h"

PlayerManager *PlayerManager::m_instance = 0;

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

PlayerManager::PlayerManager() :
    QObject(0, "PlayerManager"),
    Player(),
    m_actionCollection(0),
    m_sliderAction(0),
    m_playlistInterface(0),
    m_player(0),
    m_timer(0),
    m_noSeek(false)
{
    setup();
}

PlayerManager::~PlayerManager()
{
    delete m_player;
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

PlayerManager *PlayerManager::instance() // static
{
    if(!m_instance)
        m_instance = new PlayerManager;
    return m_instance;
}

bool PlayerManager::playing() const
{
    if(!m_player)
        return false;

    return m_player->playing();
}

bool PlayerManager::paused() const
{
    if(!m_player)
        return false;

    return m_player->paused();
}

float PlayerManager::volume() const
{
    if(!m_player)
        return 0;

    return m_player->volume();
}

long PlayerManager::totalTime() const
{
    if(!m_player)
        return 0;

    return m_player->totalTime();
}

long PlayerManager::currentTime() const
{
    if(!m_player)
        return 0;

    return m_player->currentTime();
}

int PlayerManager::position() const
{
    if(!m_player)
        return 0;

    return m_player->position();
}

void PlayerManager::setPlaylistInterface(const PlaylistInterface *interface)
{
    m_playlistInterface = interface;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::play(const QString &fileName)
{
    if(!m_player || !m_playlistInterface)
        return;

    if(fileName.isNull()) {
	if(m_player->paused())
            m_player->play();
        if(m_player->playing())
            m_player->seekPosition(0);
        else
            m_player->play(m_playlistInterface->nextFile());
    }
    else {
        if(m_player->paused())
            m_player->stop();
        m_player->play(fileName);
    }

    // Make sure that the m_player actually starts before doing anything.

    if(!m_player->playing()) {
        stop();
        return;
    }

    m_actionCollection->action("pause")->setEnabled(true);
    m_actionCollection->action("stop")->setEnabled(true);
    m_actionCollection->action("forward")->setEnabled(true);
    m_actionCollection->action("back")->setEnabled(true);

    m_sliderAction->trackPositionSlider()->setValue(0);
    m_sliderAction->trackPositionSlider()->setEnabled(true);

    m_timer->start(m_pollInterval);
}

void PlayerManager::pause()
{
    if(!m_player)
        return;

    m_player->pause();
}

void PlayerManager::stop()
{
    if(!m_player)
        return;

    m_player->stop();
}

void PlayerManager::setVolume(float volume)
{
    if(!m_player)
        return;

    m_player->setVolume(volume);
}

void PlayerManager::seek(long seekTime)
{
    if(!m_player)
        return;

    m_player->seek(seekTime);
}

void PlayerManager::seekPosition(int position)
{
    if(!m_player)
        return;

    m_player->seekPosition(position);
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::slotPollPlay()
{
    if(!m_player || !m_playlistInterface)
        return;

    m_noSeek = true;

    if(!m_player->playing()) {
        m_timer->stop();
        play(m_playlistInterface->nextFile());
    }
    else if(!m_sliderAction->dragging()) {
        m_sliderAction->trackPositionSlider()->setValue(m_player->position());

        // TODO: figure out something to do with the status bar interface

        // m_statusLabel->setItemTotalTime(m_player->totalTime());
        // m_statusLabel->setItemCurrentTime(m_player->currentTime());
    }

    // Ok, this is weird stuff, but it works pretty well.  Ordinarily we don't
    // need to check up on our playing time very often, but in the span of the
    // last interval, we want to check a lot -- to figure out that we've hit the
    // end of the song as soon as possible.

    if(m_player->playing() &&
       m_player->totalTime() > 0 &&
       float(m_player->totalTime() - m_player->currentTime()) < m_pollInterval * 2)
    {
        m_timer->changeInterval(50);
    }

    m_noSeek = false;
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::setup()
{
    // Since we're doing a little wizardry to keep the interaction and API
    // requirements as light as possible we want to check everything we're
    // going to need to make sure that everything's in order.

    KMainWindow *mainWindow = dynamic_cast<KMainWindow *>(kapp->mainWidget());

    if(!mainWindow) {
        kdWarning(65432) << k_funcinfo << "Could not find main window." << endl;
        return;
    }

    m_actionCollection = mainWindow->actionCollection();

    if(!m_actionCollection) {
        kdWarning(65432) << k_funcinfo << "Action collection is null." << endl;
        return;
    }

    // All of the actions required by this class should be listed here.

    if(!m_actionCollection->action("pause") ||
       !m_actionCollection->action("stop") ||
       !m_actionCollection->action("back") ||
       !m_actionCollection->action("forward") ||
       !m_actionCollection->action("trackPositionAction"))

    {
        kdWarning(65432) << k_funcinfo << "Could not find all of the required actions." << endl;
        return;
    }

    m_actionCollection->action("pause")->setEnabled(false);
    m_actionCollection->action("stop")->setEnabled(false);
    m_actionCollection->action("back")->setEnabled(false);
    m_actionCollection->action("forward")->setEnabled(false);

    m_sliderAction = static_cast<SliderAction *>(m_actionCollection->action("trackPositionAction"));

    KAction *outputAction = m_actionCollection->action("outputSelect");

    if(outputAction) {
        int mediaSystem = static_cast<KSelectAction *>(outputAction)->currentItem();
        m_player = Player::createPlayer(mediaSystem);
    }
    else
        m_player = Player::createPlayer();

    m_timer = new QTimer(this, "play timer");
    connect(m_timer, SIGNAL(timeout()), this, SLOT(slotPollPlay()));
}

#include "playermanager.moc"
