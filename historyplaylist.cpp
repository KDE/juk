/**
 * Copyright (C) 2003-2004 Scott Wheeler <wheeler@kde.org>
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

#include "historyplaylist.h"

#include <QTimer>

#include <KLocalizedString>

#include "collectionlist.h"
#include "playermanager.h"
#include "juk-exception.h"
#include "juk_debug.h"

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

    setSortingEnabled(false);
    headerItem()->setText(0, i18n("Time"));
}

HistoryPlaylist::~HistoryPlaylist()
{

}

HistoryPlaylistItem *HistoryPlaylist::createItem(const FileHandle &file, QTreeWidgetItem *after)
{
    if(!after)
        after = topLevelItem(topLevelItemCount() - 1);
    return Playlist::createItem<HistoryPlaylistItem>(file, after);
}

void HistoryPlaylist::createItems(const PlaylistItemList &siblings)
{
    Playlist::createItems<QVector, HistoryPlaylistItem, PlaylistItem>(siblings);
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
    m_file = FileHandle();
    playlistItemsChanged();
}

////////////////////////////////////////////////////////////////////////////////
// HistoryPlaylistItem public members
////////////////////////////////////////////////////////////////////////////////

HistoryPlaylistItem::HistoryPlaylistItem(CollectionListItem *item, Playlist *parent, QTreeWidgetItem *after) :
    PlaylistItem(item, parent, after),
    m_dateTime(QDateTime::currentDateTime())
{
    setText(0, m_dateTime.toString());
}

void HistoryPlaylistItem::setDateTime(const QDateTime &dt)
{
    m_dateTime = dt;
    setText(0, m_dateTime.toString());
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

        HistoryPlaylistItem *a = p.createItem(FileHandle(fileName), after);
        if(Q_LIKELY(a)) {
            after = a;
            after->setDateTime(dateTime);
        }
    }

    p.playlistItemsChanged();

    return s;
}

// vim: set et sw=4 tw=0 sta:
