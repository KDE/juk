/***************************************************************************
                          playlistbox.cpp  -  description
                             -------------------
    begin                : Thu Sep 12 2002
    copyright            : (C) 2002 by Scott Wheeler, 
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

#include <kiconloader.h>
#include <kurldrag.h>
#include <kmessagebox.h>
#include <kinputdialog.h>
#include <kpopupmenu.h>
#include <kaction.h>
#include <kmainwindow.h>
#include <kdebug.h>

#include <qheader.h>
#include <qpainter.h>
#include <qregexp.h>

#include <assert.h>

#include "playlistbox.h"
#include "playlistsplitter.h"
#include "viewmode.h"
#include "searchplaylist.h"

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::PlaylistBox(PlaylistSplitter *parent, const QString &name) :
    KListView(parent, name.latin1()),
    m_splitter(parent),
    m_updatePlaylistStack(true),
    m_viewModeIndex(0),
    m_hasSelection(false)
{
    readConfig();
    addColumn("Playlists", width());
    header()->hide();
    setSorting(0);
    setFullWidth(true);
    setItemMargin(3);
	
    setAcceptDrops(true);
    setSelectionModeExt(Extended);
    
    m_contextMenu = new KPopupMenu(this);

    // Find the main window and then get the associated KActionCollection.

    QObject *w = parent;
    while(w && !dynamic_cast<KMainWindow *>(w))
	w = w->parent();
    
    if(!w)
	return;
    
    KActionCollection *actions = static_cast<KMainWindow *>(w)->actionCollection();

    actions->action("file_new")->plug(m_contextMenu);
    actions->action("renamePlaylist")->plug(m_contextMenu);
    actions->action("duplicatePlaylist")->plug(m_contextMenu);
    actions->action("reloadPlaylist")->plug(m_contextMenu);
    actions->action("deleteItemPlaylist")->plug(m_contextMenu);
    actions->action("file_save")->plug(m_contextMenu);
    actions->action("file_save_as")->plug(m_contextMenu);
    
    // add the view modes stuff
	
    m_viewModeAction = new KSelectAction(actions, "viewModeMenu");
    m_viewModeAction->setText(i18n("View Modes"));
    
    m_viewModes.append(new ViewMode(this));
    m_viewModes.append(new CompactViewMode(this));
    m_viewModes.append(new TreeViewMode(this));

    QStringList modeNames;

    QValueListIterator<ViewMode *> it = m_viewModes.begin();
    for(; it != m_viewModes.end(); ++it)
	modeNames.append((*it)->name());

    m_viewModeAction->setItems(modeNames);
    m_viewModeAction->setCurrentItem(m_viewModeIndex);
    m_viewModes[m_viewModeIndex]->setShown(true);
    
    m_viewModeAction->plug(m_contextMenu);
    connect(m_viewModeAction, SIGNAL(activated(int)), this, SLOT(slotSetViewMode(int)));

    connect(this, SIGNAL(selectionChanged()),
	    this, SLOT(slotPlaylistChanged()));
    
    connect(this, SIGNAL(doubleClicked(QListViewItem *)), 
	    this, SLOT(slotDoubleClicked(QListViewItem *)));
    
    connect(this, SIGNAL(contextMenuRequested(QListViewItem *, const QPoint &, int)),
	    this, SLOT(slotShowContextMenu(QListViewItem *, const QPoint &, int)));
}

PlaylistBox::~PlaylistBox()
{
    saveConfig();
}

void PlaylistBox::createItem(Playlist *playlist, const char *icon, bool raise, bool sortedFirst)
{
    if(!playlist)
	return;

    Item *i = new Item(this, icon, playlist->name(), playlist);
    
    setupItem(i, playlist);

    if(raise) {
	setSingleItem(i);
	ensureCurrentVisible();
    }
    i->setSortedFirst(sortedFirst);

    if(playlist == CollectionList::instance())
	emit signalCollectionInitialized();
}

void PlaylistBox::createSearchItem(SearchPlaylist *playlist, const QString &searchCategory)
{
    Item *i = m_viewModes[m_viewModeIndex]->createSearchItem(this, playlist, searchCategory);
    setupItem(i, playlist);
}

void PlaylistBox::raise(Playlist *playlist)
{
    if(!playlist)
	return;

    Item *i = m_playlistDict.find(playlist);

    clearSelection();
    setSelected(i, true);

    setSingleItem(i);
    ensureCurrentVisible();
}

PlaylistList PlaylistBox::playlists()
{
    PlaylistList l;

    CollectionList *collection = CollectionList::instance();

    Item *i = static_cast<Item *>(firstChild());
    for(; i; i = static_cast<Item *>(i->nextSibling()))
	if(i->playlist() && i->playlist() != collection)
	    l.append(i->playlist());

    return l;
}

void PlaylistBox::save()
{
    save(static_cast<Item *>(currentItem()));
}

void PlaylistBox::saveAs()
{
    saveAs(static_cast<Item *>(currentItem()));
}

void PlaylistBox::rename()
{
    rename(static_cast<Item *>(currentItem()));
}

void PlaylistBox::duplicate()
{
    duplicate(static_cast<Item *>(currentItem()));
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::paste()
{
    Item *i = static_cast<Item *>(currentItem());
    decode(kapp->clipboard()->data(), i);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox private methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::readConfig()
{
    KConfig *config = kapp->config();
    {
	KConfigGroupSaver saver(config, "PlaylistBox");
	m_viewModeIndex = config->readNumEntry("ViewMode", 0);
    }
}

void PlaylistBox::saveConfig()
{
    KConfig *config = kapp->config();
    {
	KConfigGroupSaver saver(config, "PlaylistBox");
	config->writeEntry("ViewMode", m_viewModeAction->currentItem());
	config->sync();
    }
}

void PlaylistBox::save(Item *item)
{
    if(item)
	item->playlist()->save();
}

void PlaylistBox::saveAs(Item *item)
{
    // kdDebug(65432) << "saveAs() - " << bool(item) << endl;
    if(item)
        item->playlist()->saveAs();
}

void PlaylistBox::rename(Item *item)
{
    if(!item)
	return;

    bool ok;

    QString name = KInputDialog::getText(i18n("Rename"),
        i18n("Please enter a name for this playlist:"), item->text(), &ok);

    if(ok) {
	item->setText(0, name);

	// Telling the playlist to change it's name will emit a signal that
	// is connected to Item::slotSetName().

	if(item->playlist())
	    item->playlist()->setName(name);

	sort();
	setSelected(item, true);
	ensureCurrentVisible();
    }
}

void PlaylistBox::duplicate(Item *item)
{
    if(item) {
	bool ok;

	// If this text is changed, please also change it in PlaylistSplitter::createPlaylist().

	QString name = KInputDialog::getText(i18n("New Playlist"), 
					     i18n("Please enter a name for the new playlist:"), 
					     m_splitter->uniquePlaylistName(item->text(0), true), &ok);

	if(ok) {
	    Playlist *p = m_splitter->createPlaylist(name);
	    p->createItems(item->playlist()->items());
	}
    }
}

void PlaylistBox::deleteItem(Playlist *playlist)
{
    Item *i = m_playlistDict.find(playlist);

    if(!i)
	return;

    ItemList l;
    l.append(i);

    deleteItems(l, false);
}

void PlaylistBox::deleteItems(const ItemList &items, bool confirm)
{
    if(items.isEmpty())
	return;

    QStringList files;

    for(ItemList::ConstIterator it = items.begin(); it != items.end(); ++it) {
	if(*it && (*it)->playlist() && !(*it)->playlist()->fileName().isEmpty())
	    files.append((*it)->playlist()->fileName());
    }

    if(confirm) {
	if(!files.isEmpty()) {
	    int remove = KMessageBox::warningYesNoCancelList(
		this, i18n("Do you want to delete these files from the disk as well?"), files);
	
	    if(remove == KMessageBox::Yes) {
		QStringList couldNotDelete;
		for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it) {
		    if(!QFile::remove(*it))
			couldNotDelete.append(*it);
		}

		// Would be nice if there were a KMessageBox::sorryList() to use with
		// couldNotDelete.

		if(!couldNotDelete.isEmpty())
		    KMessageBox::sorry(this, i18n("Could not delete all of the specified files."));
	    }
	    else if(remove == KMessageBox::Cancel)
		return;
	}
	else {
	    if(KMessageBox::warningYesNo(this, i18n("Are you sure you want to remove these items?")) == KMessageBox::No)
		return;
	}
    }

    QValueList< QPair<Item *, Playlist *> > removeQueue;

    for(ItemList::ConstIterator it = items.begin(); it != items.end(); ++it) {
	m_names.remove((*it)->text(0));
	m_playlistDict.remove((*it)->playlist());
	removeQueue.append(qMakePair(*it, (*it)->playlist()));
    }

    if(items.back()->nextSibling() && static_cast<Item *>(items.back()->nextSibling())->playlist())
	setSingleItem(items.back()->nextSibling());
    else {
	Item *i = static_cast<Item *>(items.front()->itemAbove());
	while(i && !i->playlist())
	    i = static_cast<Item *>(i->itemAbove());

	assert(i);

	setSingleItem(i);
    }

    QValueListConstIterator< QPair<Item *, Playlist *> > it = removeQueue.begin();
    for(; it != removeQueue.end(); ++it) {
	delete (*it).first;
	delete (*it).second;
    }
}

void PlaylistBox::decode(QMimeSource *s, Item *item)
{
    if(!s || (item->playlist() && item->playlist()->readOnly()))
	return;

    KURL::List urls;
    
    if(KURLDrag::decode(s, urls) && !urls.isEmpty()) {
	QStringList files;
	
	for(KURL::List::Iterator it = urls.begin(); it != urls.end(); it++)
	    files.append((*it).path());

	if(item && item->playlist())
	    m_splitter->slotAddToPlaylist(files, item->playlist());
	else
	    emit signalCreatePlaylist(files);
    }
}

void PlaylistBox::contentsDropEvent(QDropEvent *e)
{
    Item *i = static_cast<Item *>(itemAt(contentsToViewport(e->pos())));
    decode(e, i);
}

void PlaylistBox::contentsDragMoveEvent(QDragMoveEvent *e)
{
    // If we can decode the input source, there is a non-null item at the "move"
    // position, the playlist for that Item is non-null, is not the 
    // selected playlist and is not the CollectionList, then accept the event.
    //
    // Otherwise, do not accept the event.
    
    if(!KURLDrag::canDecode(e)) {
	e->accept(false);
	return;
    }

    if(itemAt(e->pos())) {
	Item *target = static_cast<Item *>(itemAt(e->pos()));

	if(target->playlist() && target->playlist()->readOnly())
	    return;

	// This is a semi-dirty hack to check if the items are coming from within
	// JuK.  If they are not coming from a Playlist (or subclass) then the
	// dynamic_cast will fail and we can safely assume that the item is 
	// coming from outside of JuK.

	if(dynamic_cast<Playlist *>(e->source())) {
	    if(target->playlist() &&
	       target->playlist() != CollectionList::instance() &&
	       !target->isSelected())
	    {
		e->accept(true);
	    }
	    else
		e->accept(false);
	}
	else // the dropped items are coming from outside of JuK
	    e->accept(true);
    }
    else {

	// We're dragging over the whitespace.  We'll use this case to make it
	// possible to create new lists.

	e->accept(true);
    }
}

PlaylistBox::ItemList PlaylistBox::selectedItems()
{
    ItemList l;

    for(QListViewItemIterator it(this); it.current(); ++it) {
	if(isSelected(*it))
	    l.append(static_cast<Item *>(*it));
    }

    return l;
}

void PlaylistBox::setSingleItem(QListViewItem *item)
{
    setSelectionModeExt(Single);
    KListView::setCurrentItem(item);
    setSelectionModeExt(Extended);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox private slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::slotPlaylistChanged()
{
    ItemList items = selectedItems();
    m_hasSelection = !items.isEmpty();

    if(!m_updatePlaylistStack)
	return;

    PlaylistList playlists;
    for(ItemList::ConstIterator i = items.begin(); i != items.end(); ++i) {
	if((*i)->playlist())
	    playlists.append((*i)->playlist());
    }

    emit signalCurrentChanged(playlists);
}

void PlaylistBox::slotDoubleClicked(QListViewItem *)
{
    emit signalDoubleClicked();
}

void PlaylistBox::slotShowContextMenu(QListViewItem *, const QPoint &point, int)
{
    m_contextMenu->popup(point);
}

void PlaylistBox::slotSetViewMode(int index)
{
    if(index == m_viewModeIndex)
	return;

    viewMode()->setShown(false);
    m_viewModeIndex = index;
    viewMode()->setShown(true);    
}

void PlaylistBox::setupItem(Item *item, Playlist *playlist)
{
    m_playlistDict.insert(playlist, item);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox::Item protected methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::Item *PlaylistBox::Item::m_collectionItem = 0;

PlaylistBox::Item::Item(PlaylistBox *listBox, const char *icon, const QString &text, Playlist *l) 
    : QObject(listBox), KListViewItem(listBox, text),
      m_list(l), m_text(text), m_iconName(icon), m_sortedFirst(false)
{
    init();
}

PlaylistBox::Item::Item(Item *parent, const char *icon, const QString &text, Playlist *l)
    : QObject(parent->listView()), KListViewItem(parent, text),
    m_list(l), m_text(text), m_iconName(icon), m_sortedFirst(false)
{
    init();
}

PlaylistBox::Item::~Item()
{

}

int PlaylistBox::Item::compare(QListViewItem *i, int col, bool) const
{
    Item *otherItem = static_cast<Item *>(i);

    if(m_sortedFirst && !otherItem->m_sortedFirst)
	return -1;
    else if(otherItem->m_sortedFirst && !m_sortedFirst)
	return 1;

    return text(col).lower().localeAwareCompare(i->text(col).lower());
}

void PlaylistBox::Item::paintCell(QPainter *painter, const QColorGroup &colorGroup, int column, int width, int align)
{
    PlaylistBox *playlistBox = static_cast<PlaylistBox *>(listView());
    playlistBox->viewMode()->paintCell(this, painter, colorGroup, column, width, align);
}

void PlaylistBox::Item::setText(int column, const QString &text)
{
    m_text = text;
    KListViewItem::setText(column, text);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox::Item protected slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::Item::slotSetName(const QString &name)
{
    if(listView()) {
	listView()->m_names.remove(text(0));
	listView()->m_names.append(name);

	setText(0, name);
    }
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox::Item private methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::Item::init()
{
    int iconSize = static_cast<PlaylistBox *>(listView())->viewModeIndex() == 0 ? 32 : 16;
    setPixmap(0, SmallIcon(m_iconName, iconSize));
    static_cast<PlaylistBox *>(listView())->addName(m_text);

    if(m_list)
	connect(m_list, SIGNAL(signalNameChanged(const QString &)), this, SLOT(slotSetName(const QString &)));

    if(m_list == CollectionList::instance())
	m_collectionItem = this;
}

#include "playlistbox.moc"
