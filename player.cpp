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

#include <kaction.h>
#include <klocale.h>

#include "player.h"
#include "artsplayer.h"
#include "gstreamerplayer.h"

#include "../config.h"

/*
 * The goal here is to contain all of the #if'ed ugliness in this file.
 */


Player *Player::createPlayer(int system)
{
    Player *p = 0;
#if HAVE_GSTREAMER
    switch(system) {
    case Arts:
	p = new ArtsPlayer();
	break;
    case GStreamer:
	p = new GStreamerPlayer();
	break;
    }
#else
    Q_UNUSED(system);
    p = new ArtsPlayer();
#endif

    return p;
}

KSelectAction *Player::playerSelectAction(QObject *parent)
{
    KSelectAction *action = 0;
#if HAVE_GSTREAMER
    action = new KSelectAction(i18n("Output To"), 0, parent, "outputSelect");
    QStringList l;
    l << "aRts" << "GStreamer";
    action->setItems(l);
#else
    Q_UNUSED(parent);
#endif
    return action;
}
