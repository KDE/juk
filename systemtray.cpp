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
#include <qtooltip.h>

#include "systemtray.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

SystemTray::SystemTray(QWidget *parent, const char *name) : KSystemTray(parent, name), m_blinkStatus(false)
{
    m_blinkTimer = new QTimer(this, "blinktimer");
    
    m_appPix   = SmallIcon("juk");
    m_playPix  = SmallIcon("player_play");
    m_pausePix = SmallIcon("player_pause");
    
    setPixmap(m_appPix);
    
    KPopupMenu *cm = contextMenu();
    cm->insertItem(m_playPix, i18n("Play"), this, SIGNAL(signalPlay()));
    cm->insertItem(m_pausePix, i18n("Pause"), this, SIGNAL(signalPause()));
    cm->insertItem(SmallIcon("player_stop"), i18n("Stop"), this, SIGNAL(signalStop()));
    cm->insertItem(SmallIcon("player_start"), i18n("Back"), this, SIGNAL(signalBack()));
    cm->insertItem(SmallIcon("player_end" ), i18n("Forward"), this, SIGNAL(signalForward()));
}

SystemTray::~SystemTray()
{

}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void SystemTray::slotNewSong(const QString& songName)
{
    QToolTip::remove(this);
    QToolTip::add(this, songName);
}

void SystemTray::slotStop()
{
    m_blinkTimer->stop();
    setPixmap(m_appPix);
    QToolTip::remove(this);
}

#include "systemtray.moc"
