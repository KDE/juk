/***************************************************************************
                          player.cpp  -  description
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

#include <kdebug.h>
#include <kconfig.h>
#include <kstandarddirs.h>

#include <qdir.h>

#include <connect.h>
#include <flowsystem.h>

#include <sys/wait.h>

#include "artsplayer.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

ArtsPlayer::ArtsPlayer() : Player()
{
    // set pointers to null
    m_server = 0;
    m_volumeControl = 0;
    m_media = 0;
    m_dispatcher = 0;

    // startup volume of "full" volume
    m_currentVolume = 1.0;

    setupPlayer();
}

ArtsPlayer::~ArtsPlayer()
{
    delete m_volumeControl;
    delete m_media;
    delete m_server;
    delete m_dispatcher;
}

void ArtsPlayer::play(const QString &fileName, float volume)
{
    m_currentFile = fileName;
    play(volume);
}

void ArtsPlayer::play(float volume)
{
    if (!serverRunning() || (m_server && m_server->error()) ) restart();
      
    if(serverRunning()) {
        if(m_media && m_media->state() == posPaused) {
            m_media->play();
        }
        else {
            if(m_media)
                stop();

            m_media = new PlayObject(m_server->createPlayObject(QFile::encodeName(m_currentFile).data()));
            //      m_media = new PlayObject(m_server->createPlayObject(m_currentFile.latin1()));
            if(!m_media->isNull()) {
                setVolume(volume);
                m_media->play();
            }
            else {
                kdDebug() << "Media did not initialize properly! (" << m_currentFile << ")" << endl;
                delete m_media;
                m_media = 0;
            }
        }
    }
}

void ArtsPlayer::pause()
{
    if(serverRunning() && m_media)
	m_media->pause();
}

void ArtsPlayer::stop()
{
    if(serverRunning()) {
        if(m_media) {
            m_media->halt();
            delete m_media;
            m_media = 0;
        }
        if(m_volumeControl) {
            delete m_volumeControl;
            m_volumeControl = 0;
        }
    }
}

void ArtsPlayer::setVolume(float volume)
{
    if(serverRunning() && m_media && !m_media->isNull()) {
        if(!m_volumeControl)
            setupVolumeControl();
        if(m_volumeControl) {
            m_currentVolume = volume;
            m_volumeControl->scaleFactor(volume);
        }
    }
}

float ArtsPlayer::getVolume() const
{
    return m_currentVolume;
}

/////////////////////////////////////////////////////////////////////////////////
// player status functions
/////////////////////////////////////////////////////////////////////////////////

bool ArtsPlayer::playing() const
{
    if(serverRunning() && m_media && m_media->state() == posPlaying)
        return true;
    else
        return false;
}

bool ArtsPlayer::paused() const
{
    if(serverRunning() && m_media && m_media->state() == posPaused)
        return true;
    else
        return false;
}

long ArtsPlayer::totalTime() const
{
    if(serverRunning() && m_media)
        return m_media->overallTime().seconds;
    else
        return -1;
}

long ArtsPlayer::currentTime() const
{
    if(serverRunning() && m_media && m_media->state() == posPlaying)
        return m_media->currentTime().seconds;
    else
        return -1;
}

int ArtsPlayer::position() const
{
    if(serverRunning() && m_media && m_media->state() == posPlaying) {
        //    long total=m_media->overallTime().ms;
        //    long current=m_media->currentTime().ms;
        long total = m_media->overallTime().seconds * 1000 + m_media->overallTime().ms;
        long current = m_media->currentTime().seconds * 1000 + m_media->currentTime().ms;
        // add .5 to make rounding happen properly
        return int(double(current) * 1000 / total + .5);
    }
    else
        return -1;
}

/////////////////////////////////////////////////////////////////////////////////
// player seek functions
/////////////////////////////////////////////////////////////////////////////////

void ArtsPlayer::seek(long seekTime)
{
    if(serverRunning() && m_media) {
        poTime poSeekTime;
        poSeekTime.seconds = seekTime;
        m_media->seek(poSeekTime);
    }
}

void ArtsPlayer::seekPosition(int position)
{
    if(serverRunning() && m_media) {
        poTime poSeekTime;
        long total = m_media->overallTime().seconds;
        poSeekTime.seconds = long(double(total) * position / 1000 + .5);
        m_media->seek(poSeekTime);
    }
}

/////////////////////////////////////////////////////////////////////////////////
// private
/////////////////////////////////////////////////////////////////////////////////

void ArtsPlayer::setupPlayer()
{
    m_dispatcher = new Dispatcher;
    m_server = new SimpleSoundServer(Reference("global:Arts_SimpleSoundServer"));
}

void ArtsPlayer::setupVolumeControl()
{
    m_volumeControl = new StereoVolumeControl(DynamicCast(m_server->createObject("Arts::StereoVolumeControl")));
    if(m_volumeControl && m_media && !m_volumeControl->isNull() && !m_media->isNull()) {

        Synth_BUS_UPLINK uplink = Arts::DynamicCast(m_media->_getChild( "uplink" ));
        uplink.stop();
        Arts::disconnect(*m_media, "left", uplink, "left");
        Arts::disconnect(*m_media, "right", uplink, "right");

        m_volumeControl->start();
        uplink.start();
        m_media->_addChild(*m_volumeControl, "volume" );

        Arts::connect(*m_media, "left", *m_volumeControl, "inleft");
        Arts::connect(*m_media, "right", *m_volumeControl, "inright");
        Arts::connect(*m_volumeControl, "outleft", uplink, "left");
        Arts::connect(*m_volumeControl, "outright", uplink, "right");
    }
    else {
        delete m_volumeControl;
        m_volumeControl = 0;
        kdDebug() << "Could not initialize volume control!" << endl;
    }
}

void ArtsPlayer::restart()
{
    delete m_volumeControl;
    m_volumeControl=0;
    delete m_server;
    m_server=0;

    // *** This piece of text was copied from Noatun's engine
    // and slightly modified
    KConfig config("kcmartsrc");
    QCString cmdline;

    config.setGroup("Arts");

    bool rt = config.readBoolEntry("StartRealtime",false);
    bool x11Comm = config.readBoolEntry("X11GlobalComm",false);

    // put the value of x11Comm into .mcoprc
    {
      KConfig X11CommConfig(QDir::homeDirPath()+"/.mcoprc");

      if(x11Comm)
	X11CommConfig.writeEntry("GlobalComm", "Arts::X11GlobalComm");
      else
	X11CommConfig.writeEntry("GlobalComm", "Arts::TmpGlobalComm");

      X11CommConfig.sync();
    }

    cmdline = QFile::encodeName(KStandardDirs::findExe(
	  QString::fromLatin1("kdeinit_wrapper") ));
    cmdline += " ";

    if (rt)
      cmdline += QFile::encodeName(KStandardDirs::findExe(
	    QString::fromLatin1("artswrapper")));
    else
      cmdline += QFile::encodeName(KStandardDirs::findExe(
	    QString::fromLatin1("artsd")));

    cmdline += " ";
    cmdline += config.readEntry("Arguments","-F 10 -S 4096 -s 60 -m artsmessage -l 3 -f").utf8();

    int status=::system(cmdline);

    if ( status!=-1 && WIFEXITED(status) )
    {
      // We could have a race-condition here.
      // The correct way to do it is to make artsd fork-and-exit
      // after starting to listen to connections (and running artsd
      // directly instead of using kdeinit), but this is better
      // than nothing.
      int time = 0;
      do
      {
	// every time it fails, we should wait a little longer
	// between tries
	::sleep(1+time/2);
	delete m_server;
	m_server = new SimpleSoundServer(Reference("global:Arts_SimpleSoundServer"));
      } while(++time < 5 && (m_server->isNull()));
    }
    // *** Until here

}

bool ArtsPlayer::serverRunning() const
{
    if(m_server)
        return !(m_server->isNull());
    else
        return false;
}
