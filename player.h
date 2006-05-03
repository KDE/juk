/***************************************************************************
    begin                : Sun Feb 17 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
    email                : wheeler@kde.org
                           (C) 2006 Matthias Kretz <kretz@kde.org>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PLAYER_H
#define PLAYER_H

#include <QObject>

#include "filehandle.h"

namespace Phonon
{
    class MediaObject;
    class AudioPath;
    class AudioOutput;
}

class Player : public QObject
{
    Q_OBJECT
public:
    Player( QObject* parent = 0 );
    ~Player();

    void play(const FileHandle &file = FileHandle::null());
    void pause();
    void stop();

    void setVolume( float volume = 1.0 );
    float volume() const;

    bool playing() const;
    bool paused() const;

    int totalTime() const;
    int currentTime() const;
    int position() const; // in this case not really the percent

    void seek( int seekTime );
    void seekPosition( int position );

private:
    Phonon::MediaObject* m_media;
    Phonon::AudioPath* m_path;
    Phonon::AudioOutput* m_output;
};

#endif

// vim: set et sw=4 tw=0 sta:
