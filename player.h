/***************************************************************************
                          player.h  -  description
                             -------------------
    begin                : Sun Feb 17 2002
    copyright            : (C) 2002, 2003 by Scott Wheeler
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

#ifndef PLAYER_H
#define PLAYER_H

/**
 * Just an interface for concrete player implementations
 */

class QString;

class Player
{
public:

    enum SoundSystem { Arts, GStreamer };

    virtual ~Player() {}

    virtual void play(const QString &fileName, float volume = 1.0) = 0;
    virtual void play(float volume = 1.0) = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;

    virtual void setVolume(float volume = 1.0) = 0;
    virtual float getVolume() const = 0;

    virtual bool playing() const = 0;
    virtual bool paused() const = 0;

    virtual long totalTime() const = 0;
    virtual long currentTime() const = 0;
    virtual int position() const = 0; // in this case not really the percent

    virtual void seek(long seekTime) = 0;
    virtual void seekPosition(int position) = 0;

    /**
     * Returns a pointer to a Player object.
     */

    static Player *createPlayer(SoundSystem s = Arts);

protected:
    Player() {}

};

#endif
