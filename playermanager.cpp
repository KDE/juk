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

#include <kdebug.h>

#include <qslider.h>
#include <qtimer.h>

#include "playermanager.h"
#include "playlistinterface.h"
#include "slideraction.h"
#include "statuslabel.h"
#include "actioncollection.h"

using namespace ActionCollection;

PlayerManager *PlayerManager::m_instance = 0;

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

PlayerManager::PlayerManager() :
    QObject(0, "PlayerManager"),
    Player(),
    m_sliderAction(0),
    m_playlistInterface(0),
    m_statusLabel(0),
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
    if(!player())
        return false;

    return player()->playing();
}

bool PlayerManager::paused() const
{
    if(!player())
        return false;

    return player()->paused();
}

float PlayerManager::volume() const
{
    if(!player())
        return 0;

    return player()->volume();
}

long PlayerManager::totalTime() const
{
    if(!player())
        return 0;

    return player()->totalTime();
}

long PlayerManager::currentTime() const
{
    if(!player())
        return 0;

    return player()->currentTime();
}

int PlayerManager::position() const
{
    if(!player())
        return 0;

    return player()->position();
}

void PlayerManager::setPlaylistInterface(PlaylistInterface *interface)
{
    m_playlistInterface = interface;
}

void PlayerManager::setStatusLabel(StatusLabel *label)
{
    m_statusLabel = label;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::play(const QString &fileName)
{
    if(!player() || !m_playlistInterface)
        return;

    if(fileName.isNull()) {
	if(player()->paused())
            player()->play();
        else if(player()->playing())
            player()->seekPosition(0);
        else {
            QString file = m_playlistInterface->currentFile();
            if(!file.isNull())
                player()->play(file);
	}
    }
    else
        player()->play(fileName);

    // Make sure that the player() actually starts before doing anything.

    if(!player()->playing()) {
        stop();
        return;
    }

    action("pause")->setEnabled(true);
    action("stop")->setEnabled(true);
    action("forward")->setEnabled(true);
    action("back")->setEnabled(true);

    m_sliderAction->trackPositionSlider()->setValue(0);
    m_sliderAction->trackPositionSlider()->setEnabled(true);

    m_timer->start(m_pollInterval);
}

void PlayerManager::pause()
{
    if(!player())
        return;

    if(player()->paused()) {
        play();
        return;
    }

    m_timer->stop();
    action("pause")->setEnabled(false);

    player()->pause();
}

void PlayerManager::stop()
{
    if(!player())
        return;

    m_timer->stop();

    action("pause")->setEnabled(false);
    action("stop")->setEnabled(false);
    action("back")->setEnabled(false);
    action("forward")->setEnabled(false);

    m_sliderAction->trackPositionSlider()->setValue(0);
    m_sliderAction->trackPositionSlider()->setEnabled(false);

    m_statusLabel->clear();

    player()->stop();
}

void PlayerManager::setVolume(float volume)
{
    if(!player())
        return;

    player()->setVolume(volume);
}

void PlayerManager::seek(long seekTime)
{
    if(!player())
        return;

    player()->seek(seekTime);
}

void PlayerManager::seekPosition(int position)
{
    if(!player())
        return;

    if(!player()->playing() || m_noSeek)
        return;

    slotUpdateTime(position);
    player()->seekPosition(position);
}

void PlayerManager::forward()
{
    QString file = m_playlistInterface->nextFile();
    if(!file.isNull())
        play(file);
    else
        stop();
}

void PlayerManager::back()
{
    QString file = m_playlistInterface->previousFile();
    if(!file.isNull())
        play(file);
    else
        stop();
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void PlayerManager::slotPollPlay()
{
    if(!player() || !m_playlistInterface)
        return;

    m_noSeek = true;

    if(!player()->playing()) {
        m_timer->stop();
        QString nextFile = m_playlistInterface->nextFile();
        if(!nextFile.isNull())
            play(nextFile);
        else
            stop();
    }
    else if(!m_sliderAction->dragging()) {
        m_sliderAction->trackPositionSlider()->setValue(player()->position());

        if(m_statusLabel) {
            m_statusLabel->setItemTotalTime(player()->totalTime());
            m_statusLabel->setItemCurrentTime(player()->currentTime());
        }
    }

    // Ok, this is weird stuff, but it works pretty well.  Ordinarily we don't
    // need to check up on our playing time very often, but in the span of the
    // last interval, we want to check a lot -- to figure out that we've hit the
    // end of the song as soon as possible.

    if(player()->playing() &&
       player()->totalTime() > 0 &&
       float(player()->totalTime() - player()->currentTime()) < m_pollInterval * 2)
    {
        m_timer->changeInterval(50);
    }

    m_noSeek = false;
}

void PlayerManager::slotSetOutput(int system)
{
    stop();
    delete m_player;
    m_player = Player::createPlayer(system);
}

void PlayerManager::slotSetVolume(int volume)
{
    setVolume(float(volume) / float(m_sliderAction->volumeSlider()->maxValue()));
}

void PlayerManager::slotUpdateTime(int position)
{
    if(!m_statusLabel)
        return;

    float positionFraction = float(position) / m_sliderAction->trackPositionSlider()->maxValue();
    float totalTime = float(m_player->totalTime());
    long seekTime = long(positionFraction * totalTime + 0.5); // "+0.5" for rounding

    m_statusLabel->setItemCurrentTime(seekTime);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

Player *PlayerManager::player() const
{
    if(!m_player)
        instance()->setup();

    return m_player;
}

void PlayerManager::setup()
{
    // All of the actions required by this class should be listed here.

    if(!action("pause") ||
       !action("stop") ||
       !action("back") ||
       !action("forward") ||
       !action("trackPositionAction"))

    {
        kdWarning(65432) << k_funcinfo << "Could not find all of the required actions." << endl;
        return;
    }

    // initialize action states

    action("pause")->setEnabled(false);
    action("stop")->setEnabled(false);
    action("back")->setEnabled(false);
    action("forward")->setEnabled(false);

    // setup sliders

    m_sliderAction = action<SliderAction>("trackPositionAction");

    connect(m_sliderAction, SIGNAL(signalPositionChanged(int)),
            this, SLOT(seekPosition(int)));
    connect(m_sliderAction->trackPositionSlider(), SIGNAL(valueChanged(int)),
            this, SLOT(slotUpdateTime(int)));
    connect(m_sliderAction->volumeSlider(), SIGNAL(valueChanged(int)),
            this, SLOT(slotSetVolume(int)));

    KAction *outputAction = action("outputSelect");

    if(outputAction) {
        int mediaSystem = static_cast<KSelectAction *>(outputAction)->currentItem();
        m_player = Player::createPlayer(mediaSystem);
        connect(outputAction, SIGNAL(activated(int)), this, SLOT(slotSetOutput(int)));
    }
    else
        m_player = Player::createPlayer();

    float volume =
        float(m_sliderAction->volumeSlider()->value()) /
	float(m_sliderAction->volumeSlider()->maxValue());

    m_player->setVolume(volume);

    m_timer = new QTimer(this, "play timer");
    connect(m_timer, SIGNAL(timeout()), this, SLOT(slotPollPlay()));
}

#include "playermanager.moc"
