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
#include <klineeditdlg.h>
#include <kpopupmenu.h>
#include <kaction.h>
#include <kmainwindow.h>
#include <kdebug.h>

#include <qheader.h>
#include <qpainter.h>
#include <qregexp.h>

#include "playlistbox.h"
#include "playlistsplitter.h"
#include "playlistsearch.h"

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::PlaylistBox(PlaylistSplitter *parent, const char *name) : KListView(parent, name),
								       m_splitter(parent),
								       m_updatePlaylistStack(true),
								       m_viewMode(Compact),
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
    actions->action("deleteItemPlaylist")->plug(m_contextMenu);
    actions->action("file_save")->plug(m_contextMenu);
    actions->action("file_save_as")->plug(m_contextMenu);
    
    // add the view modes stuff
	
    m_viewModeAction = new KSelectAction(actions, "viewModeMenu");
    m_viewModeAction->setText(i18n("View Modes"));
    
    QStringList modes;
    modes << i18n("Default") << i18n("Compact") << i18n("Tree");
    m_viewModeAction->setItems(modes);
    m_viewModeAction->setCurrentItem(m_viewMode);
    
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

void PlaylistBox::createItem(Playlist *playlist, const char *icon, bool raise)
{
    if(!playlist)
	return;

    Item *i = new Item(this, icon, playlist->name(), playlist);
    m_playlistDict.insert(playlist, i);

    if(raise) {
	setSingleItem(i);
	ensureCurrentVisible();
    }
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

    for(QListViewItem *it = firstChild(); it; it = it->nextSibling()) {
	Item *i = static_cast<Item *>(it);
	if(i->playlist() != CollectionList::instance())
	    l.append(i->playlist());
    }

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

void PlaylistBox::initViewMode()
{
    int iconSize = m_viewMode != Default ? 16 : 32;
    for(QListViewItem *it = firstChild(); it; it = it->nextSibling()) {
	Item *i = static_cast<Item *>(it);
	i->setPixmap(0, SmallIcon(i->iconName(), iconSize));

	for(QListViewItem *n = i->firstChild(); n; n = n->nextSibling())
	    n->setVisible(m_viewMode == Tree);
	if(m_viewMode == Tree && i->childCount() == 0)
	    i->slotSetData();
    }
    setRootIsDecorated(m_viewMode == Tree);
    setColumnWidthMode(0, (m_viewMode == Tree) ? Maximum : Manual);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::deleteItem()
{
    deleteItem(static_cast<Item *>(currentItem()));
}

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
	m_viewMode = (ViewMode) config->readNumEntry("ViewMode", 0);
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
    kdDebug(65432) << "saveAs() - " << bool(item) << endl;
    if(item)
        item->playlist()->saveAs();
}

void PlaylistBox::rename(Item *item)
{
    if(!item)
	return;

    bool ok;

    QString name = KLineEditDlg::getText(i18n("Rename"),
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

    QString name = KLineEditDlg::getText(i18n("New Playlist"), 
        i18n("Please enter a name for the new playlist:"), 
        m_splitter->uniquePlaylistName(item->text(0), true), &ok);

	if(ok) {
	    Playlist *p = m_splitter->createPlaylist(name);
	    p->createItems(item->playlist()->items());
	}
    }
}

void PlaylistBox::deleteItem(Item *item)
{
    if(!item || !item->playlist())
	return;

    if(!item->playlist()->fileName().isEmpty()) {
	int remove = KMessageBox::warningYesNoCancel(this, i18n("Do you want to delete this file from the disk as well?"));
	
	if(remove == KMessageBox::Yes) {
	    if(!QFile::remove(item->playlist()->fileName()))
		KMessageBox::sorry(this, i18n("Could not delete the specified file."));
	}
	else if(remove == KMessageBox::Cancel)
	    return;
    }
    else {
	if(KMessageBox::warningYesNo(this, i18n("Are you sure you want to remove this item?")) == KMessageBox::No)
	    return;
    }
    
    m_names.remove(item->text(0));
    m_playlistDict.remove(item->playlist());

    setSingleItem(item->nextSibling() ? item->nextSibling() : lastItem());

    delete item->playlist();
    delete item;
}

void PlaylistBox::reload(Item *item)
{
    if(item && item->playlist())
	item->playlist()->slotReload();
}

void PlaylistBox::decode(QMimeSource *s, Item *item)
{
    if(!s || !item || !item->playlist())
	return;

    KURL::List urls;
    
    if(KURLDrag::decode(s, urls) && !urls.isEmpty()) {
	QStringList files;
	
	for(KURL::List::Iterator it = urls.begin(); it != urls.end(); it++)
	    files.append((*it).path());
	
	m_splitter->slotAddToPlaylist(files, item->playlist());
	item->rootItem()->slotSetData();
    }
}

void PlaylistBox::contentsDropEvent(QDropEvent *e)
{
    Item *i = static_cast<Item *>(itemAt(e->pos()));
    decode(e, i);
}

void PlaylistBox::contentsDragMoveEvent(QDragMoveEvent *e)
{
    // If we can decode the input source, there is a non-null item at the "move"
    // position, the playlist for that Item is non-null, is not the 
    // selected playlist and is not the CollectionList, then accept the event.
    //
    // Otherwise, do not accept the event.
    
    if(KURLDrag::canDecode(e) && itemAt(e->pos())) {
	Item *target = static_cast<Item *>(itemAt(e->pos()));

	// This is a semi-dirty hack to check if the items are coming from within
	// JuK.  If they are not coming from a Playlist (or subclass) then the
	// dynamic_cast will fail and we can safely assume that the item is 
	// coming from outside of JuK.

	if(dynamic_cast<Playlist *>(e->source())) {
	    if(target->playlist() && target->playlist() != CollectionList::instance() && !target->isSelected())
		e->accept(true);
	    else
		e->accept(false);
	}
	else // the dropped items are coming from outside of JuK
	    e->accept(true);
    }
    else
	e->accept(false);
}

QValueList<PlaylistBox::Item *> PlaylistBox::selectedItems()
{
    QValueList<Item *> l;

    for(QListViewItem *it = firstChild(); it; it = it->nextSibling()) {
	if(isSelected(it))
	    l.append(static_cast<Item *>(it));
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
    QValueList<Item *> items = selectedItems();
    m_hasSelection = items.count() > 0;

    if(m_updatePlaylistStack) {

	QValueList<Playlist *> playlists;
	for(QValueList<Item *>::iterator i = items.begin(); i != items.end(); ++i)
	    playlists.append((*i)->playlist());

	emit signalCurrentChanged(playlists);
    }
    else {
	for(QListViewItemIterator it(this); it.current(); ++it) {
	    if(isSelected(*it)) {
		Item *i = static_cast<Item *>(*it);
		PlaylistList playlists;
		playlists.append(i->playlist());
		
		if(i->childCount() > 0)
		    i->playlist()->setItemsVisible(i->playlist()->items(), true);
		else {
		    ColumnList searchedColumns;
		    searchedColumns.append(i->category());
		    PlaylistSearch::Component component(i->text(), true, searchedColumns);
		    PlaylistSearch::ComponentList components;
		    components.append(component);
		    PlaylistSearch search(playlists, components);
		    Playlist::setItemsVisible(search.matchedItems(), true);    
		    Playlist::setItemsVisible(search.unmatchedItems(), false);
		}
		emit signalCurrentChanged(playlists);
		break;
	    }
	}
    }
}

void PlaylistBox::slotDoubleClicked(QListViewItem *)
{
    emit signalDoubleClicked();
}

void PlaylistBox::slotShowContextMenu(QListViewItem *, const QPoint &point, int)
{
    m_contextMenu->popup(point);
}

void PlaylistBox::slotSetViewMode(int viewMode)
{
    if(viewMode != m_viewMode) {
	m_viewMode = ViewMode(viewMode);
	initViewMode();
    }
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::Item::Item(PlaylistBox *listBox, const char *icon, const QString &text, Playlist *l) 
    : QObject(listBox), KListViewItem(listBox, text),
      m_list(l), m_text(text), m_iconName(icon), m_category(-1)
{
    int iconSize = listBox->viewMode() != PlaylistBox::Default ? 16 : 32;
    setPixmap(0, SmallIcon(icon, iconSize));
    listBox->addName(text);

    connect(l, SIGNAL(signalNameChanged(const QString &)), this, SLOT(slotSetName(const QString &)));
    connect(l, SIGNAL(signalDataChanged()),                                 this, SLOT(slotTriggerSetData()));
    connect(l, SIGNAL(signalNumberOfItemsChanged(Playlist *)),              this, SLOT(slotTriggerSetData()));
    connect(l, SIGNAL(signalFilesDropped(const QStringList &, Playlist *)), this, SLOT(slotSetData()));
}

PlaylistBox::Item::Item(Item *parent, const char *icon, const QString &text, int category, Playlist *l)
    : QObject(parent), KListViewItem(parent, text),
      m_list(l), m_text(text), m_iconName(icon), m_category(category)
{
    setPixmap(0, SmallIcon(icon, 16));
}

PlaylistBox::Item::~Item()
{

}

int PlaylistBox::Item::compare(QListViewItem *i, int col, bool) const
{
    if(playlist() == CollectionList::instance() && category() == -1)
	return -1;
    else if(static_cast<Item *>(i)->playlist() == CollectionList::instance() &&
	    static_cast<Item *>(i)->category() == -1)
	return 1;

    return text(col).lower().localeAwareCompare(i->text(col).lower());
}

void PlaylistBox::Item::paintCell(QPainter *painter, const QColorGroup &colorGroup, int column, int width, int align)
{
    if(width < pixmap(column)->width())
	return;

    QFontMetrics fm = painter->fontMetrics();
    
    QString line = m_text;
    
    switch(static_cast<PlaylistBox *>(listView())->viewMode()) {
    case Tree:
	KListViewItem::setText(column, line);
	KListViewItem::paintCell(painter, colorGroup, column, width, align);
	break;
    case Compact:
    {
	int baseWidth = pixmap(column)->width() + listView()->itemMargin() * 4;
	if(baseWidth + fm.width(line) > width) {
	    int ellipsisLength = fm.width("...");
	    if(width > baseWidth + ellipsisLength) {
		while(baseWidth + fm.width(line) + ellipsisLength > width)
		    line.truncate(line.length() - 1);
		line = line.append("...");
	    }
	    else
		line = "...";
	}
	KListViewItem::setText(column, line);
	KListViewItem::paintCell(painter, colorGroup, column, width, align);
	break;
    }
    default:
    {
	QStringList lines;
	while(!line.isEmpty()) {
	    int textLength = line.length();
	    while(textLength > 0 && 
		  fm.width(line.mid(0, textLength).stripWhiteSpace()) + listView()->itemMargin() * 2 > width)
	    {
		int i = line.findRev(QRegExp( "\\W"), textLength - 1);
		if(i > 0)
		    textLength = i;
		else
		    textLength--;
	    }
	    
	    lines.append(line.mid(0, textLength).stripWhiteSpace());
	    line = line.mid(textLength);
	}
	
	int y = listView()->itemMargin();
	const QPixmap *pm = pixmap(column);
	
	int height = 3 * listView()->itemMargin() + pm->height() + 
	    (fm.height() - fm.descent()) * lines.count();
	
	if(isSelected()) {
	    painter->fillRect(0, 0, width, height, colorGroup.brush(QColorGroup::Highlight));
	    painter->setPen(colorGroup.highlightedText());
	}
	else
	    painter->eraseRect(0, 0, width, height);
	
	if (!pm->isNull()) {
	    int x = (width - pm->width()) / 2;
	    x = QMAX(x, listView()->itemMargin());
	    painter->drawPixmap(x, y, *pm);
	}
	y += pm->height() + fm.height() - fm.descent();
	for(QStringList::Iterator it = lines.begin(); it != lines.end(); ++it) {
	    int x = (width - fm.width(*it)) / 2;
	    x = QMAX(x, listView()->itemMargin());
	    painter->drawText(x, y, *it);
	    y += fm.height() - fm.descent();
	}
	setHeight(height);
	break;
    }
    }
}

void PlaylistBox::Item::setText(int column, const QString &text)
{
    m_text = text;
    KListViewItem::setText(column, text);
}

PlaylistBox::Item *PlaylistBox::Item::rootItem()
{
    Item *i = this;
    while(i->category() != -1)
        i = static_cast<Item *>(i->KListViewItem::parent());
    return i;
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

void PlaylistBox::Item::slotTriggerSetData()
{
    // Trigger setting data in tree view later, to ensure deleteLater is done
    QTimer::singleShot(1, this, SLOT(slotSetData()));
}

void PlaylistBox::Item::slotSetData()
{
    static const char *categoryText[] =
        {I18N_NOOP("Artist"), I18N_NOOP("Album"), I18N_NOOP("Genre"), I18N_NOOP("Year")};
    static const int categoryIndex[] =
        {PlaylistItem::ArtistColumn, PlaylistItem::AlbumColumn, 
         PlaylistItem::GenreColumn,  PlaylistItem::YearColumn};
    static const int categoryCount = sizeof(categoryText)/sizeof(categoryText[0]);

    PlaylistBox *listBox = static_cast<PlaylistBox *>(listView());
    if(listBox->viewMode() != Tree && childCount() == 0)
        return;

    // Fetch item data and put in categories
    PlaylistItemList items = m_list->items();
    SortedStringList categories[categoryCount];
    for(PlaylistItemList::iterator item = items.begin(); item != items.end(); ++item) {
        Tag *tag = (*item)->tag();
        categories[0].insert(tag->artist());
        categories[1].insert(tag->album());
        categories[2].insert(tag->genre());
        categories[3].insert(tag->yearString());
    }

    // Delete items that no longer exist, and remove existing items from categories
    listBox->setUpdatesEnabled(false);
    if(childCount() > 0) {
	for(QListViewItem *i = firstChild(); i; ) {
	    Item *categoryItem = static_cast<Item *>(i);
	    i = i->nextSibling();

	    int category = categoryItem->category();
	    for(QListViewItem *n = categoryItem->firstChild(); n; ) {
		Item *currentItem = static_cast<Item *>(n);
		n = n->nextSibling();
		if(categories[category].contains(currentItem->text()))
		    categories[category].remove(currentItem->text());
		else
		    delete currentItem;
	    }
	}
    }

    // Find or create category items, and then add new items remaining in categories
    for(int category = 0; category < categoryCount; category++) {
	QStringList categoryList = categories[category].values();
	Item *categoryItem = 0;
	QListViewItem *item;
	for(item = firstChild(); item; item = item->nextSibling()) {
	    Item *currentItem = static_cast<Item *>(item);
	    if(currentItem->category() == category) {
		categoryItem = currentItem;
		break;
	    }
	}
	if(!item)
	    categoryItem = new Item(this, iconName(), i18n(categoryText[category]), category, m_list);
	for(QStringList::iterator text = categoryList.begin(); text != categoryList.end(); ++text)
	    if(!(*text).isEmpty())
		new Item(categoryItem, "cdimage", *text, categoryIndex[category], m_list);
	if(categoryItem->childCount() < 2) {
	    delete categoryItem;
	    categoryItem = 0;
	}
    }

    listBox->setUpdatesEnabled(true);
    listBox->triggerUpdate();
}

#include "playlistbox.moc"
