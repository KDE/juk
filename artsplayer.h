/***************************************************************************
                          artsplayer.h  -  description
                             -------------------
    begin                : Sun Feb 17 2002
    copyright            : (C) 2002 by Scott Wheeler <wheeler@kde.org>
                           (C) 2003 by Matthias Kretz <kretz@kde.org>
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

#include "player.h"

#include <kurl.h>
#include <artsflow.h>

class QString;

class KArtsDispatcher;
class KArtsServer;
class KAudioManagerPlay;
namespace KDE {
    class PlayObjectFactory;
    class PlayObject;
}

class ArtsPlayer : public QObject, public Player
{
    Q_OBJECT

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

private slots:
    void setupArtsObjects();
    void playObjectCreated();

private:
    void setupPlayer();
    void setupVolumeControl();
    bool serverRunning() const;

    KArtsDispatcher *m_dispatcher;
    KArtsServer *m_server;
    KDE::PlayObjectFactory *m_factory;
    KDE::PlayObject *m_playobject;
    KAudioManagerPlay *m_amanPlay;

    // This is a pretty heavy module for the needs that JuK has, it would probably
    // be good to use two Synth_MUL instead or the one from Noatun.
    Arts::StereoVolumeControl m_volumeControl;

    KURL m_currentURL;
    float m_currentVolume;
};

#endif

// vim: sw=4 ts=8 et
