/***************************************************************************
                          systray.cpp  -  description
                             -------------------
							 
    copyright            : (C) 2002 by Daniel Molkentin, 
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

#ifndef SYSTEMTRAY_H
#define SYSTEMTRAY_H

#include <ksystemtray.h>

class QTimer;
class QPimap;

class SystemTray : public KSystemTray
{
    Q_OBJECT

public:	
    SystemTray(QWidget *parent = 0, const char* name = 0);
    virtual ~SystemTray();
    
public slots:
    void slotNewSong(const QString& songName);
    void slotPlay() { setPixmap(m_playPix); }
    void slotPause() { setPixmap(m_pausePix); }
    void slotStop();
    
signals:
    void signalPlay();
    void signalStop();
    void signalPause();
    void signalForward();
    void signalBack();

private:
    QTimer *m_blinkTimer;
    bool m_blinkStatus;
    QPixmap m_playPix;
    QPixmap m_pausePix;
    QPixmap m_currentPix;
    QPixmap m_appPix;
};

#endif // SYSTEMTRAY_H
