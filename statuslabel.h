/***************************************************************************
    begin                : Fri Oct 18 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
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

#include "playlistinterface.h"

#include <qhbox.h>

class QLabel;
class KSqueezedTextLabel;

class FileHandle;

class StatusLabel : public QHBox, public PlaylistObserver
{
    Q_OBJECT

public:
    StatusLabel(PlaylistInterface *playlist, QWidget *parent = 0, const char *name = 0);
    virtual ~StatusLabel();
    virtual void updateCurrent();

public slots:
    /**
     * This just sets internal variables that are used by setItemCurrentTime().
     * Please call that method to display the time.
     */      
    void setItemTotalTime(int time) { m_itemTotalTime = time; }
    void setItemCurrentTime(int time) { m_itemCurrentTime = time; updateTime(); }
    virtual void updateData();

signals:
    void jumpButtonClicked();

private:
    void updateTime();
    virtual bool eventFilter(QObject *o, QEvent *e);

    static QString formatTime(int minutes, int seconds);

    int m_itemTotalTime;
    int m_itemCurrentTime;
    bool m_showTimeRemaining;

    KSqueezedTextLabel *m_playlistLabel;
    KSqueezedTextLabel *m_trackLabel;
    QLabel *m_itemTimeLabel;
};

#endif
