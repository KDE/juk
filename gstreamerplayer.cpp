/***************************************************************************
                          gstreamerplayer.cpp  -  description
                             -------------------
    begin                : Sat Feb 9 2003
    copyright            : (C) 2003 by Tim Jansen
    email                : tim@tjansen.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "../config.h"

#if HAVE_GSTREAMER

#include <kdebug.h>

#include <qfile.h>

#include <connect.h>
#include <flowsystem.h>

#include "gstreamerplayer.h"

using namespace QGst;
using namespace QGstPlay;

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

GStreamerPlayer::GStreamerPlayer() : QObject(0), Player()
{
    currentVolume = 1.0;
    positionNs = 0;
    durationNs = 0;

    setupPlayer();
}

GStreamerPlayer::~GStreamerPlayer()
{
    delete(player);
}

void GStreamerPlayer::play(const QString &fileName, float volume)
{
    currentFile = fileName;
    if(!fileName.isNull())
	player->setLocation(fileName);  

    play(volume);
}

void GStreamerPlayer::play(float volume)
{
    // 1.0 is full volume
    positionNs = 0;
    durationNs = 0;
    if (player->getState() != Element::STATE_PLAYING)
	player->setState(Element::STATE_PLAYING);

}

void GStreamerPlayer::pause()
{
    if(player->getState() != Element::STATE_PAUSED)
	player->setState(Element::STATE_PAUSED);
}

void GStreamerPlayer::stop()
{
    if(player->getState() != Element::STATE_READY)
	player->setState(Element::STATE_READY);

}

void GStreamerPlayer::setVolume(float volume)
{
    // 1.0 is full volume
    player->setVolume(volume);
}

float GStreamerPlayer::getVolume() const
{
    // 1.0 is full volume
    return player->getVolume();
}

/////////////////////////////////////////////////////////////////////////////////
// player status functions
/////////////////////////////////////////////////////////////////////////////////

bool GStreamerPlayer::playing() const
{
    // true if playing
    return player->getState() == Element::STATE_PLAYING;
}

bool GStreamerPlayer::paused() const
{
    // true if paused
    return player->getState() == Element::STATE_PAUSED;
}

long GStreamerPlayer::totalTime() const
{
    return durationNs / 1000000000L;
}

long GStreamerPlayer::currentTime() const
{
    return positionNs / 1000000000L;
}

int GStreamerPlayer::position() const
{
    if (durationNs > 0)
	return (int)((positionNs * 1000.0) / durationNs);
    else
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////
// player seek functions
/////////////////////////////////////////////////////////////////////////////////

void GStreamerPlayer::seek(long seekTime)
{
    // seek time in seconds?
    player->seekToTime(seekTime*1000000000);
}

void GStreamerPlayer::seekPosition(int position)
{
    // position unit is 1/1000th
    if(durationNs > 0)
	player->seekToTime(position * durationNs / 1000L);
    else
	player->seekToTime(0);

}

/////////////////////////////////////////////////////////////////////////////////
// private
/////////////////////////////////////////////////////////////////////////////////

void GStreamerPlayer::setDuration(long long d) 
{
    durationNs = d;
}

void GStreamerPlayer::setPosition(long long d)
{
    positionNs = d;
}

void GStreamerPlayer::setupPlayer()
{
    player = new Play(Play::PIPE_AUDIO_THREADED, this, "Play");
    connect(player, SIGNAL(timeTick(long long)), 
	    SLOT(setPosition(long long)));
    connect(player, SIGNAL(streamLength(long long)), 
	    SLOT(setDuration(long long)));
    connect(player, SIGNAL(streamEnd()), SLOT(stop()));
}

#include "gstreamerplayer.moc"

#endif
