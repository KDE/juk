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
