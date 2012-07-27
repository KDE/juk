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

HistoryPlaylistItem *HistoryPlaylist::createItem(const FileHandle &file,
                                                 HistoryPlaylistItem *after, bool emitChanged)
{
    if(!after)
        after = static_cast<HistoryPlaylistItem*>(lastItem());
    return Playlist::createItem<HistoryPlaylistItem>(file, after, emitChanged);
}

void HistoryPlaylist::createItems(const PlaylistItemList &siblings)
{
    Playlist::createItems<HistoryPlaylistItem, PlaylistItem>(siblings);
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
    createItem(m_file);
    m_file = FileHandle::null();
}

////////////////////////////////////////////////////////////////////////////////
// HistoryPlaylistItem public members
////////////////////////////////////////////////////////////////////////////////

HistoryPlaylistItem::HistoryPlaylistItem(CollectionListItem *item, Playlist *parent, Q3ListViewItem *after) :
    PlaylistItem(item, parent, after),
    m_dateTime(QDateTime::currentDateTime())
{
    setText(0, KGlobal::locale()->formatDateTime(m_dateTime));
}

HistoryPlaylistItem::HistoryPlaylistItem(CollectionListItem *item, Playlist *parent) :
    PlaylistItem(item, parent),
    m_dateTime(QDateTime::currentDateTime())
{
    setText(0, KGlobal::locale()->formatDateTime(m_dateTime));
}

HistoryPlaylistItem::~HistoryPlaylistItem()
{

}

void HistoryPlaylistItem::setDateTime(const QDateTime &dt)
{
    m_dateTime = dt;
    setText(0, KGlobal::locale()->formatDateTime(m_dateTime));
}

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, const HistoryPlaylist &p)
{
    PlaylistItemList l = const_cast<HistoryPlaylist *>(&p)->items();

    s << qint32(l.count());

    for(PlaylistItemList::ConstIterator it = l.constBegin(); it != l.constEnd(); ++it) {
        const HistoryPlaylistItem *i = static_cast<HistoryPlaylistItem *>(*it);
        s << i->file().absFilePath();
        s << i->dateTime();
    }

    return s;
}

QDataStream &operator>>(QDataStream &s, HistoryPlaylist &p)
{
    qint32 count;
    s >> count;

    HistoryPlaylistItem *after = 0;

    QString fileName;
    QDateTime dateTime;

    for(int i = 0; i < count; i++) {
        s >> fileName;
        s >> dateTime;

        if(fileName.isEmpty() || !dateTime.isValid())
            throw BICStreamException();

        HistoryPlaylistItem *a = p.createItem(FileHandle(fileName), after, false);
        if(a) {
            after = a;
            after->setDateTime(dateTime);
        }
    }

    p.dataChanged();

    return s;
}

#include "historyplaylist.moc"

// vim: set et sw=4 tw=0 sta:
