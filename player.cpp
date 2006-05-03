/*  This file is part of the KDE project
    Copyright (C) 2006 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.

*/

#include "player.h"
#include <phonon/mediaobject.h>
#include <phonon/audiopath.h>
#include <phonon/audiooutput.h>

Player::Player( QObject* parent )
	: QObject( parent )
	, m_media( new Phonon::MediaObject( this ) )
	, m_path( new Phonon::AudioPath( this ) )
	, m_output( new Phonon::AudioOutput( this ) )
{
	m_output->setCategory( Phonon::MusicCategory );
	m_path->addOutput( m_output );
	m_media->addAudioPath( m_path );
}

Player::~Player()
{
}

void Player::play( const FileHandle &file )
{
	if( file.isNull() )
		return;

	m_media->setUrl( KUrl::fromPath( file.absFilePath() ) );
	m_media->play();
}

void Player::pause()
{
	m_media->pause();
}

void Player::stop()
{
	m_media->stop();
}

void Player::setVolume( float volume )
{
	m_output->setVolume( volume );
}

float Player::volume() const
{
	return m_output->volume();
}

bool Player::playing() const
{
	return m_media->state() == Phonon::PlayingState;
}

bool Player::paused() const
{
	return m_media->state() == Phonon::PausedState;
}

int Player::totalTime() const
{
	return m_media->totalTime() / 1000;
}

int Player::currentTime() const
{
	return m_media->currentTime() / 1000;
}

int Player::position() const
{
	if( m_media->currentTime() > 0 )
		return static_cast<int>( m_media->currentTime() / 1000.0f / m_media->totalTime() + 0.5f );
	return -1;
}

void Player::seek( int seekTime )
{
	m_media->seek( seekTime * 1000 );
}

void Player::seekPosition( int position )
{
	m_media->seek( static_cast<long>( m_media->totalTime() / 1000.0f * position + 0.5f ) );
}

#include "player.moc"

// vim: sw=4 ts=4 noet
