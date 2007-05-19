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

#include <klocale.h>
#include <kglobal.h>
#include <kdebug.h>

////////////////////////////////////////////////////////////////////////////////
// HistoryPlayList public members
////////////////////////////////////////////////////////////////////////////////

HistoryPlaylist::HistoryPlaylist(PlaylistCollection *collection) :
    Playlist(collection, true), m_timer(0)
{
    setAllowDuplicates(true);
    m_timer = new QTimer(this);

    connect(PlayerManager::instance(), SIGNAL(signalPlay()), this, SLOT(slotAddPlaying()));
    connect(m_timer, SIGNAL(timeout()), this, SLOT(slotCreateNewItem()));
}

HistoryPlaylist::~HistoryPlaylist()
{

}

HistoryPlaylistItem *HistoryPlaylist::createItem(const FileHandle &file,
                                                 Q3ListViewItem *after, bool emitChanged)
{
    if(!after)
        after = lastItem();
    return Playlist::createItem<HistoryPlaylistItem, CollectionListItem,
        CollectionList>(file, after, emitChanged);
}

void HistoryPlaylist::createItems(const PlaylistItemList &siblings)
{
    Playlist::createItems<CollectionListItem, HistoryPlaylistItem, PlaylistItem>(siblings);
}

////////////////////////////////////////////////////////////////////////////////
// HistoryPlaylist protected members
////////////////////////////////////////////////////////////////////////////////

void HistoryPlaylist::polish()
{
    addColumn(i18n("Time"));
    Playlist::polish();
    setSorting(-1);
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void HistoryPlaylist::slotAddPlaying()
{
    m_file = PlayerManager::instance()->playingFile();
    m_timer->stop();
    m_timer->setSingleShot(true);
    m_timer->start(delay());
}

void HistoryPlaylist::slotCreateNewItem()
{
    PlayerManager *player = PlayerManager::instance();

    if(player->playing() && m_file == player->playingFile()) {
        createItem(m_file);
        m_file = FileHandle::null();
    }
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

    for(PlaylistItemList::ConstIterator it = l.begin(); it != l.end(); ++it) {
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

        after = p.createItem(FileHandle(fileName), after, false);
        after->setDateTime(dateTime);
    }

    p.dataChanged();

    return s;
}

#include "historyplaylist.moc"

// vim: set et sw=4 tw=0 sta:
