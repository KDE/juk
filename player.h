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

class QObject;
class QString;
class KSelectAction;

class Player
{
public:
    enum SoundSystem { Arts = 0, GStreamer = 1 };

    virtual ~Player() {}

    virtual void play(const QString &fileName = QString::null) = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;

    virtual void setVolume(float volume = 1.0) = 0;
    virtual float volume() const = 0;

    virtual bool playing() const = 0;
    virtual bool paused() const = 0;

    virtual long totalTime() const = 0;
    virtual long currentTime() const = 0;
    virtual int position() const = 0; // in this case not really the percent

    virtual void seek(long seekTime) = 0;
    virtual void seekPosition(int position) = 0;

    /**
     * Returns a pointer to a Player object.
     * Ownership of the returned player is transferred to the caller.
     */
    static Player *createPlayer(int system = Arts);
    static KSelectAction *playerSelectAction(QObject *parent);

protected:
    Player() {}

};

#endif
