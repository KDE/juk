/***************************************************************************
                          statuslabel.h  -  description
                             -------------------
    begin                : Fri Oct 18 2002
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

#ifndef STATUSLABEL_H
#define STATUSLABEL_H

#include <qhbox.h>

class QLabel;

class PlaylistItem;

class StatusLabel : public QHBox
{
    Q_OBJECT

public: 
    StatusLabel(QWidget *parent = 0, const char *name = 0);
    virtual ~StatusLabel();

    void setPlayingItem(PlaylistItem *item);
    void clear();

    /**
     * This just sets internal variables that are used by setItemCurrentTime().
     * Please call that method to display the time.
     */      
    void setItemTotalTime(int time);
    void setItemCurrentTime(int time);

private:
    static QString formatTime(int minutes, int seconds);
    void updateTime();
    virtual bool eventFilter(QObject *o, QEvent *e);

    QLabel *playlistLabel;
    QLabel *trackLabel;
    QLabel *itemTimeLabel;
    PlaylistItem *playingItem;
    int itemTotalTime;
    int itemCurrentTime;
    bool showTimeRemaining;

private slots:
    void jumpToPlayingItem() const;

};

#endif
