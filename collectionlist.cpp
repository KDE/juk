/***************************************************************************
                          collectionlist.cpp  -  description
                             -------------------
    begin                : Fri Sep 13 2002
    copyright            : (C) 2002 by Scott Wheeler
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


#include "collectionlist.h"
#include "splashscreen.h"

////////////////////////////////////////////////////////////////////////////////
// static methods
////////////////////////////////////////////////////////////////////////////////

CollectionList *CollectionList::m_list = 0;

CollectionList *CollectionList::instance()
{
    return m_list;
}

void CollectionList::initialize(QWidget *parent, bool restoreOnLoad)
{
    m_list = new CollectionList(parent);

    if(restoreOnLoad)
	for(QDictIterator<Tag>it(*Cache::instance()); it.current(); ++it)
	    new CollectionListItem(it.current()->fileInfo(), it.current()->absFilePath());
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem *CollectionList::createItem(const QFileInfo &file, const QString &absFilePath, QListViewItem *, bool)
{
    QString filePath;

    if(absFilePath.isNull())
	filePath = resolveSymLinks(file);
    else
	filePath = absFilePath;
    
    if(m_itemsDict.find(filePath))
	return 0;

    PlaylistItem *item = new CollectionListItem(file, filePath);
    
    if(!item->isValid()) {
	kdError() << "CollectinList::createItem() -- A valid tag was not created for \"" << file.filePath() << "\"" << endl;
	delete item;
	return 0;
    }
    
    return item;
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
    if(result == KMessageBox::Yes)			      
	Playlist::clear();

    emit signalCollectionChanged();
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

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

CollectionList::CollectionList(QWidget *parent) :
    Playlist(parent, i18n("Collection List")),
    m_itemsDict(5003),
    m_viewModeItems(m_uniqueSetCount, SortedStringList()),
    m_uniqueSets(m_uniqueSetCount, SortedStringList()),
    m_uniqueSetLast(m_uniqueSetCount, QString::null)
{
    m_dirWatch = new KDirWatch();
    connect(m_dirWatch, SIGNAL(deleted(const QString &)), this, SLOT(slotRemoveItem(const QString &)));
    connect(m_dirWatch, SIGNAL(dirty(const QString &)), this, SLOT(slotRefreshItem(const QString &)));
    m_dirWatch->startScan();

    polish();

    rmbMenu()->insertSeparator();
    rmbMenu()->insertItem(SmallIcon("new"), i18n("Create Group From Selected Items"), this, SLOT(slotCreateGroup()));
}

CollectionList::~CollectionList()
{
    delete m_dirWatch;
}

void CollectionList::decode(QMimeSource *s)
{
    KURL::List urls;
    
    if(!KURLDrag::decode(s, urls) || urls.isEmpty())
	return;
	
    QStringList files;
	
    for(KURL::List::Iterator it = urls.begin(); it != urls.end(); it++)
	files.append((*it).path());
	
    emit signalFilesDropped(files, this);
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

    if(value == m_uniqueSetLast[t] || m_uniqueSets[t].insert(value))
	m_viewModeItems[t].insert(value);
    else
        m_uniqueSetLast[t] = value;
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void CollectionList::slotRemoveItem(const QString &file)
{
    clearItem(m_itemsDict[file]);
}

void CollectionList::slotRefreshItem(const QString &file)
{
    m_itemsDict[file]->slotRefresh();
}

void CollectionList::slotCreateGroup()
{
    emit signalRequestPlaylistCreation(selectedItems());
}

////////////////////////////////////////////////////////////////////////////////
// CollectionListItem public slots
////////////////////////////////////////////////////////////////////////////////

void CollectionListItem::slotRefresh()
{
    slotRefreshImpl();
    
    CollectionList::instance()->addUnique(CollectionList::Artists, text(ArtistColumn));
    CollectionList::instance()->addUnique(CollectionList::Albums, text(AlbumColumn));
    CollectionList::instance()->addUnique(CollectionList::Genres, text(GenreColumn));

    // This is connected to slotRefreshImpl() for all of the items children.
    emit signalRefreshed();
}

////////////////////////////////////////////////////////////////////////////////
// CollectionListItem protected methods
////////////////////////////////////////////////////////////////////////////////

CollectionListItem::CollectionListItem(const QFileInfo &file, const QString &path) :
    PlaylistItem(CollectionList::instance()),
    m_path(path)
{
    CollectionList *l = CollectionList::instance();
    if(l) {
	l->addToDict(m_path, this);
	setData(Data::newUser(file, m_path));
	if(data()->tag()) {
	    slotRefresh();
	    connect(this, SIGNAL(signalRefreshed()), l, SIGNAL(signalDataChanged()));
	    l->emitNumberOfItemsChanged();
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
    CollectionList *l = CollectionList::instance();
    if(l) {
	QString path = Playlist::resolveSymLinks(*data()->fileInfo());
//	l->removeWatched(m_path);
	l->removeFromDict(m_path);
    }
}

void CollectionListItem::addChildItem(PlaylistItem *child)
{
    connect(child, SIGNAL(signalRefreshed()), this, SLOT(slotRefresh()));
    connect(this, SIGNAL(signalRefreshed()), child, SLOT(slotRefreshImpl()));
}

bool CollectionListItem::checkCurrent()
{
    if(!data()->exists() || !data()->isFile())
	return false;

    if(!data()->tag()->current()) {
	data()->refresh();
	slotRefresh();
    }

    return true;
}

#include "collectionlist.moc"
