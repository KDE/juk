/***************************************************************************
    begin                : Mon Jun 21 2004
    copyright            : (C) 2004 by Michael Pyne
    email                : michael.pyne@kdemail.net
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
#include "tagtransactionmanager.h"

TreeViewItemPlaylist::TreeViewItemPlaylist(PlaylistCollection *collection,
                                           const PlaylistSearch &search,
                                           const QString &name) :
    SearchPlaylist(collection, search, name, false)
{
    PlaylistSearch::Component component = *(search.components().begin());
    m_columnType = static_cast<PlaylistItem::ColumnType>(*(component.columns().begin()));
}

void TreeViewItemPlaylist::retag(const QStringList &files, Playlist *)
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
           i18n("Changing Track Tags"),
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

        Tag *tag = TagTransactionManager::duplicateTag(item->file().tag());
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

	TagTransactionManager::instance()->changeTagOnItem(item, tag);
    }
}

#include "treeviewitemplaylist.moc"
