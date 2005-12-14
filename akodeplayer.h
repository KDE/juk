/***************************************************************************
    copyright            : (C) 2004 by Allan Sandfeld Jensen
    email                : kde@carewolf.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef AKODEPLAYER_H
#define AKODEPLAYER_H

#include <config.h>

#ifdef HAVE_AKODE

#include <qstring.h>

#include "player.h"
#include <kdemacros.h>

namespace aKode {
    class File;
    class Player;
}

class KDE_EXPORT aKodePlayer : public Player
{
    Q_OBJECT

public:
    aKodePlayer();
    virtual ~aKodePlayer();

    virtual void play(const FileHandle &file = FileHandle::null());

    virtual void setVolume(float volume = 1.0);
    virtual float volume() const;

    virtual bool playing() const;
    virtual bool paused() const;

    virtual int totalTime() const;
    virtual int currentTime() const;
    virtual int position() const;

    virtual void seek(int seekTime);
    virtual void seekPosition(int position);

public slots:
    void pause();
    void stop();

private:
    aKode::Player *m_player;
    float m_volume;
};

#endif
#endif
