/***************************************************************************
    begin                : Sun Feb 17 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
    email                : wheeler@kde.org

    copyright            : (C) 2003 by Matthias Kretz
    email                : kretz@kde.org
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

#include <config.h>

#if HAVE_ARTS

#include "player.h"

#include <kurl.h>

#include <qstring.h>
#include <qobject.h>
#include <artsflow.h>

class KArtsDispatcher;
class KArtsServer;
class KAudioManagerPlay;

namespace KDE {
    class PlayObjectFactory;
    class PlayObject;
}

class ArtsPlayer : public Player
{
    Q_OBJECT

public:
    ArtsPlayer();
    virtual ~ArtsPlayer();

    virtual void play(const FileHandle &file = FileHandle::null());
    virtual void pause();
    virtual void stop();

    virtual void setVolume(float volume = 1.0);
    virtual float volume() const;

    virtual bool playing() const;
    virtual bool paused() const;

    virtual int totalTime() const;
    virtual int currentTime() const;
    virtual int position() const; // in this case not really the percent

    virtual void seek(int seekTime);
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
#endif

// vim: sw=4 ts=8 et
