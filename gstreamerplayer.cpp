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

#include <config.h>

#if HAVE_GSTREAMER

#include <kdebug.h>

#include <qfile.h>

#include "gstreamerplayer.h"

using namespace KDE::GST;
using namespace KDE::GSTPlay;

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

GStreamerPlayer::GStreamerPlayer() : Player(),
                                     m_positionNs(0), m_durationNs(0), m_currentVolume(1.0)
{
    setupPlayer();
}

GStreamerPlayer::~GStreamerPlayer()
{
    delete m_player;
}

void GStreamerPlayer::play(const FileHandle &file)
{
    m_currentFile = file.absFilePath();
    m_positionNs = 0;
    m_durationNs = 0;

    if(!file.isNull())
        m_player->setLocation(file.absFilePath());

    if(m_player->getState() != Element::STATE_PLAYING)
        m_player->setState(Element::STATE_PLAYING);
}

void GStreamerPlayer::pause()
{
    if(m_player->getState() != Element::STATE_PAUSED)
        m_player->setState(Element::STATE_PAUSED);
}

void GStreamerPlayer::stop()
{
    if(m_player->getState() != Element::STATE_READY)
        m_player->setState(Element::STATE_READY);
}

void GStreamerPlayer::setVolume(float volume)
{
    // 1.0 is full volume
    m_player->setVolume(volume);
}

float GStreamerPlayer::volume() const
{
    // 1.0 is full volume
    return m_player->getVolume();
}

/////////////////////////////////////////////////////////////////////////////////
// m_player status functions
/////////////////////////////////////////////////////////////////////////////////

bool GStreamerPlayer::playing() const
{
    // true if playing
    return m_player->getState() == Element::STATE_PLAYING;
}

bool GStreamerPlayer::paused() const
{
    // true if paused
    return m_player->getState() == Element::STATE_PAUSED;
}

long GStreamerPlayer::totalTime() const
{
    return m_durationNs / 1000000000L;
}

long GStreamerPlayer::currentTime() const
{
    return m_positionNs / 1000000000L;
}

int GStreamerPlayer::position() const
{
    if (m_durationNs > 0)
        return int((m_positionNs * 1000.0) / m_durationNs);
    else
        return 0;
}

/////////////////////////////////////////////////////////////////////////////////
// m_player seek functions
/////////////////////////////////////////////////////////////////////////////////

void GStreamerPlayer::seek(long seekTime)
{
    // seek time in seconds?
    m_player->seekToTime(seekTime*1000000000);
}

void GStreamerPlayer::seekPosition(int position)
{
    // position unit is 1/1000th
    if(m_durationNs > 0)
        m_player->seekToTime(position * m_durationNs / 1000L);
    else
        m_player->seekToTime(0);

}

/////////////////////////////////////////////////////////////////////////////////
// private
/////////////////////////////////////////////////////////////////////////////////

void GStreamerPlayer::setupPlayer()
{
    m_player = new Play(Play::PIPE_AUDIO_BUFFER_THREADED, this, "Play");
    connect(m_player, SIGNAL(timeTick(long long)),
            SLOT(slotSetPosition(long long)));
    connect(m_player, SIGNAL(streamLength(long long)),
            SLOT(slotSetDuration(long long)));
    connect(m_player, SIGNAL(streamEnd()), SLOT(slotStopIfNotPlaying()));
}

void GStreamerPlayer::slotStopIfNotPlaying()
{
    if(!playing())
        stop();
}

#include "gstreamerplayer.moc"

#endif
