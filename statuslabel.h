/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef JUK_STATUSLABEL_H
#define JUK_STATUSLABEL_H

#include "playlistinterface.h"

#include <QWidget>

class QEvent;
class QLabel;
class KSqueezedTextLabel;

class StatusLabel : public QWidget, public PlaylistObserver
{
    Q_OBJECT

public:
    explicit StatusLabel(PlaylistInterface *playlist, QWidget *parent = nullptr);
    virtual void playingItemHasChanged() Q_DECL_FINAL;

public slots:
    /**
     * This just sets internal variables that are used by setItemCurrentTime().
     * Please call that method to display the time.
     */
    void setItemTotalTime(int time) { m_itemTotalTime = time; }
    void setItemCurrentTime(int time) { m_itemCurrentTime = time; updateTime(); }
    virtual void playlistItemDataHasChanged() Q_DECL_FINAL;

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

// vim: set et sw=4 tw=0 sta:
