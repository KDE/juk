/***************************************************************************
                          artsplayer.h  -  description
                             -------------------
    begin                : Sun Feb 17 2002
    copyright            : (C) 2002 by Scott Wheeler
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

#ifndef ARTSPLAYER_H
#define ARTSPLAYER_H

#include <qstring.h>
#include <soundserver.h>
#include <arts/artsflow.h>

#include "player.h"

using namespace Arts;

class ArtsPlayer : public Player
{
public:
    ArtsPlayer();
    virtual ~ArtsPlayer();

    virtual void play(const QString &fileName, float volume = 1.0);
    virtual void play(float volume = 1.0);
    virtual void pause();
    virtual void stop();

    virtual void setVolume(float volume = 1.0);
    virtual float getVolume() const;

    virtual bool playing() const;
    virtual bool paused() const;

    virtual long totalTime() const;
    virtual long currentTime() const;
    virtual int position() const; // in this case not really the percent

    virtual void seek(long seekTime);
    virtual void seekPosition(int position);

private:
    void setupPlayer();
    void setupVolumeControl();
    bool serverRunning() const;

    Dispatcher *dispatcher;
    SimpleSoundServer *server;
    PlayObject *media;
    StereoVolumeControl *volumeControl;

    QString currentFile;
    float currentVolume;
};

#endif
