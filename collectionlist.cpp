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

#include "collectionlist.h"
#include "playlistcollection.h"
#include "splashscreen.h"
#include "stringshare.h"
#include "cache.h"
#include "actioncollection.h"

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// static methods
////////////////////////////////////////////////////////////////////////////////

CollectionList *CollectionList::m_list = 0;

CollectionList *CollectionList::instance()
{
    return m_list;
}

void CollectionList::initialize(PlaylistCollection *collection, bool restoreOnLoad)
{
    if(m_list)
	return;

    // We have to delay initilaization here because dynamic_cast or comparing to
    // the collection instance won't work in the PlaylistBox::Item initialization
    // won't work until the CollectionList is fully constructed.

    m_list = new CollectionList(collection);
    m_list->setName(i18n("Collection List"));

    if(restoreOnLoad) {
	for(FileHandleHash::Iterator it = Cache::instance()->begin();
	    it != Cache::instance()->end();
	    ++it)
	{
	    new CollectionListItem(*it);
	}

	SplashScreen::update();
    }

    collection->setupPlaylist(m_list, "folder_sound");
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem *CollectionList::createItem(const FileHandle &file, QListViewItem *, bool)
{
    if(m_itemsDict.find(file.absFilePath()))
	return 0;

    PlaylistItem *item = new CollectionListItem(file);
    
    if(!item->isValid()) {
	kdError() << "CollectinList::createItem() -- A valid tag was not created for \""
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

    PlaylistInterface::update();
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void CollectionList::clear()
{
    int result = KMessageBox::warningYesNo(this, 
	i18n("Removing an item from the collection will also remove it from "
	     "all of your playlists. Are you sure you want to continue?\n\n"
	     "Note, however, that if the directory that these files are in is in "
	     "your \"scan on startup\" list, they will be readded on startup."));
    if(result == KMessageBox::Yes) {
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
	kapp->processEvents();
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
    m_uniqueSets(m_uniqueSetCount, SortedStringList()),
    m_uniqueSetLast(m_uniqueSetCount, QString::null)
{
    new KAction(i18n("Show Playing"), KShortcut(), actions(), "showPlaying");

    m_dirWatch = new KDirWatch;
    connect(m_dirWatch, SIGNAL(deleted(const QString &)), this, SLOT(slotRemoveItem(const QString &)));
    connect(m_dirWatch, SIGNAL(dirty(const QString &)), this, SLOT(slotRefreshItem(const QString &)));
    connect(action("showPlaying"), SIGNAL(activated()), this, SLOT(slotShowPlaying()));
    m_dirWatch->startScan();

    KConfigGroup config(KGlobal::config(), "Playlists");
    setSortColumn(config.readNumEntry("CollectionListSortColumn", 1));

    polish();
}

CollectionList::~CollectionList()
{
    KConfigGroup config(KGlobal::config(), "Playlists");
    config.writeEntry("CollectionListSortColumn", sortColumn());

    delete m_dirWatch;
}

void CollectionList::contentsDropEvent(QDropEvent *e)
{
    if(e->source() == this)
	return;
    else
	decode(e);
}

void CollectionList::contentsDragMoveEvent(QDragMoveEvent *e)
{
    if(KURLDrag::canDecode(e) && e->source() != this)
	e->accept(true);
    else
	e->accept(false);
}

void CollectionList::addUnique(UniqueSetType t, const QString &value)
{
    if(value.isEmpty())
	return;

    if(value != m_uniqueSetLast[t] && !m_uniqueSets[t].insert(value)) {
	m_uniqueSetLast[t] = value;
	emit signalCollectionChanged();
    }
}

////////////////////////////////////////////////////////////////////////////////
// CollectionListItem public methods
////////////////////////////////////////////////////////////////////////////////

void CollectionListItem::refresh()
{
    CollectionList::instance()->addUnique(CollectionList::Artists, text(ArtistColumn));
    CollectionList::instance()->addUnique(CollectionList::Albums, text(AlbumColumn));
    CollectionList::instance()->addUnique(CollectionList::Genres, text(GenreColumn));

    int offset = static_cast<Playlist *>(listView())->columnOffset();
    int columns = lastColumn() + offset + 1;
    data()->local8Bit.resize(columns);
    data()->cachedWidths.resize(columns);

    for(int i = offset; i < columns; i++) {
	int id = i - offset;
	if(id != TrackNumberColumn && id != LengthColumn)
	{        
	    // All columns other than track num and length need local-encoded data for sorting        

	    QCString lower = id == FileNameColumn
		? file().absFilePath().lower().local8Bit()
		: text(i).lower().local8Bit();

	    // For some columns, we may be able to share some strings

	    if((id == ArtistColumn) || (id == AlbumColumn) ||
	       (id == GenreColumn)  || (id == YearColumn)  ||
	       (id == CommentColumn))
	    {
		lower = StringShare::tryShare(lower);
	    }
	    data()->local8Bit[id] = lower;
	    data()->shortFileName = file().fileInfo().fileName().lower().local8Bit();
	}

	int newWidth = width(listView()->fontMetrics(), listView(), i);
	data()->cachedWidths[i] = newWidth;

	if(newWidth != data()->cachedWidths[i])
	    playlist()->slotWeightDirty(i);
    }

    repaint();
    playlist()->PlaylistInterface::update();
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
	    l->PlaylistInterface::update();
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
	delete *it;
    }

    CollectionList *l = CollectionList::instance();
    if(l)
	l->removeFromDict(file().absFilePath());
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
