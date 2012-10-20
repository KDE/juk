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

#include "historyplaylist.h"
#include "collectionlist.h"
#include "playermanager.h"
#include "juk-exception.h"

#include <QTimer>

#include <klocale.h>
#include <kglobal.h>
#include <kdebug.h>

////////////////////////////////////////////////////////////////////////////////
// HistoryPlayList public members
////////////////////////////////////////////////////////////////////////////////

HistoryPlaylist::HistoryPlaylist(PlaylistCollection *collection) :
    Playlist(collection, true, 1),
    m_timer(new QTimer(this))
{
    setAllowDuplicates(true);

    m_timer->setSingleShot(true);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(slotCreateNewItem()));

    //setSorting(-1);
    setHeaderData(0, Qt::Horizontal, i18n("Time"));
}

HistoryPlaylist::~HistoryPlaylist()
{

}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void HistoryPlaylist::appendProposedItem(const FileHandle &file)
{
    m_file = file;

    if(!m_file.isNull())
        m_timer->start(delay());
    else
        m_timer->stop();
}

void HistoryPlaylist::slotCreateNewItem()
{
    insertFile(m_file);
    m_dateTimes.append(QDateTime::currentDateTime());
    m_file = FileHandle::null();
}

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, const HistoryPlaylist &p)
{
    qint32 count = p.rowCount();
    s << count;

    for (qint32 i=0; i<count; i++) {
        s << p.data(p.index(i, Playlist::FullPathColumn), Qt::DisplayRole).toString();
        s << p.data(p.index(i, Playlist::PlayedColumn), Qt::DisplayRole).toString();
    }

    return s;
}

QDataStream &operator>>(QDataStream &s, HistoryPlaylist &p)
{
    qint32 count;
    s >> count;

    QString fileName;
    QDateTime dateTime;

    for(int i = 0; i < count; i++) {
        s >> fileName;
        s >> dateTime;

        if(fileName.isEmpty() || !dateTime.isValid())
            throw BICStreamException();

        p.insertFile(FileHandle(fileName));
        p.m_dateTimes.append(dateTime);
    }

    p.weChanged();

    return s;
}

QVariant HistoryPlaylist::data(const QModelIndex& index, int role) const
{
    if (index.isValid() && index.column() == Playlist::PlayedColumn && role == Qt::DisplayRole)
        return KGlobal::locale()->formatDateTime(m_dateTimes[index.row()]);
    else
        return Playlist::data(index, role);
}

int HistoryPlaylist::columnCount(const QModelIndex& parent) const
{
    return Playlist::columnCount(parent) + 1;
}


#include "historyplaylist.moc"

// vim: set et sw=4 tw=0 sta:
