/***************************************************************************
    begin                : Fri Sep 13 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler
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

#include "collectionlist.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kmenu.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kactioncollection.h>
#include <ktoolbarpopupaction.h>
#include <kdirwatch.h>

#include <QList>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QApplication>
#include <QClipboard>
#include <QFileInfo>

#include "playlist/playlistcollection.h"
#include "splashscreen.h"
#include "stringshare.h"
#include "cache.h"
#include "actioncollection.h"
#include "tag.h"
#include "viewmode.h"

using ActionCollection::action;

////////////////////////////////////////////////////////////////////////////////
// static methods
////////////////////////////////////////////////////////////////////////////////

CollectionList *CollectionList::m_list = 0;

CollectionList *CollectionList::instance()
{
    return m_list;
}

void CollectionList::loadCachedItems()
{
    if(!m_list)
        return;

    FileHandleHash::ConstIterator end = Cache::instance()->constEnd();
    for(FileHandleHash::ConstIterator it = Cache::instance()->constBegin(); it != end; ++it) {
        // This may have already been created via a loaded playlist.
        if(!m_itemsDict.contains(it.key())) {
            insertFile(*it);
            m_itemsDict.insert(it.key(), *it);
        }
    }

    SplashScreen::update();

    // The CollectionList is created with sorting disabled for speed.  Re-enable
    // it here, and perform the sort.
//     KConfigGroup config(KGlobal::config(), "Playlists");
// 
//     Qt::SortOrder order = Qt::DescendingOrder;
//     if(config.readEntry("CollectionListSortAscending", true))
//         order = Qt::AscendingOrder;
// ### TODO: View
//     m_list->setSortOrder(order);
//     m_list->setSortColumn(config.readEntry("CollectionListSortColumn", 1));

//     m_list->sort();
    
        
    emit dataChanged(index(0, 0), index(rowCount(), columnCount()));

    SplashScreen::finishedLoading();
}

void CollectionList::initialize(PlaylistCollection *collection)
{
    if(m_list)
        return;

    // We have to delay initialization here because dynamic_cast or comparing to
    // the collection instance won't work in the PlaylistBox::Item initialization
    // won't work until the CollectionList is fully constructed.

    m_list = new CollectionList(collection);
    m_list->setName(i18n("Collection List"));

    collection->setupPlaylist(m_list, "folder-sound");
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

void CollectionList::setupTreeViewEntries(ViewMode *viewMode) const
{
    TreeViewMode *treeViewMode = dynamic_cast<TreeViewMode *>(viewMode);
    if(!treeViewMode) {
        kWarning() << "Can't setup entries on a non-tree-view mode!\n";
        return;
    }

    QList<int> columnList;
    columnList << ArtistColumn;
    columnList << GenreColumn;
    columnList << AlbumColumn;

    foreach(int column, columnList)
        treeViewMode->addItems(m_columnTags[column]->keys(), column);
}

void CollectionList::slotNewItems(const KFileItemList &items)
{
    QStringList files;

    for(KFileItemList::ConstIterator it = items.constBegin(); it != items.constEnd(); ++it)
        files.append((*it).url().path());

    addFiles(files);
//     update();
}

void CollectionList::slotRefreshItems(const QList<QPair<KFileItem, KFileItem> > &items)
{
    for(int i = 0; i < items.count(); ++i) {
        const KFileItem fileItem = items[i].second;
        FileHandle file = lookup(fileItem.url().path());

        if(!file.isNull()) {
            file.refresh();

            // If the item is no longer on disk, remove it from the collection.

            if(!file.fileInfo().exists()) {
                removeFile(file);
            }
        }
    }

//     update();
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void CollectionList::paste()
{
    // ### TODO: View
//     decode(QApplication::clipboard()->mimeData());
}

void CollectionList::clear()
{
    // ### TODO: View
    int result = KMessageBox::warningContinueCancel(/*this*/0,
        i18n("Removing an item from the collection will also remove it from "
             "all of your playlists. Are you sure you want to continue?\n\n"
             "Note, however, that if the directory that these files are in is in "
             "your \"scan on startup\" list, they will be readded on startup."));

    if(result == KMessageBox::Continue) {
        Playlist::clear();
        emit signalCollectionChanged();
    }
}

void CollectionList::slotCheckCache()
{
    loadCachedItems();
    
    foreach(const FileHandle &file, m_itemsDict) {
        if(!file.fileInfo().exists()) {
            removeFile(file);
        }
        processEvents();
    }
}

void CollectionList::slotRemoveItem(const QString &file)
{
    removeFile(m_itemsDict[file]);
}

void CollectionList::slotRefreshItem(const QString &file)
{
    if(!m_itemsDict[file].isNull())
        m_itemsDict[file].refresh();
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

CollectionList::CollectionList(PlaylistCollection *collection) :
    Playlist(collection, true),
    m_columnTags(15, 0)
{
    QAction *spaction = ActionCollection::actions()->addAction("showPlaying");
    spaction->setText(i18n("Show Playing"));
    connect(spaction, SIGNAL(triggered(bool)), SLOT(slotShowPlaying()));

    connect(action<KToolBarPopupAction>("back")->menu(), SIGNAL(aboutToShow()),
            this, SLOT(slotPopulateBackMenu()));
    connect(action<KToolBarPopupAction>("back")->menu(), SIGNAL(triggered(QAction*)),
            this, SLOT(slotPlayFromBackMenu(QAction*)));
//     setSorting(-1); // Temporarily disable sorting to add items faster.

    m_columnTags[ArtistColumn] = new TagCountDict;
    m_columnTags[AlbumColumn] = new TagCountDict;
    m_columnTags[GenreColumn] = new TagCountDict;
}

CollectionList::~CollectionList()
{
    KConfigGroup config(KGlobal::config(), "Playlists");
    // ### TODO: View
//     config.writeEntry("CollectionListSortColumn", sortColumn());
//     config.writeEntry("CollectionListSortAscending", sortOrder() == Qt::AscendingOrder);

    // In some situations the dataChanged signal from clearItems will cause observers to
    // subsequently try to access a deleted item.  Since we're going away just remove all
    // observers.

    clearObservers();

    // The CollectionListItems will try to remove themselves from the
    // m_columnTags member, so we must make sure they're gone before we
    // are.

    clear();

    qDeleteAll(m_columnTags);
    m_columnTags.clear();
}

// ### TODO: View
// void CollectionList::contentsDropEvent(QDropEvent *e)
// {
//     if(e->source() == this)
//         return; // Don't rearrange in the CollectionList.
//     else
//         Playlist::contentsDropEvent(e);
// }
// 
// void CollectionList::contentsDragMoveEvent(QDragMoveEvent *e)
// {
//     if(e->source() != this)
//         Playlist::contentsDragMoveEvent(e);
//     else
//         e->setAccepted(false);
// }

QString CollectionList::addStringToDict(const QString &value, int column)
{
    if(column > m_columnTags.count() || value.trimmed().isEmpty())
        return QString();

    if(m_columnTags[column]->contains(value))
        ++((*m_columnTags[column])[value]);
    else {
        m_columnTags[column]->insert(value, 1);
        emit signalNewTag(value, column);
    }

    return value;
}

QStringList CollectionList::uniqueSet(UniqueSetType t) const
{
    int column;

    switch(t)
    {
    case Artists:
        column = ArtistColumn;
    break;

    case Albums:
        column = AlbumColumn;
    break;

    case Genres:
        column = GenreColumn;
    break;

    default:
        return QStringList();
    }

    return m_columnTags[column]->keys();
}

const FileHandle &CollectionList::lookup(const QString& file) const
{
    return m_itemsDict.value(file);
}

void CollectionList::removeStringFromDict(const QString &value, int column)
{
    if(column > m_columnTags.count() || value.trimmed().isEmpty())
        return;

    if(m_columnTags[column]->contains(value) &&
       --((*m_columnTags[column])[value])) // If the decrement goes to 0...
    {
        emit signalRemovedTag(value, column);
        m_columnTags[column]->remove(value);
    }
}

void CollectionList::addWatched(const QString &file)
{
    m_dirWatch->addFile(file);
}

void CollectionList::removeWatched(const QString &file)
{
    m_dirWatch->removeFile(file);
}

void CollectionList::removeFile(const FileHandle& file)
{
    removeFromDict(file.absFilePath());
    removeStringFromDict(file.tag()->album(), Playlist::AlbumColumn);
    removeStringFromDict(file.tag()->artist(), Playlist::ArtistColumn);
    removeStringFromDict(file.tag()->genre(), Playlist::GenreColumn);
    Playlist::removeFile(file);
}


#include "collectionlist.moc"

// vim: set et sw=4 tw=0 sta:
