/***************************************************************************
    begin                : Thu Sep 12 2002
    copyright            : (C) 2002 - 2004 by Scott Wheeler,
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
#include <kpopupmenu.h>
#include <kaction.h>
#include <kdebug.h>

#include <qheader.h>
#include <qpainter.h>
#include <qwidgetstack.h>

#include "playlistbox.h"
#include "playlist.h"
#include "collectionlist.h"
#include "dynamicplaylist.h"
#include "viewmode.h"
#include "searchplaylist.h"
#include "actioncollection.h"
#include "cache.h"
#include "k3bexporter.h"

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::PlaylistBox(QWidget *parent, QWidgetStack *playlistStack,
			 const QString &name) :
    KListView(parent, name.latin1()),
    PlaylistCollection(playlistStack),
    m_updatePlaylistStack(true),
    m_viewModeIndex(0),
    m_hasSelection(false),
    m_doingMultiSelect(false),
    m_dropItem(0),
    m_dynamicPlaylist(0)
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

    K3bPlaylistExporter *exporter = new K3bPlaylistExporter(this);
    m_k3bAction = exporter->action();
    
    action("file_new")->plug(m_contextMenu);
    action("renamePlaylist")->plug(m_contextMenu);
    action("editSearch")->plug(m_contextMenu);
    action("duplicatePlaylist")->plug(m_contextMenu);
    action("reloadPlaylist")->plug(m_contextMenu);
    action("deleteItemPlaylist")->plug(m_contextMenu);
    action("file_save")->plug(m_contextMenu);
    action("file_save_as")->plug(m_contextMenu);
    if(m_k3bAction)
	m_k3bAction->plug(m_contextMenu);

    m_contextMenu->insertSeparator();

    // add the view modes stuff

    m_viewModeAction = new KSelectAction(i18n("View Modes"), "view_choose",
					 KShortcut(), actions(), "viewModeMenu");

    m_viewModes.append(new ViewMode(this));
    m_viewModes.append(new CompactViewMode(this));
    m_viewModes.append(new TreeViewMode(this));

    QStringList modeNames;

    for(QValueListIterator<ViewMode *> it = m_viewModes.begin(); it != m_viewModes.end(); ++it)
	modeNames.append((*it)->name());

    m_viewModeAction->setItems(modeNames);

    QPopupMenu *p = m_viewModeAction->popupMenu();
    p->changeItem(0, SmallIconSet("view_detailed"), modeNames[0]);
    p->changeItem(1, SmallIconSet("view_text"), modeNames[1]);
    p->changeItem(2, SmallIconSet("view_tree"), modeNames[2]);

    m_viewModeAction->setCurrentItem(m_viewModeIndex);
    m_viewModes[m_viewModeIndex]->setShown(true);

    m_viewModeAction->plug(m_contextMenu);
    connect(m_viewModeAction, SIGNAL(activated(int)), this, SLOT(slotSetViewMode(int)));

    connect(this, SIGNAL(selectionChanged()),
	    this, SLOT(slotPlaylistChanged()));

    connect(this, SIGNAL(doubleClicked(QListViewItem *)),
	    this, SLOT(slotDoubleClicked()));

    connect(this, SIGNAL(contextMenuRequested(QListViewItem *, const QPoint &, int)),
	    this, SLOT(slotShowContextMenu(QListViewItem *, const QPoint &, int)));

    CollectionList::initialize(this);
    Cache::loadPlaylists(this);
    raise(CollectionList::instance());
}

PlaylistBox::~PlaylistBox()
{
    PlaylistList l;
    CollectionList *collection = CollectionList::instance();
    for(QListViewItem *i = firstChild(); i; i = i->itemBelow()) {
	Item *item = static_cast<Item *>(i);
	if(item->playlist() && item->playlist() != collection)
	    l.append(item->playlist());
    }

    Cache::savePlaylists(l);
    saveConfig();
}

void PlaylistBox::raise(Playlist *playlist)
{
    if(!playlist)
	return;

    Item *i = m_playlistDict.find(playlist);

    clearSelection();
    setSelected(i, true);

    setSingleItem(i);
    ensureItemVisible(currentItem());
}

void PlaylistBox::duplicate()
{
    Item *item = static_cast<Item *>(currentItem());
    if(!item || !item->playlist())
	return;

    QString name = playlistNameDialog(i18n("Duplicate"), item->text(0));

    if(name.isNull())
	return;

    Playlist *p = new Playlist(this, name);
    p->createItems(item->playlist()->items());
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
// PlaylistBox protected methods
////////////////////////////////////////////////////////////////////////////////

Playlist *PlaylistBox::currentPlaylist() const
{
    if(m_dynamicPlaylist)
	return m_dynamicPlaylist;

    return currentItem() ? static_cast<Item *>(currentItem())->playlist() : 0;
}

void PlaylistBox::setupPlaylist(Playlist *playlist, const QString &iconName)
{
    PlaylistCollection::setupPlaylist(playlist, iconName);
    new Item(this, iconName, playlist->name(), playlist);
}

void PlaylistBox::setupPlaylist(Playlist *playlist, const QString &iconName, Item *parentItem)
{
    PlaylistCollection::setupPlaylist(playlist, iconName);
    new Item(parentItem, iconName, playlist->name(), playlist);
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

void PlaylistBox::remove()
{
    ItemList items = selectedItems();

    if(items.isEmpty())
	return;

    QStringList files;

    for(ItemList::ConstIterator it = items.begin(); it != items.end(); ++it) {
	if(*it && (*it)->playlist() && !(*it)->playlist()->fileName().isEmpty())
	    files.append((*it)->playlist()->fileName());
    }

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

    QValueList< QPair<Item *, Playlist *> > removeQueue;

    for(ItemList::ConstIterator it = items.begin(); it != items.end(); ++it) {
	if(*it != Item::collectionItem() &&
	   (*it)->playlist() &&
	   (!(*it)->playlist()->readOnly()))
	{
	    m_names.remove((*it)->text(0));
	    m_playlistDict.remove((*it)->playlist());
	    removeQueue.append(qMakePair(*it, (*it)->playlist()));
	}
    }

    if(items.back()->nextSibling() && static_cast<Item *>(items.back()->nextSibling())->playlist())
	setSingleItem(items.back()->nextSibling());
    else {
	Item *i = static_cast<Item *>(items.front()->itemAbove());
	while(i && !i->playlist())
	    i = static_cast<Item *>(i->itemAbove());

	if(!i)
	    i = Item::collectionItem();

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
    if(!s || (item && item->playlist() && item->playlist()->readOnly()))
	return;

    KURL::List urls;

    if(KURLDrag::decode(s, urls) && !urls.isEmpty()) {
	QStringList files;

	for(KURL::List::Iterator it = urls.begin(); it != urls.end(); it++)
	    files.append((*it).path());

	if(item && item->playlist())
	    item->playlist()->addFiles(files, true);
	else {
	    QString name = playlistNameDialog();
	    if(!name.isNull()) {
		Playlist *p = new Playlist(this, name);
		p->addFiles(files, true);
	    }
	}
    }
}

void PlaylistBox::contentsDropEvent(QDropEvent *e)
{
    Item *i = static_cast<Item *>(itemAt(contentsToViewport(e->pos())));
    decode(e, i);

    if(m_dropItem) {
	Item *old = m_dropItem;
	m_dropItem = 0;
	old->repaint();
    }
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

    Item *target = static_cast<Item *>(itemAt(contentsToViewport(e->pos())));

    if(target) {

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

	if(m_dropItem != target) {
	    Item *old = m_dropItem;

	    if(e->isAccepted()) {
		m_dropItem = target;
		target->repaint();
	    }
	    else
		m_dropItem = 0;

	    if(old)
		old->repaint();
	}
    }
    else {

	// We're dragging over the whitespace.  We'll use this case to make it
	// possible to create new lists.

	e->accept(true);
    }
}

void PlaylistBox::contentsDragLeaveEvent(QDragLeaveEvent *e)
{
    if(m_dropItem) {
	Item *old = m_dropItem;
	m_dropItem = 0;
	old->repaint();
    }
    KListView::contentsDragLeaveEvent(e);
}

void PlaylistBox::contentsMousePressEvent(QMouseEvent *e)
{
    if(e->button() == LeftButton)
	m_doingMultiSelect = true;
    KListView::contentsMousePressEvent(e);
}

void PlaylistBox::contentsMouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() == LeftButton) {
	m_doingMultiSelect = false;
	slotPlaylistChanged();
    }
    KListView::contentsMouseReleaseEvent(e);
}

void PlaylistBox::keyPressEvent(QKeyEvent *e)
{
    if((e->key() == Key_Up || e->key() == Key_Down) && e->state() == ShiftButton)
	m_doingMultiSelect = true;
    KListView::keyPressEvent(e);
}

void PlaylistBox::keyReleaseEvent(QKeyEvent *e)
{
    if(m_doingMultiSelect && e->key() == Key_Shift) {
	m_doingMultiSelect = false;
	slotPlaylistChanged();
    }
    KListView::keyReleaseEvent(e);
}

PlaylistBox::ItemList PlaylistBox::selectedItems()
{
    ItemList l;

    for(QListViewItemIterator it(this, QListViewItemIterator::Selected); it.current(); ++it)
	l.append(static_cast<Item *>(*it));

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
    // Don't update while the mouse is pressed down.

    if(m_doingMultiSelect)
	return;

    ItemList items = selectedItems();
    m_hasSelection = !items.isEmpty();

    if(!m_updatePlaylistStack)
	return;

    bool allowReload = false;

    PlaylistList playlists;
    for(ItemList::ConstIterator it = items.begin(); it != items.end(); ++it) {

	Playlist *p = (*it)->playlist();
	if(p) {
	    if(p == CollectionList::instance() || !p->fileName().isNull())
		allowReload = true;
	    playlists.append(p);
	}
    }

    if(playlists.isEmpty() ||
       (playlists.count() == 1 &&
	(playlists.front() == CollectionList::instance() ||
	 playlists.front()->readOnly())))
    {
        action("file_save")->setEnabled(false);
        action("file_save_as")->setEnabled(false);
        action("renamePlaylist")->setEnabled(false);
        action("deleteItemPlaylist")->setEnabled(false);
    }
    else {
        action("file_save")->setEnabled(true);
        action("file_save_as")->setEnabled(true);
        action("renamePlaylist")->setEnabled(playlists.count() == 1);
        action("deleteItemPlaylist")->setEnabled(true);
    }
    action("reloadPlaylist")->setEnabled(allowReload);
    action("duplicatePlaylist")->setEnabled(!playlists.isEmpty());

    if(m_k3bAction)
	m_k3bAction->setEnabled(!playlists.isEmpty());

    bool searchList =
	playlists.count() == 1 && dynamic_cast<SearchPlaylist *>(playlists.front());

    action("editSearch")->setEnabled(searchList);

    if(playlists.count() == 1)
	playlistStack()->raiseWidget(playlists.front());
}

void PlaylistBox::slotDoubleClicked()
{
    action("stop")->activate();
    action("play")->activate();
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

void PlaylistBox::setupItem(Item *item)
{
    m_playlistDict.insert(item->playlist(), item);
    viewMode()->queueRefresh();
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox::Item protected methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::Item *PlaylistBox::Item::m_collectionItem = 0;

PlaylistBox::Item::Item(PlaylistBox *listBox, const QString &icon, const QString &text, Playlist *l)
    : QObject(listBox), KListViewItem(listBox, text),
      m_playlist(l), m_text(text), m_iconName(icon), m_sortedFirst(false)
{
    init();
}

PlaylistBox::Item::Item(Item *parent, const QString &icon, const QString &text, Playlist *l)
    : QObject(parent->listView()), KListViewItem(parent, text),
    m_playlist(l), m_text(text), m_iconName(icon), m_sortedFirst(false)
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
	setSelected(true);

	listView()->sort();
	listView()->ensureItemVisible(listView()->currentItem());
    }
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox::Item private methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::Item::init()
{
    PlaylistBox *list = static_cast<PlaylistBox *>(listView());

    list->setupItem(this);

    int iconSize = list->viewModeIndex() == 0 ? 32 : 16;
    setPixmap(0, SmallIcon(m_iconName, iconSize));
    list->addName(m_text);

    if(m_playlist)
	connect(m_playlist, SIGNAL(signalNameChanged(const QString &)),
		this, SLOT(slotSetName(const QString &)));

    if(m_playlist == CollectionList::instance()) {
	m_sortedFirst = true;
	m_collectionItem = this;

	if(dynamic_cast<TreeViewMode *>(list->viewMode()))
	    static_cast<TreeViewMode *>(list->viewMode())->setupCategories();
    }
}

#include "playlistbox.moc"
