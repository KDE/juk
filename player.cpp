/***************************************************************************
                          player.cpp  -  description
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

#include "player.h"
#include "artsplayer.h"
#include "gstreamerplayer.h"

#include "../config.h"

Player *Player::createPlayer(SoundSystem s)
{
    Player *p = 0;
    
#if HAVE_GSTREAMER

    if(s == Arts)
	p = new ArtsPlayer();
    else if(s == GStreamer)
	p = new GStreamerPlayer();

#else

    p = new ArtsPlayer();

#endif

    return p;
}
