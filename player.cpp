/***************************************************************************
                          player.cpp  -  description
                             -------------------
    begin                : Sun Feb 17 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
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

#include <qfile.h>

#include <connect.h>
#include <flowsystem.h>

#include "player.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

Player::Player()
{
    // set pointers to null
    server = 0;
    volumeControl = 0;
    media = 0;
    dispatcher = 0;

    // startup volume of "full" volume
    currentVolume = 1.0;

    setupPlayer();
}

Player::Player(const QString &fileName)
{
    setupPlayer();
    currentFile = fileName;
}

Player::~Player()
{
    delete(volumeControl);
    delete(media);
    delete(server);
    delete(dispatcher);
}

void Player::play(const QString &fileName, float volume)
{
    currentFile=fileName;
    play(volume);
}

void Player::play(float volume)
{
    if(serverRunning()) {
        if(media && media->state()==posPaused) {
            media->play();
        }
        else {
            if(media)
                stop();

            media = new PlayObject(server->createPlayObject(QFile::encodeName(currentFile).data()));
            //      media = new PlayObject(server->createPlayObject(currentFile.latin1()));
            if(!media->isNull()) {
                setVolume(volume);
                media->play();
            }
            else {
                kdDebug() << "Media did not initialize properly! (" << currentFile << ")" << endl;
                delete(media);
                media = 0;
            }
        }
    }
}

void Player::pause()
{
    if(serverRunning() && media)
	media->pause();
}

void Player::stop()
{
    if(serverRunning()) {
        if(media) {
            media->halt();
            delete(media);
            media = 0;
        }
        if(volumeControl) {
            delete(volumeControl);
            volumeControl = 0;
        }
    }
}

void Player::setVolume(float volume)
{
    if(serverRunning() && media && !media->isNull()) {
        if(!volumeControl)
            setupVolumeControl();
        if(volumeControl) {
            currentVolume = volume;
            volumeControl->scaleFactor(volume);
        }
    }
}

float Player::getVolume() const
{
    return(currentVolume);
}

/////////////////////////////////////////////////////////////////////////////////
// player status functions
/////////////////////////////////////////////////////////////////////////////////

bool Player::playing() const
{
    if(serverRunning() && media && media->state() == posPlaying)
        return(true);
    else
        return(false);
}

bool Player::paused() const
{
    if(serverRunning() && media && media->state() == posPaused)
        return(true);
    else
        return(false);
}

long Player::totalTime() const
{
    if(serverRunning() && media)
        return(media->overallTime().seconds);
    else
        return(-1);
}

long Player::currentTime() const
{
    if(serverRunning() && media && media->state()==posPlaying)
        return(media->currentTime().seconds);
    else
        return(-1);
}

int Player::position() const
{
    if(serverRunning() && media && media->state()==posPlaying) {
        //    long total=media->overallTime().ms;
        //    long current=media->currentTime().ms;
        long total=media->overallTime().seconds * 1000 + media->overallTime().ms;
        long current=media->currentTime().seconds * 1000 + media->currentTime().ms;
        // add .5 to make rounding happen properly
        return(int(double(current)*1000/total+.5));
    }
    else
        return(-1);
}

/////////////////////////////////////////////////////////////////////////////////
// player seek functions
/////////////////////////////////////////////////////////////////////////////////

void Player::seek(long seekTime)
{
    if(serverRunning() && media) {
        poTime poSeekTime;
        poSeekTime.seconds=seekTime;
        media->seek(poSeekTime);
    }
}

void Player::seekPosition(int position)
{
    if(serverRunning() && media) {
        poTime poSeekTime;
        long total=media->overallTime().seconds;
        poSeekTime.seconds=long(double(total)*position/1000+.5);
        media->seek(poSeekTime);
    }
}

/////////////////////////////////////////////////////////////////////////////////
// private
/////////////////////////////////////////////////////////////////////////////////

void Player::setupPlayer()
{
    dispatcher=new Dispatcher;
    server=new SimpleSoundServer(Reference("global:Arts_SimpleSoundServer"));
}

void Player::setupVolumeControl()
{
    volumeControl = new StereoVolumeControl(DynamicCast(server->createObject("Arts::StereoVolumeControl")));
    if(volumeControl && media && !volumeControl->isNull() && !media->isNull()) {

        Synth_BUS_UPLINK uplink = Arts::DynamicCast(media->_getChild( "uplink" ));
        uplink.stop();
        Arts::disconnect(*media, "left", uplink, "left");
        Arts::disconnect(*media, "right", uplink, "right");

        volumeControl->start();
        uplink.start();
        media->_addChild(*volumeControl, "volume" );

        Arts::connect(*media, "left", *volumeControl, "inleft");
        Arts::connect(*media, "right", *volumeControl, "inright");
        Arts::connect(*volumeControl, "outleft", uplink, "left");
        Arts::connect(*volumeControl, "outright", uplink, "right");
    }
    else {
        delete(volumeControl);
        volumeControl = 0;
        kdDebug() << "Could not initialize volume control!" << endl;
    }
}

bool Player::serverRunning() const
{
    if(server)
        return(!(server->isNull()));
    else
        return(0);
}
