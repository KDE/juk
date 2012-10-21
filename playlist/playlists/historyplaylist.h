/***************************************************************************
    begin                : Fri Aug 8 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler
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

#ifndef HISTORYPLAYLIST_H
#define HISTORYPLAYLIST_H

#include <QDateTime>

#include "playlist.h"

class HistoryPlaylist : public Playlist
{
    Q_OBJECT

public:
    HistoryPlaylist(PlaylistCollection *collection);
    virtual ~HistoryPlaylist();

    virtual int columnOffset() const { return 1; }
    virtual bool readOnly() const { return true; }
    virtual QVariant data(const QModelIndex& index, int role) const;
    virtual int columnCount (const QModelIndex & parent = QModelIndex()) const;

    static int delay() { return 5000; }

public slots:
    void cut() {}
    void clear() {}
    void appendProposedItem(const FileHandle &file);

private slots:
    void slotCreateNewItem();

private:
    friend QDataStream &operator<<(QDataStream &s, const HistoryPlaylist &p);
    friend QDataStream &operator>>(QDataStream &s, HistoryPlaylist &p);

    FileHandle m_file;
    QTimer *m_timer;
    QList<QDateTime> m_dateTimes;
};

QDataStream &operator<<(QDataStream &s, const HistoryPlaylist &p);
QDataStream &operator>>(QDataStream &s, HistoryPlaylist &p);

#endif

// vim: set et sw=4 tw=0 sta:
