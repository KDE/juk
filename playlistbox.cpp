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
#include <klocale.h>
#include <kapplication.h>
#include <klineeditdlg.h>
#include <kpopupmenu.h>
#include <kaction.h>
#include <kdebug.h>

#include <qfile.h>
#include <qdrawutil.h>
#include <qclipboard.h>
#include <qheader.h>
#include <qpainter.h>
#include <qregexp.h>

#include "playlistbox.h"
#include "collectionlist.h"
#include "playlistsplitter.h"

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::PlaylistBox(PlaylistSplitter *parent, const char *name) : KListView(parent, name),
								       m_splitter(parent),
								       m_updatePlaylistStack(true),
								       m_viewMode(Compact)
{
    readConfig();
    addColumn("Playlists", width());
    header()->hide();
    setSorting(0);
    setFullWidth(true);
    setItemMargin(3);
    // setAlternateBackground(QColor());

    
    // Sadly the actions from the main menu can't be reused because these require being enabled and disabled at
    // different times.

    m_playlistContextMenu = new KPopupMenu();

    m_popupIndex["save"] = m_playlistContextMenu->insertItem(
	SmallIconSet("filesave"), i18n("Save"), this, SLOT(slotContextSave()));

    m_popupIndex["saveas"] = m_playlistContextMenu->insertItem(
        SmallIconSet("filesaveas"), i18n("Save As..."), this, SLOT(slotContextSaveAs()));

    m_popupIndex["rename"] = m_playlistContextMenu->insertItem(
	i18n("Rename..."), this, SLOT(slotContextRename()));

    m_popupIndex["duplicate"] = m_playlistContextMenu->insertItem(
	SmallIconSet("editcopy"), i18n("Duplicate..."), this, SLOT(slotContextDuplicate()));

    m_popupIndex["remove"] = m_playlistContextMenu->insertItem( 
	SmallIconSet("edittrash"), i18n("Remove"), this, SLOT(slotContextDeleteItem()));

    m_popupIndex["reload"] = m_playlistContextMenu->insertItem(
	SmallIconSet("reload"), i18n("Reload Playlist File"), this, SLOT(slotContextReload()));

    // add the view modes stuff

    m_viewModeAction = new KSelectAction(m_playlistContextMenu, "viewModeMenu");
    m_viewModeAction->setText(i18n("View Modes"));

    QStringList modes;
    modes << i18n("Default") << i18n("Compact");
    m_viewModeAction->setItems(modes);
    m_viewModeAction->setCurrentItem(m_viewMode);

    m_viewModeAction->plug(m_playlistContextMenu);
    connect(m_viewModeAction, SIGNAL(activated(int)), this, SLOT(slotSetViewMode(int)));

    setAcceptDrops(true);
    setSelectionModeExt(Extended);

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
    delete m_playlistContextMenu;
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

    for(QListViewItemIterator it(this); it.current(); ++it) {
	Item *i = static_cast<Item *>(*it);
	if(i->playlist() != CollectionList::instance())
	    l.append(i->playlist());
    }

    return l;
}

void PlaylistBox::save()
{
    save(static_cast<Item *>(selectedItem()));
}

void PlaylistBox::saveAs()
{
    saveAs(static_cast<Item *>(selectedItem()));
}

void PlaylistBox::rename()
{
    rename(static_cast<Item *>(selectedItem()));
}

void PlaylistBox::duplicate()
{
    duplicate(static_cast<Item *>(selectedItem()));
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::deleteItem()
{
    deleteItem(static_cast<Item *>(selectedItem()));
}

void PlaylistBox::paste()
{
    Item *i = static_cast<Item *>(selectedItem());
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
    QValueList<Item *> items = selectedItems();
    if(m_updatePlaylistStack && !items.isEmpty()) {
	QValueList<Playlist *> playlists;
	for(QValueList<Item *>::iterator i = items.begin(); i != items.end(); ++i)
	    playlists.append((*i)->playlist());

	emit signalCurrentChanged(playlists);
    }
}

void PlaylistBox::slotDoubleClicked(QListViewItem *)
{
    emit signalDoubleClicked();
}

void PlaylistBox::slotShowContextMenu(QListViewItem *item, const QPoint &point, int)
{
    Item *i = static_cast<Item *>(item);

    m_contextMenuOn = i;

    if(i) {
	bool isCollection = i->playlist() == CollectionList::instance();
	bool hasFile = !i->playlist()->fileName().isEmpty();
	
	m_playlistContextMenu->setItemEnabled(m_popupIndex["save"], !isCollection);
	m_playlistContextMenu->setItemEnabled(m_popupIndex["saveas"], !isCollection);
	m_playlistContextMenu->setItemEnabled(m_popupIndex["rename"], !isCollection);
	m_playlistContextMenu->setItemEnabled(m_popupIndex["remove"], !isCollection);
	m_playlistContextMenu->setItemEnabled(m_popupIndex["reload"], !isCollection);
	m_playlistContextMenu->setItemEnabled(m_popupIndex["reload"], hasFile);

	m_playlistContextMenu->popup(point);
    }
}

void PlaylistBox::slotContextSave()
{
    save(m_contextMenuOn);
}

void PlaylistBox::slotContextSaveAs()
{
    saveAs(m_contextMenuOn);
}

void PlaylistBox::slotContextRename()
{
    rename(m_contextMenuOn);
}

void PlaylistBox::slotContextDuplicate()
{
    duplicate(m_contextMenuOn);
}

void PlaylistBox::slotContextDeleteItem()
{
    deleteItem(m_contextMenuOn);
    m_contextMenuOn = 0;
}

void PlaylistBox::slotContextReload()
{
    reload(m_contextMenuOn);
    m_contextMenuOn = 0;
}

void PlaylistBox::slotSetViewMode(int viewMode)
{
    if(viewMode != m_viewMode) {
	m_viewMode = ViewMode(viewMode);
	int iconSize = m_viewMode == Compact ? 16 : 32;
	for(QListViewItemIterator it(this); it.current(); ++it) {
	    Item *i = static_cast<Item *>(*it);
	    i->setPixmap(0, SmallIcon(i->iconName(), iconSize));
	}
    }
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::Item::Item(PlaylistBox *listBox, const char *icon, const QString &text, Playlist *l) 
    : QObject(listBox), KListViewItem(listBox, text),
      m_list(l), m_text(text), m_iconName(icon)
{
    int iconSize = listBox->viewMode() == PlaylistBox::Compact ? 16 : 32;
    setPixmap(0, SmallIcon(icon, iconSize));
    listBox->addName(text);

    connect(l, SIGNAL(signalNameChanged(const QString &)), this, SLOT(slotSetName(const QString &)));
}

PlaylistBox::Item::~Item()
{

}

int PlaylistBox::Item::compare(QListViewItem *i, int col, bool) const
{
    if(playlist() == CollectionList::instance())
	return -1;
    else if(static_cast<Item *>(i)->playlist() == CollectionList::instance())
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

#include "playlistbox.moc"
