/***************************************************************************
                          systray.cpp  -  description
                             -------------------
    copyright            : (C) 2002 by Daniel Molkentin 
    email                : molkentin@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kapplication.h>
#include <kiconloader.h>
#include <kpopupmenu.h>
#include <klocale.h>

#include <qtimer.h>

#include "systemtray.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

SystemTray::SystemTray(QWidget *parent, const char *name) : KSystemTray(parent, name), blinkStatus(false)
{
    blinkTimer = new QTimer(this, "blinktimer");
    
    appPix   = SmallIcon("juk");
    playPix  = SmallIcon("player_play");
    pausePix = SmallIcon("player_pause");
    
    setPixmap(appPix);
    
    KPopupMenu *cm = contextMenu();
    cm->insertTitle(i18n("Playing"));
    cm->insertItem(playPix, i18n("Play"), this, SIGNAL(play()));
    cm->insertItem(pausePix, i18n("Pause"), this, SIGNAL(pause()));
    cm->insertItem(SmallIcon("player_stop"), i18n("Stop"), this, SIGNAL(stop()));
    cm->insertItem(SmallIcon("player_end" ), i18n("Forward"), this, SIGNAL(forward()));
    cm->insertItem(SmallIcon("player_start"), i18n("Back"), this, SIGNAL(back()));
    
    connect(blinkTimer, SIGNAL(timeout()), this, SLOT(slotBlink()));
}

SystemTray::~SystemTray()
{

}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void SystemTray::slotPlay()
{
    currentPix = playPix;
    blinkTimer->start(blinkInterval);
}

void SystemTray::slotPause()
{
    currentPix = pausePix;
    blinkTimer->start(blinkInterval);
}

void SystemTray::slotStop()
{
    blinkTimer->stop();
    setPixmap(appPix);
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void SystemTray::slotBlink()
{
    if(blinkStatus)
	setPixmap(currentPix);
    else
	setPixmap(appPix);

    blinkStatus = !blinkStatus;
}

#include "systemtray.moc"
