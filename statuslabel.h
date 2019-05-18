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

#include <QStatusBar>
#include <QWidget>

class KSqueezedTextLabel;

class QEvent;
class QLabel;

class FileHandle;
class PlaylistInterface;

class StatusLabel : public QWidget
{
    Q_OBJECT

public:
    explicit StatusLabel(const PlaylistInterface &currentPlaylist, QStatusBar *parent = nullptr);

public slots:
    void slotPlayingItemHasChanged(const FileHandle &file);
    void slotCurrentPlaylistHasChanged(const PlaylistInterface &currentPlaylist);

    /**
     * This just sets internal variables that are used by updateTime().
     * Please call that method to display the time.
     */
    void setItemTotalTime(qint64 time_msec) { m_itemTotalTime = time_msec; }
    void setItemCurrentTime(qint64 time_msec) { m_itemCurrentTime = time_msec; updateTime(); }

private:
    void updateTime();
    virtual bool eventFilter(QObject *o, QEvent *e) override;

    KSqueezedTextLabel *m_playlistLabel = nullptr;
    QLabel             *m_trackLabel    = nullptr;
    QLabel             *m_itemTimeLabel = nullptr;

    int  m_itemTotalTime     = 0;
    int  m_itemCurrentTime   = 0;
    bool m_showTimeRemaining = false;
};

#endif

// vim: set et sw=4 tw=0 sta:
