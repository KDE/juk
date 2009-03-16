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

#include <kurldrag.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kaction.h>
#include <kurl.h>

#include "collectionlist.h"
#include "playlistcollection.h"
#include "splashscreen.h"
#include "stringshare.h"
#include "cache.h"
#include "actioncollection.h"
#include "tag.h"
#include "viewmode.h"
#include "coverinfo.h"
#include "covermanager.h"

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// static methods
////////////////////////////////////////////////////////////////////////////////

CollectionList *CollectionList::m_list = 0;

CollectionList *CollectionList::instance()
{
    return m_list;
}

void CollectionList::initialize(PlaylistCollection *collection)
{
    if(m_list)
	return;

    // We have to delay initilaization here because dynamic_cast or comparing to
    // the collection instance won't work in the PlaylistBox::Item initialization
    // won't work until the CollectionList is fully constructed.

    m_list = new CollectionList(collection);
    m_list->setName(i18n("Collection List"));

    FileHandleHash::Iterator end = Cache::instance()->end();
    for(FileHandleHash::Iterator it = Cache::instance()->begin(); it != end; ++it)
	new CollectionListItem(*it);

    SplashScreen::update();

    // The CollectionList is created with sorting disabled for speed.  Re-enable
    // it here, and perform the sort.
    KConfigGroup config(KGlobal::config(), "Playlists");
    
    SortOrder order = Descending;
    if(config.readBoolEntry("CollectionListSortAscending", true))
	order = Ascending;

    m_list->setSortOrder(order);
    m_list->setSortColumn(config.readNumEntry("CollectionListSortColumn", 1));

    m_list->sort();

    collection->setupPlaylist(m_list, "folder_sound");
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem *CollectionList::createItem(const FileHandle &file, QListViewItem *, bool)
{
    // It's probably possible to optimize the line below away, but, well, right
    // now it's more important to not load duplicate items.

    if(m_itemsDict.find(file.absFilePath()))
	return 0;

    PlaylistItem *item = new CollectionListItem(file);
    
    if(!item->isValid()) {
	kdError() << "CollectionList::createItem() -- A valid tag was not created for \""
		  << file.absFilePath() << "\"" << endl;
	delete item;
	return 0;
    }

    setupItem(item);

    return item;
}

void CollectionList::clearItems(const PlaylistItemList &items)
{
    for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it) {
	Cache::instance()->remove((*it)->file());
	clearItem(*it, false);
    }

    dataChanged();
}

void CollectionList::setupTreeViewEntries(ViewMode *viewMode) const
{
    TreeViewMode *treeViewMode = dynamic_cast<TreeViewMode *>(viewMode);
    if(!treeViewMode) {
	kdWarning(65432) << "Can't setup entries on a non-tree-view mode!\n";
	return;
    }

    QValueList<int> columnList;
    columnList << PlaylistItem::ArtistColumn;
    columnList << PlaylistItem::GenreColumn;
    columnList << PlaylistItem::AlbumColumn;

    QStringList items;
    for(QValueList<int>::Iterator colIt = columnList.begin(); colIt != columnList.end(); ++colIt) {
	items.clear();
	for(TagCountDictIterator it(*m_columnTags[*colIt]); it.current(); ++it)
	    items << it.currentKey();

	treeViewMode->addItems(items, *colIt);
    }
}

void CollectionList::slotNewItems(const KFileItemList &items)
{
    QStringList files;

    for(KFileItemListIterator it(items); it.current(); ++it)
	files.append((*it)->url().path());

    addFiles(files);
    update();
}

void CollectionList::slotRefreshItems(const KFileItemList &items)
{
    for(KFileItemListIterator it(items); it.current(); ++it) {
	CollectionListItem *item = lookup((*it)->url().path());

	if(item) {
	    item->refreshFromDisk();

	    // If the item is no longer on disk, remove it from the collection.

	    if(item->file().fileInfo().exists())
		item->repaint();
	    else
		clearItem(item);
	}
    }

    update();
}

void CollectionList::slotDeleteItem(KFileItem *item)
{
    CollectionListItem *listItem = lookup(item->url().path());
    if(listItem)
	clearItem(listItem);
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void CollectionList::clear()
{
    int result = KMessageBox::warningContinueCancel(this, 
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
    PlaylistItemList invalidItems;

    for(QDictIterator<CollectionListItem>it(m_itemsDict); it.current(); ++it) {
	if(!it.current()->checkCurrent())
	    invalidItems.append(*it);
	processEvents();
    }

    clearItems(invalidItems);
}

void CollectionList::slotRemoveItem(const QString &file)
{
    clearItem(m_itemsDict[file]);
}

void CollectionList::slotRefreshItem(const QString &file)
{
    m_itemsDict[file]->refresh();
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

CollectionList::CollectionList(PlaylistCollection *collection) :
    Playlist(collection, true),
    m_itemsDict(5003),
    m_columnTags(15, 0)
{
    new KAction(i18n("Show Playing"), KShortcut(), actions(), "showPlaying");

    connect(action("showPlaying"), SIGNAL(activated()), this, SLOT(slotShowPlaying()));

    connect(action<KToolBarPopupAction>("back")->popupMenu(), SIGNAL(aboutToShow()),
	    this, SLOT(slotPopulateBackMenu()));
    connect(action<KToolBarPopupAction>("back")->popupMenu(), SIGNAL(activated(int)),
	    this, SLOT(slotPlayFromBackMenu(int)));
    setSorting(-1); // Temporarily disable sorting to add items faster.

    m_columnTags[PlaylistItem::ArtistColumn] = new TagCountDict(5001, false);
    m_columnTags[PlaylistItem::ArtistColumn]->setAutoDelete(true);

    m_columnTags[PlaylistItem::AlbumColumn] = new TagCountDict(5001, false);
    m_columnTags[PlaylistItem::AlbumColumn]->setAutoDelete(true);

    m_columnTags[PlaylistItem::GenreColumn] = new TagCountDict(5001, false);
    m_columnTags[PlaylistItem::GenreColumn]->setAutoDelete(true);

    polish();
}

CollectionList::~CollectionList()
{
    KConfigGroup config(KGlobal::config(), "Playlists");
    config.writeEntry("CollectionListSortColumn", sortColumn());
    config.writeEntry("CollectionListSortAscending", sortOrder() == Ascending);

    // The CollectionListItems will try to remove themselves from the
    // m_columnTags member, so we must make sure they're gone before we
    // are.

    clearItems(items());
    for(TagCountDicts::Iterator it = m_columnTags.begin(); it != m_columnTags.end(); ++it)
	delete *it;
}

void CollectionList::contentsDropEvent(QDropEvent *e)
{
    if(e->source() == this)
	return; // Don't rearrange in the CollectionList.
    else 
	Playlist::contentsDropEvent(e);
}

void CollectionList::contentsDragMoveEvent(QDragMoveEvent *e)
{
    if(canDecode(e) && e->source() != this)
	e->accept(true);
    else
	e->accept(false);
}

QString CollectionList::addStringToDict(const QString &value, unsigned column)
{
    if(column > m_columnTags.count() || value.stripWhiteSpace().isEmpty())
	return QString::null;

    int *refCountPtr = m_columnTags[column]->find(value);
    if(refCountPtr)
	++(*refCountPtr);
    else {
	m_columnTags[column]->insert(value, new int(1));
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
        column = PlaylistItem::ArtistColumn;
    break;
    
    case Albums:
        column = PlaylistItem::AlbumColumn;
    break;
    
    case Genres:
        column = PlaylistItem::GenreColumn;
    break;

    default:
	return QStringList();
    }

    if((unsigned) column >= m_columnTags.count())
	return QStringList();

    TagCountDictIterator it(*m_columnTags[column]);
    QStringList list;

    for(; it.current(); ++it)
	list += it.currentKey();

    return list;
}

void CollectionList::removeStringFromDict(const QString &value, unsigned column)
{
    if(column > m_columnTags.count() || value.isEmpty())
	return;

    int *refCountPtr = m_columnTags[column]->find(value);
    if(refCountPtr) {
	--(*refCountPtr);
	if(*refCountPtr == 0) {
	    emit signalRemovedTag(value, column);
	    m_columnTags[column]->remove(value);
	}
    }
}

////////////////////////////////////////////////////////////////////////////////
// CollectionListItem public methods
////////////////////////////////////////////////////////////////////////////////

void CollectionListItem::refresh()
{
    int offset = static_cast<Playlist *>(listView())->columnOffset();
    int columns = lastColumn() + offset + 1;

    data()->local8Bit.resize(columns);
    data()->cachedWidths.resize(columns);

    for(int i = offset; i < columns; i++) {
	int id = i - offset;
	if(id != TrackNumberColumn && id != LengthColumn) {        
	    // All columns other than track num and length need local-encoded data for sorting        

	    QCString lower = text(i).lower().local8Bit();

	    // For some columns, we may be able to share some strings

	    if((id == ArtistColumn) || (id == AlbumColumn) ||
	       (id == GenreColumn)  || (id == YearColumn)  ||
	       (id == CommentColumn))
	    {
		lower = StringShare::tryShare(lower);

		if(id != YearColumn && id != CommentColumn && data()->local8Bit[id] != lower) {
		    CollectionList::instance()->removeStringFromDict(data()->local8Bit[id], id);
		    CollectionList::instance()->addStringToDict(text(i), id);
		}
	    }

	    data()->local8Bit[id] = lower;
	}

	int newWidth = width(listView()->fontMetrics(), listView(), i);
	data()->cachedWidths[i] = newWidth;

	if(newWidth != data()->cachedWidths[i])
	    playlist()->slotWeightDirty(i);
    }

    file().coverInfo()->setCover();

    if(listView()->isVisible())
	repaint();

    for(PlaylistItemList::Iterator it = m_children.begin(); it != m_children.end(); ++it) {
	(*it)->playlist()->update();
	(*it)->playlist()->dataChanged();
	if((*it)->listView()->isVisible())
	    (*it)->repaint();
    }

    CollectionList::instance()->dataChanged();
    emit CollectionList::instance()->signalCollectionChanged();
}

PlaylistItem *CollectionListItem::itemForPlaylist(const Playlist *playlist)
{
    if(playlist == CollectionList::instance())
	return this;

    PlaylistItemList::ConstIterator it;
    for(it = m_children.begin(); it != m_children.end(); ++it)
	if((*it)->playlist() == playlist)
	    return *it;
    return 0;
}

void CollectionListItem::updateCollectionDict(const QString &oldPath, const QString &newPath)
{
    CollectionList *collection = CollectionList::instance();

    if(!collection)
	return;

    collection->removeFromDict(oldPath);
    collection->addToDict(newPath, this);
}

void CollectionListItem::repaint() const
{
    QListViewItem::repaint();
    for(PlaylistItemList::ConstIterator it = m_children.begin(); it != m_children.end(); ++it)
	(*it)->repaint();
}

////////////////////////////////////////////////////////////////////////////////
// CollectionListItem protected methods
////////////////////////////////////////////////////////////////////////////////

CollectionListItem::CollectionListItem(const FileHandle &file) :
    PlaylistItem(CollectionList::instance()),
    m_shuttingDown(false)
{
    CollectionList *l = CollectionList::instance();
    if(l) {
	l->addToDict(file.absFilePath(), this);

	data()->fileHandle = file;

	if(file.tag()) {
	    refresh();
	    l->dataChanged();
	    // l->addWatched(m_path);
	}
	else
	    kdError() << "CollectionListItem::CollectionListItem() -- Tag() could not be created." << endl;
    }
    else
	kdError(65432) << "CollectionListItems should not be created before "
		       << "CollectionList::initialize() has been called." << endl;

    SplashScreen::increment();
}

CollectionListItem::~CollectionListItem()
{
    m_shuttingDown = true;

    for(PlaylistItemList::ConstIterator it = m_children.begin();
	it != m_children.end();
	++it)
    {
        (*it)->playlist()->clearItem(*it);
    }

    CollectionList *l = CollectionList::instance();
    if(l) {
	l->removeFromDict(file().absFilePath());
	l->removeStringFromDict(file().tag()->album(), AlbumColumn);
	l->removeStringFromDict(file().tag()->artist(), ArtistColumn);
	l->removeStringFromDict(file().tag()->genre(), GenreColumn);
    }
}

void CollectionListItem::addChildItem(PlaylistItem *child)
{
    m_children.append(child);
}

void CollectionListItem::removeChildItem(PlaylistItem *child)
{
    if(!m_shuttingDown)
	m_children.remove(child);
}

bool CollectionListItem::checkCurrent()
{
    if(!file().fileInfo().exists() || !file().fileInfo().isFile())
	return false;

    if(!file().current()) {
	file().refresh();
	refresh();
    }

    return true;
}

#include "collectionlist.moc"
