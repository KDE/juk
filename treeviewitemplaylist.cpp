/**
 * Copyright (C) 2004 Michael Pyne <mpyne@kde.org>
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

#include "treeviewitemplaylist.h"

#include <kmessagebox.h>
#include <KLocalizedString>

#include <QStringList>

#include "collectionlist.h"
#include "tag.h"
#include "playlistitem.h"
#include "playlistsearch.h"
#include "tagtransactionmanager.h"
#include "juk_debug.h"

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
           i18n("You are about to change the %1 on these files.", changedTag),
           files,
           i18n("Changing Track Tags"),
           KStandardGuiItem::cont(),
           KStandardGuiItem::cancel(),
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
            qCDebug(JUK_LOG) << "Unhandled column type editing " << *it;
        }

        TagTransactionManager::instance()->changeTagOnItem(item, tag);
    }
}

// vim: set et sw=4 tw=0 sta:
