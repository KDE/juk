/***************************************************************************
    begin                : Mon Jun 21 2004
    copyright            : (C) 2004 by Michael Pyne
    email                : pynm0001@comcast.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kapplication.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <klocale.h>

#include <qstringlist.h>
#include <qlistview.h>

#include "collectionlist.h"
#include "treeviewitemplaylist.h"
#include "tag.h"
#include "playlistitem.h"
#include "playlistsearch.h"

TreeViewItemPlaylist::TreeViewItemPlaylist(PlaylistCollection *collection,
                                           const PlaylistSearch &search,
                                           const QString &name,
                                           bool setupPlaylist) :
    SearchPlaylist(collection, search, name, setupPlaylist)
{
    PlaylistSearch::Component component = *(search.components().begin());
    m_columnType = static_cast<PlaylistItem::ColumnType>(*(component.columns().begin()));
}

void TreeViewItemPlaylist::retag(const QStringList &files, Playlist *donorPlaylist)
{
    CollectionList *collection = CollectionList::instance();

    if(files.isEmpty())
        return;

    QString changedTag = i18n("artist");
    if(m_columnType == PlaylistItem::GenreColumn)
        changedTag = i18n("genre");
    else if(m_columnType == PlaylistItem::AlbumColumn)
        changedTag = i18n("album");

    if(KMessageBox::warningContinueCancelList(
           this,
           i18n("You are about to change the %1 on these files.").arg(changedTag),
           files,
           i18n("Changing track tags"),
           KStdGuiItem::cont(),
           "dragDropRetagWarn"
       ) == KMessageBox::Cancel)
    {
        return;
    }

    QStringList::ConstIterator it;
    for(it = files.begin(); it != files.end(); ++it) {
        CollectionListItem *item = collection->lookup(*it);
        if(!item)
            continue;

        Tag *tag = item->file().tag();
        switch(m_columnType) {
        case PlaylistItem::ArtistColumn:
            tag->setArtist(name());
            break;
 
        case PlaylistItem::AlbumColumn:
            tag->setAlbum(name());
            break;

        case PlaylistItem::GenreColumn:
            tag->setGenre(name());
            break;

        default:
            kdDebug() << "Unhandled column type editing " << *it << endl;
        }

        tag->save();
        item->refresh();

        DynamicPlaylist *dynPlaylist = dynamic_cast<DynamicPlaylist *>(donorPlaylist);
        if(dynPlaylist) {
            dynPlaylist->slotSetDirty();
            static_cast<QWidget*>(dynPlaylist)->update();
        }
        else {
            PlaylistItem *donorItem = item->itemForPlaylist(donorPlaylist);
            if(donorItem)
                donorItem->repaint();
        }

        kapp->processEvents();
    }

    dataChanged();
}

#include "treeviewitemplaylist.moc"
