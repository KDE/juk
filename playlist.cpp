/***************************************************************************
                          playlist.cpp  -  description
                             -------------------
    begin                : Sat Feb 16 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kmessagebox.h>
#include <kurl.h>
#include <kurldrag.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kdebug.h>

#include <qfileinfo.h>
#include <qdir.h>
#include <qtimer.h>
#include <qapplication.h>
#include <qptrlist.h>

#include <stdlib.h>
#include <time.h>

#include "playlist.h"
#include "collectionlist.h"
#include "playlistsplitter.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Playlist::Playlist(PlaylistSplitter *s, QWidget *parent, const QString &name) : KListView(parent, name.latin1())
{
    playlistName = name;
    internalFile = true;
    playlistFileName = QString::null;
    splitter = s;
    setup();
}

Playlist::Playlist(PlaylistSplitter *s, const QFileInfo &playlistFile, QWidget *parent, const char *name) : KListView(parent, name)
{
    internalFile = false;
    playlistFileName = playlistFile.absFilePath();
    splitter = s;
    setup();

    QFile file(playlistFileName);
    if(!file.open(IO_ReadOnly))
	return;

    QTextStream stream(&file);

    // turn off non-explicit sorting
    setSorting(columns() + 1);

    PlaylistItem *after = 0;

    while(!stream.atEnd()) {
	QString itemName = (stream.readLine()).stripWhiteSpace();

	// Here we're checking to see if JuK has recorded a name for the playlist
	// in the m3u file.  i.e. if one of the lines is 
	// "#NAME=Really Good Music" this will pick that out.

	if(itemName.startsWith("#")) {
	    if(itemName.startsWith("#NAME="))
		setName(itemName.section('=', 1, -1));
	}
	else {
	    QFileInfo item(itemName);

	    if(item.isRelative())
		item.setFile(QDir::cleanDirPath(playlistFile.dirPath(true) + "/" + itemName));
	    
	    if(item.exists() && item.isFile() && item.isReadable()) {
		if(after)
		    after = createItem(item, after);
		else
		    after = createItem(item);
	    }
	}
    }
    
    file.close();
}

Playlist::~Playlist()
{

}

void Playlist::save(bool autoGenerateFileName)
{
    if(autoGenerateFileName && playlistFileName == QString::null) {
	QString dataDir = KGlobal::dirs()->saveLocation("appdata");
	playlistFileName = dataDir + name() + "." + splitter->playlistExtensions().first();
    }
    else if(!autoGenerateFileName && (internalFile || playlistFileName == QString::null))
	return saveAs();
    
    QFile file(playlistFileName);

    if(!file.open(IO_WriteOnly))
	return KMessageBox::error(this, i18n("Could not save to file %1.").arg(playlistFileName));
    
    QTextStream stream(&file);

    if(playlistName != QString::null)
	stream << "#NAME=" << playlistName << endl;

    QStringList fileList = files();

    for(QStringList::Iterator it = fileList.begin(); it != fileList.end(); ++it)
	stream << *it << endl;
    
    file.close();
}

void Playlist::saveAs()
{
    // If this is one of our internal files, remove it when we save it to an
    // "external" file.

    if(internalFile && playlistFileName != QString::null)
	QFile::remove(playlistFileName);

    // This needs to be replace with something that sets the default to the 
    // playlist name.

    QStringList extensions = splitter->playlistExtensions();

    playlistFileName = KFileDialog::getSaveFileName(QString::null, 
						    splitter->extensionsString(splitter->playlistExtensions(),
										       i18n("Playlists")));
    playlistFileName = playlistFileName.stripWhiteSpace();
    internalFile = false;

    if(playlistFileName != QString::null) {
	if(!splitter->playlistExtensions().contains(playlistFileName.section('.', -1)))
	    playlistFileName.append('.' + splitter->playlistExtensions().first());

	
	if(playlistName == QString::null)
	    emit(nameChanged(name()));

	save();
    }
}

void Playlist::refresh()
{
    PlaylistItemList list;
    for(PlaylistItem *i = static_cast<PlaylistItem *>(firstChild()); i; i = static_cast<PlaylistItem *>(i->itemBelow()))
	i->refreshFromDisk();
}

void Playlist::clearItems(const PlaylistItemList &items)
{
    QPtrListIterator<PlaylistItem> it(items);
    while(it.current()) {
        members.remove(it.current()->absFilePath());
        delete(it.current());
        ++it;
    }
}

QStringList Playlist::files() const
{
    QStringList list;
    for(PlaylistItem *i = static_cast<PlaylistItem *>(firstChild()); i; i = static_cast<PlaylistItem *>(i->itemBelow()))
	list.append(i->absFilePath());

    return(list);
}

PlaylistItemList Playlist::items() const
{
    PlaylistItemList list;
    for(PlaylistItem *i = static_cast<PlaylistItem *>(firstChild()); i; i = static_cast<PlaylistItem *>(i->itemBelow()))
	list.append(i);

    return(list);
}

PlaylistItemList Playlist::selectedItems() const
{
    PlaylistItemList list;
    for(PlaylistItem *i = static_cast<PlaylistItem *>(firstChild()); i; i = static_cast<PlaylistItem *>(i->itemBelow()))
        if(i->isSelected())
            list.append(i);
    
    return(list);
}

void Playlist::remove()
{
    remove(selectedItems());
}

void Playlist::remove(const PlaylistItemList &items)
{
    if(isVisible() && !items.isEmpty()) {

        QStringList files;
	for(QPtrListIterator<PlaylistItem> it(items); it.current(); ++it)
            files.append(it.current()->fileName());

	QString message;

	if(files.count() == 1)
	    message = i18n("Do you really want to delete this item?");
	else
	    message = i18n("Do you really want to delete these %1 items?").arg(QString::number(files.count()));
	
	if(KMessageBox::questionYesNoList(this, message, files) == KMessageBox::Yes) {
	    for(QPtrListIterator<PlaylistItem> it(items); it.current(); ++it) {
		if(QFile::remove(it.current()->filePath()))
		    delete(it.current());
		else
		    KMessageBox::sorry(this, i18n("Could not save delete ") + it.current()->fileName() + ".");
	    }

	}
    }
}

PlaylistItem *Playlist::nextItem(PlaylistItem *current, bool random)
{
    if(!current)
	return(0);

    PlaylistItem *i;

    if(random) {
	Playlist *list = static_cast<Playlist *>(current->listView());
	PlaylistItemList items = list->items();
	
	if(items.count() > 1) {
	    srand(time(0));
	    i = current;
	    while(i == current)
		i = items.at(rand() % items.count());
	}
	else
	    i = 0;
    }
    else
	i = static_cast<PlaylistItem *>(current->itemBelow());	

    return(i);
}

bool Playlist::isInternalFile() const
{
    return(internalFile);
}

void Playlist::setInternal(bool internal)
{
    internalFile = internal;
}

QString Playlist::fileName() const
{
    return(playlistFileName);
}

void Playlist::setFileName(const QString &n)
{
    playlistFileName = n;
}

QString Playlist::name() const
{
    if(playlistName == QString::null)
	return(playlistFileName.section(QDir::separator(), -1).section('.', 0, -2));
    else
	return(playlistName);
}

void Playlist::setName(const QString &n)
{
    playlistName = n;
    emit(nameChanged(playlistName));
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

QDragObject *Playlist::dragObject()
{
    PlaylistItemList items = selectedItems();
    KURL::List urls;
    for(PlaylistItem *i = items.first(); i; i = items.next()) {
	KURL url;
	url.setPath(i->absFilePath());
	urls.append(url);
    }
    
    KURLDrag *drag = new KURLDrag(urls, this, "Playlist Items");
    drag->setPixmap(SmallIcon("sound"));

    return(drag);
}

void Playlist::contentsDropEvent(QDropEvent *e)
{
    QListViewItem *moveAfter = itemAt(e->pos());
    if(!moveAfter)
	moveAfter = lastItem();

    // This is slightly more efficient since it doesn't have to cast everything
    // to PlaylistItem.

    if(e->source() == this) {
	QPtrList<QListViewItem> items = KListView::selectedItems();
	
	for(QPtrListIterator<QListViewItem> it(items); it.current(); ++it) {
	    (*it)->moveItem(moveAfter);
	    moveAfter = *it;
	}
    }
    else {
	KURL::List urls;
    
	if(KURLDrag::decode(e, urls) && !urls.isEmpty()) {
	    
	    QStringList fileList;
	    
	    for(KURL::List::Iterator it = urls.begin(); it != urls.end(); it++)
		fileList.append((*it).path());
	    
	    if(splitter)
		splitter->add(fileList, this);
	}
    }
}

void Playlist::contentsDragMoveEvent(QDragMoveEvent *e)
{
    if(KURLDrag::canDecode(e))
	e->accept(true);
    else
	e->accept(false);
}

PlaylistItem *Playlist::createItem(const QFileInfo &file, QListViewItem *after)
{
    CollectionListItem *item = CollectionList::instance()->lookup(file.absFilePath());

    if(!item && CollectionList::instance())
	item = new CollectionListItem(file);
    
    if(item && members.contains(file.absFilePath()) == 0 || allowDuplicates) {
	members.append(file.absFilePath());
	if(after)
	    return(new PlaylistItem(item, this, after));
	else
	    return(new PlaylistItem(item, this));
    }
    else
	return(0);
}

PlaylistSplitter *Playlist::playlistSplitter() const
{
    return(splitter);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void Playlist::setup()
{
    addColumn(i18n("Track Name"));
    addColumn(i18n("Artist"));
    addColumn(i18n("Album"));
    addColumn(i18n("Track"));
    addColumn(i18n("Genre"));
    addColumn(i18n("Year"));
    addColumn(i18n("Length"));
    addColumn(i18n("File Name"));

    setAllColumnsShowFocus(true);
    setSelectionMode(QListView::Extended);
    setShowSortIndicator(true);
    setItemMargin(3);

    setSorting(1);

    connect(this, SIGNAL(selectionChanged()), this, SLOT(emitSelected()));

    addColumn(QString::null);
    setResizeMode(QListView::LastColumn);
    // setFullWidth(true);

    setAcceptDrops(true);
    allowDuplicates = false;

    playlistName = QString::null;
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::emitSelected()
{
    emit(selectionChanged(selectedItems()));
}

#include "playlist.moc"
