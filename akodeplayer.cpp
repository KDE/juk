/***************************************************************************
    copyright            : (C) 2004 by Allan Sandfeld Jensen
    email                : kde@carewolf.com
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

#ifdef HAVE_AKODE

#include <kdebug.h>

#include <qfile.h>

#include <akode/player.h>
#include <akode/decoder.h>

#include "akodeplayer.h"

using namespace aKode;

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

aKodePlayer::aKodePlayer() : Player(),
                             m_player(0),
                             m_volume(1.0)
{}

aKodePlayer::~aKodePlayer()
{
    delete m_player;
}

void aKodePlayer::play(const FileHandle &file)
{
    kdDebug( 65432 ) << k_funcinfo << endl;

    if (file.isNull()) { // null FileHandle file means unpause
        if (paused())
            m_player->resume();
        else
            stop();
        return;
    }

    QString filename = file.absFilePath();

    kdDebug( 65432 ) << "Opening: " << filename << endl;

    if (m_player)
        m_player->stop();
    else {
        m_player = new aKode::Player();
        m_player->open("auto");
        m_player->setVolume(m_volume);
    }

    if (m_player->load(filename.local8Bit().data()))
        m_player->play();

}

void aKodePlayer::pause()
{
    if (m_player)
        m_player->pause();
}

void aKodePlayer::stop()
{
    if (m_player) {
        m_player->stop();
        m_player->unload();
    }
}

void aKodePlayer::setVolume(float volume)
{
    m_volume = volume;

    if (m_player)
        m_player->setVolume(m_volume);
}

float aKodePlayer::volume() const
{
    return m_volume;
}

/////////////////////////////////////////////////////////////////////////////////
// m_player status functions
/////////////////////////////////////////////////////////////////////////////////

bool aKodePlayer::playing() const
{
    if (m_player && m_player->decoder() && m_player->state() == aKode::Player::Playing)
        return !m_player->decoder()->eof();
    else
        return false;
}

bool aKodePlayer::paused() const
{
    return m_player && (m_player->state() == aKode::Player::Paused);
}

int aKodePlayer::totalTime() const
{
    if (m_player) {
        Decoder *d = m_player->decoder();
        if (d)
            return d->length() / 1000;
    }
    return -1;
}

int aKodePlayer::currentTime() const
{
    if (m_player) {
        Decoder *d = m_player->decoder();
        if (d)
            return d->position() / 1000;
    }
    return -1;
}

int aKodePlayer::position() const
{
    if (m_player) {
        Decoder *d = m_player->decoder();
        if (d && d->length())
            return (d->position()*1000)/(d->length());
        else
            return -1;
    }
    else
        return -1;
}

/////////////////////////////////////////////////////////////////////////////////
// m_player seek functions
/////////////////////////////////////////////////////////////////////////////////

void aKodePlayer::seek(int seekTime)
{
    // seek time in seconds?
    if (m_player)
        m_player->decoder()->seek(seekTime*1000);
}

void aKodePlayer::seekPosition(int position)
{
    // position unit is 1/1000th
    if (m_player)
        m_player->decoder()->seek((position * m_player->decoder()->length())/1000);
}

#include "akodeplayer.moc"

#endif
