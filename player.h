/***************************************************************************
                          player.h  -  description
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

#ifndef PLAYER_H
#define PLAYER_H

#include <qstring.h>
#include <soundserver.h>
#include <arts/artsflow.h>

using namespace Arts;

class Player
{
public:
    Player();
    Player(const QString &fileName);
    virtual ~Player();

    void play(const QString &fileName, float volume = 1.0);
    void play(float volume = 1.0);
    void pause();
    void stop();

    void setVolume(float volume = 1.0);
    float getVolume() const;

    bool playing() const;
    bool paused() const;

    long totalTime() const;
    long currentTime() const;
    int position() const; // in this case not really the percent

    void seek(long seekTime);
    void seekPosition(int position);

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
