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
class KSqueezedTextLabel;

class PlaylistItem;

class StatusLabel : public QHBox
{
    Q_OBJECT

public:

    StatusLabel(QWidget *parent = 0, const char *name = 0);
    virtual ~StatusLabel();

public slots:
    /**
     * Set the playlist name.  This text will only be used when there is not an
     * item playing.
     */ 
    void setPlaylistInfo(const QString &name, int count);
    void setPlaylistCount(int c);
    void setPlayingItemInfo(const QString &track, const QString &playlist);

    /**
     * This clears the information about the playing items and frees that status
     * bar up for playlist information.  Ok, ok, so that's not actually clearing,
     * but use your imagination.
     */
    void clear();

    /**
     * This just sets internal variables that are used by setItemCurrentTime().
     * Please call that method to display the time.
     */      
    void setItemTotalTime(int time) { m_itemTotalTime = time; }
    void setItemCurrentTime(int time) { m_itemCurrentTime = time; updateTime(); }

signals:
    void jumpButtonClicked();

private:
    void updateTime();
    virtual bool eventFilter(QObject *o, QEvent *e);

    static QString formatTime(int minutes, int seconds);

    enum Mode { PlayingItemInfo, PlaylistInfo } mode;

    QString m_playlistName;
    int m_playlistCount;
    int m_itemTotalTime;
    int m_itemCurrentTime;
    bool m_showTimeRemaining;

    KSqueezedTextLabel *m_playlistLabel;
    KSqueezedTextLabel *m_trackLabel;
    QLabel *m_itemTimeLabel;
};

#endif
