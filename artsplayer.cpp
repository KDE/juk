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

#include "artsplayer.h"

#if HAVE_ARTS

#include <kdebug.h>
#include <kconfig.h>
#include <kstandarddirs.h>

#include <qdir.h>

#include <kartsserver.h>
#include <kartsdispatcher.h>
#include <kplayobject.h>
#include <kplayobjectfactory.h>

#include <cstdlib>
#include <sys/wait.h>

#include <kmessagebox.h>
#include <kaudiomanagerplay.h>
#include <klocale.h>

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

ArtsPlayer::ArtsPlayer() : Player(),
                           m_dispatcher(0),
                           m_server(0),
                           m_factory(0),
                           m_playobject(0),
                           m_amanPlay(0),
                           m_volumeControl(Arts::StereoVolumeControl::null()),
                           m_currentVolume(1.0)
{
    setupPlayer();
}

ArtsPlayer::~ArtsPlayer()
{
    delete m_playobject;
    delete m_factory;
    delete m_amanPlay;
    delete m_server;
    delete m_dispatcher;
}

void ArtsPlayer::play(const FileHandle &file)
{
    // kdDebug(65432) << k_funcinfo << endl;
    // Make sure that the server still exists, if it doesn't a new one should
    // be started automatically and the factory and amanPlay are created again.

    if(!file.isNull())
        m_currentURL.setPath(file.absFilePath());

    if(m_server->server().isNull()) {
        KMessageBox::error(0, i18n("Cannot find the aRts soundserver."));
        return;
    }

    if(!m_playobject || !file.isNull()) {
        stop();

        delete m_playobject;
        m_playobject = m_factory->createPlayObject(m_currentURL, false);

        if(m_playobject->object().isNull())
            connect(m_playobject, SIGNAL(playObjectCreated()), SLOT(playObjectCreated()));
        else
            playObjectCreated();
    }

    m_playobject->play();
}

void ArtsPlayer::pause()
{
    // kdDebug(65432) << k_funcinfo << endl;
    if(m_playobject)
        m_playobject->pause();
}

void ArtsPlayer::stop()
{
    // kdDebug(65432) << k_funcinfo << endl;
    if(m_playobject) {
        m_playobject->halt();
        delete m_playobject;
        m_playobject = 0;
    }
    if(!m_volumeControl.isNull()) {
        m_volumeControl.stop();
        m_volumeControl = Arts::StereoVolumeControl::null();
    }
}

void ArtsPlayer::setVolume(float volume)
{
    // kdDebug( 65432 ) << k_funcinfo << endl;

    m_currentVolume = volume;

    if(serverRunning() && m_playobject && !m_playobject->isNull()) {
        if(m_volumeControl.isNull())
            setupVolumeControl();
        if(!m_volumeControl.isNull()) {
            m_volumeControl.scaleFactor(volume);
            // kdDebug( 65432 ) << "set volume to " << volume << endl;
        }
    }
}

float ArtsPlayer::volume() const
{
    return m_currentVolume;
}

/////////////////////////////////////////////////////////////////////////////////
// player status functions
/////////////////////////////////////////////////////////////////////////////////

bool ArtsPlayer::playing() const
{
    if(serverRunning() && m_playobject && m_playobject->state() == Arts::posPlaying)
        return true;
    else
        return false;
}

bool ArtsPlayer::paused() const
{
    if(serverRunning() && m_playobject && m_playobject->state() == Arts::posPaused)
        return true;
    else
        return false;
}

int ArtsPlayer::totalTime() const
{
    if(serverRunning() && m_playobject)
        return m_playobject->overallTime().seconds;
    else
        return -1;
}

int ArtsPlayer::currentTime() const
{
    if(serverRunning() && m_playobject &&
       (m_playobject->state() == Arts::posPlaying ||
        m_playobject->state() == Arts::posPaused))
    {
        return m_playobject->currentTime().seconds;
    }
    else
        return -1;
}

int ArtsPlayer::position() const
{
    if(serverRunning() && m_playobject && m_playobject->state() == Arts::posPlaying) {
        long total = m_playobject->overallTime().seconds * 1000 + m_playobject->overallTime().ms;
        long current = m_playobject->currentTime().seconds * 1000 + m_playobject->currentTime().ms;

        // add .5 to make rounding happen properly

        return int(double(current) * 1000 / total + .5);
    }
    else
        return -1;
}

/////////////////////////////////////////////////////////////////////////////////
// player seek functions
/////////////////////////////////////////////////////////////////////////////////

void ArtsPlayer::seek(int seekTime)
{
    if(serverRunning() && m_playobject) {
        Arts::poTime poSeekTime;
        poSeekTime.custom = 0;
        poSeekTime.ms = 0;
        poSeekTime.seconds = seekTime;
        m_playobject->object().seek(poSeekTime);
    }
}

void ArtsPlayer::seekPosition(int position)
{
    if(serverRunning() && m_playobject) {
        Arts::poTime poSeekTime;
        long total = m_playobject->overallTime().seconds;
        poSeekTime.custom = 0;
        poSeekTime.ms = 0;
        poSeekTime.seconds = long(double(total) * position / 1000 + .5);
        m_playobject->object().seek(poSeekTime);
    }
}

/////////////////////////////////////////////////////////////////////////////////
// private
/////////////////////////////////////////////////////////////////////////////////

void ArtsPlayer::setupArtsObjects()
{
    // kdDebug( 65432 ) << k_funcinfo << endl;
    delete m_factory;
    delete m_amanPlay;
    m_volumeControl = Arts::StereoVolumeControl::null();
    m_factory = new KDE::PlayObjectFactory(m_server);
    m_amanPlay = new KAudioManagerPlay(m_server);

    if(m_amanPlay->isNull() || !m_factory) {
        KMessageBox::error(0, i18n("Connecting/starting aRts soundserver failed. "
                                   "Make sure that artsd is configured properly."));
        exit(1);
    }

    m_amanPlay->setTitle(i18n("JuK"));
    m_amanPlay->setAutoRestoreID("JuKAmanPlay");

    m_factory->setAudioManagerPlay(m_amanPlay);
}

void ArtsPlayer::playObjectCreated()
{
    // kdDebug(65432) << k_funcinfo << endl;
    setVolume(m_currentVolume);
}

void ArtsPlayer::setupPlayer()
{
    m_dispatcher = new KArtsDispatcher;
    m_server = new KArtsServer;
    setupArtsObjects();
    connect(m_server, SIGNAL(restartedServer()), SLOT(setupArtsObjects()));
}

void ArtsPlayer::setupVolumeControl()
{
    // kdDebug( 65432 ) << k_funcinfo << endl;
    m_volumeControl = Arts::DynamicCast(m_server->server().createObject("Arts::StereoVolumeControl"));
    if(!m_volumeControl.isNull() && !m_playobject->isNull() && !m_playobject->object().isNull()) {
        Arts::Synth_AMAN_PLAY ap = m_amanPlay->amanPlay();
        Arts::PlayObject po = m_playobject->object();
        ap.stop();
        Arts::disconnect(po, "left" , ap, "left" );
        Arts::disconnect(po, "right", ap, "right");

        m_volumeControl.start();
        ap.start();

        Arts::connect(po, "left" , m_volumeControl, "inleft" );
        Arts::connect(po, "right", m_volumeControl, "inright");
        Arts::connect(m_volumeControl, "outleft" , ap, "left" );
        Arts::connect(m_volumeControl, "outright", ap, "right");
        // kdDebug( 65432 ) << "connected volume control" << endl;
    }
    else {
        m_volumeControl = Arts::StereoVolumeControl::null();
        kdDebug(65432) << "Could not initialize volume control!" << endl;
    }
}

bool ArtsPlayer::serverRunning() const
{
    if(m_server)
        return !(m_server->server().isNull());
    else
        return false;
}

#include "artsplayer.moc"

#endif 

// vim: sw=4 ts=8 et
