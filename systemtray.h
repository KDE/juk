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
    void slotPlay();
    void slotStop();
    void slotPause();
    
signals:
    void play();
    void stop();
    void pause();
    void forward();
    void back();
    
private slots:
    void slotBlink();
    
private:
    QTimer *blinkTimer;
    bool blinkStatus;
    QPixmap playPix;
    QPixmap pausePix;
    QPixmap currentPix;
    QPixmap appPix;
    
    static const int blinkInterval = 1000;
};

#endif // SYSTEMTRAY_H
