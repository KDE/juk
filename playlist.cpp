/***************************************************************************
                          playlist.cpp  -  description
                             -------------------
    begin                : Sat Feb 16 2002
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

#include <kapplication.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <kurldrag.h>
#include <kiconloader.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <klineedit.h>
#include <kaction.h>
#include <kdebug.h>

#include <qfileinfo.h>
#include <qdir.h>
#include <qtimer.h>
#include <qapplication.h>
#include <qptrlist.h>
#include <qheader.h>
#include <qcursor.h>
#include <qclipboard.h>

#include <stdlib.h>
#include <time.h>

#include "playlist.h"
#include "collectionlist.h"
#include "playlistsplitter.h"
#include "playlistbox.h"
#include "tag.h"
#include "genrelistlist.h"

Playlist::SharedSettings *Playlist::SharedSettings::m_instance = 0;

Playlist::SharedSettings *Playlist::SharedSettings::instance()
{
    if(!m_instance)
	m_instance = new SharedSettings;
    return m_instance;
}

void Playlist::SharedSettings::setColumnOrder(const Playlist *l)
{
    if(!l)
	return;

    m_columnOrder.clear();

    for(int i = 0; i < l->columns(); ++i)
	m_columnOrder.append(l->header()->mapToIndex(i));

    KConfig *config = kapp->config();
    {
	KConfigGroupSaver(config, "PlaylistShared");
	config->writeEntry("ColumnOrder", m_columnOrder);
	config->sync();
    }
}

void Playlist::SharedSettings::restoreColumnOrder(Playlist *l)
{
    if(!l)
	return;

    int i = 0;
    for(QValueListIterator<int> it = m_columnOrder.begin(); it != m_columnOrder.end(); ++it)
	l->header()->moveSection(i++, *it);

    l->updateLeftColumn();
}

Playlist::SharedSettings::SharedSettings()
{
    KConfig *config = kapp->config();
    {
	KConfigGroupSaver(config, "PlaylistShared");
	m_columnOrder = config->readIntListEntry("ColumnOrder");
    }
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Playlist::Playlist(PlaylistSplitter *s, QWidget *parent, const QString &name) : KListView(parent, name.latin1()), 
										playlistName(name), splitter(s), boxItem(0), 
										playingItem(0), leftColumn(0)
{
    setup();
}

Playlist::Playlist(PlaylistSplitter *s, const QFileInfo &playlistFile, QWidget *parent, const char *name) : KListView(parent, name), 
													    playlistFileName(playlistFile.absFilePath()),
													    splitter(s), playingItem(0), leftColumn(0)
													    
{
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
    
    file.close();
}

Playlist::~Playlist()
{

}

void Playlist::save()
{
    if(playlistFileName.isEmpty())
	return saveAs();
    
    QFile file(playlistFileName);

    if(!file.open(IO_WriteOnly))
	return KMessageBox::error(this, i18n("Could not save to file %1.").arg(playlistFileName));
    
    QTextStream stream(&file);

    QStringList fileList = files();

    for(QStringList::Iterator it = fileList.begin(); it != fileList.end(); ++it)
	stream << *it << endl;
    
    file.close();
}

void Playlist::saveAs()
{
    QStringList extensions = splitter->playlistExtensions();

    playlistFileName = KFileDialog::getSaveFileName(QString::null, splitter->extensionsString(extensions, i18n("Playlists")));
    playlistFileName = playlistFileName.stripWhiteSpace();

    if(playlistFileName != QString::null) {
	if(extensions.find(playlistFileName.section('.', -1)) == extensions.end())
	    playlistFileName.append('.' + extensions.first());
	
	if(playlistName.isEmpty())
	    emit(nameChanged(name()));
	
	save();
    }
}

void Playlist::refresh()
{
    PlaylistItemList list;

    KApplication::setOverrideCursor(Qt::waitCursor);
    int j = 0;
    for(PlaylistItem *i = static_cast<PlaylistItem *>(firstChild()); i; i = static_cast<PlaylistItem *>(i->itemBelow())) {
	i->refreshFromDisk();
	if(j % 5 == 0)
	    kapp->processEvents();
	j = j % 5 + 1;
    }
    KApplication::restoreOverrideCursor();
}

void Playlist::clearItems(const PlaylistItemList &items)
{
    QPtrListIterator<PlaylistItem> it(items);
    while(it.current()) {
	emit(aboutToRemove(it.current()));
	members.remove(it.current()->absFilePath());
        delete it.current();
        ++it;
    }
    emit(numberOfItemsChanged(this));
}

QStringList Playlist::files() const
{
    QStringList list;
    for(PlaylistItem *i = static_cast<PlaylistItem *>(firstChild()); i; i = static_cast<PlaylistItem *>(i->itemBelow()))
	list.append(i->absFilePath());

    return list;
}

PlaylistItemList Playlist::items() const
{
    PlaylistItemList list;
    for(PlaylistItem *i = static_cast<PlaylistItem *>(firstChild()); i; i = static_cast<PlaylistItem *>(i->itemBelow()))
	list.append(i);

    return list;
}

PlaylistItemList Playlist::selectedItems() const
{
    PlaylistItemList list;
    for(PlaylistItem *i = static_cast<PlaylistItem *>(firstChild()); i; i = static_cast<PlaylistItem *>(i->itemBelow()))
        if(i->isSelected())
            list.append(i);
    
    return list;
}

void Playlist::remove(const PlaylistItemList &items)
{
    if(isVisible() && !items.isEmpty()) {

        QStringList files;
	for(QPtrListIterator<PlaylistItem> it(items); it.current(); ++it)
            files.append(it.current()->fileName());

	QString message;

	if(files.count() == 1)
	    message = i18n("Do you really want to delete this item from your disk?");
	else
	    message = i18n("Do you really want to delete these %1 items from your disk?").arg(QString::number(files.count()));
	
	if(KMessageBox::questionYesNoList(this, message, files) == KMessageBox::Yes) {
	    for(QPtrListIterator<PlaylistItem> it(items); it.current(); ++it) {
		if(QFile::remove(it.current()->filePath())) {
		    emit(aboutToRemove(it.current()));
		    delete it.current();
		}
		else
		    KMessageBox::sorry(this, i18n("Could not delete ") + it.current()->fileName() + ".");
	    }

	}
	emit(numberOfItemsChanged(this));
    }
}

PlaylistItem *Playlist::nextItem(PlaylistItem *current, bool random)
{
    if(!current)
	return 0;

    PlaylistItem *i;

    if(random) {
	if(count() > 1) {
	    history.push(current);

	    srand(time(0));
	    i = current;
	    while(i == current)
		i = items().at(rand() % count());
	}
	else
	    i = 0;
    }
    else
	i = static_cast<PlaylistItem *>(current->itemBelow());	

    return i;
}

PlaylistItem *Playlist::previousItem(PlaylistItem *current, bool random)
{
    if(!current)
	return 0;

    if(random && !history.isEmpty())
	return history.pop();
    else {
        if(!current->itemAbove())
	    return current;
        else
            return static_cast<PlaylistItem *>(current->itemAbove());
    }
}

QString Playlist::name() const
{
    if(playlistName == QString::null)
	return playlistFileName.section(QDir::separator(), -1).section('.', 0, -2);
    else
	return playlistName;
}

void Playlist::setName(const QString &n)
{
    playlistName = n;
    emit(nameChanged(playlistName));
}

void Playlist::updateLeftColumn()
{
    int newLeftColumn = leftMostVisibleColumn();

    if(leftColumn != newLeftColumn) {
	if(playingItem) {
	    playingItem->setPixmap(leftColumn, QPixmap(0, 0));
	    playingItem->setPixmap(newLeftColumn, QPixmap(UserIcon("playing")));
	}
	leftColumn = newLeftColumn;
    }
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::copy()
{
    kapp->clipboard()->setData(dragObject(0), QClipboard::Clipboard);
}

void Playlist::paste()
{
    decode(kapp->clipboard()->data());
}

void Playlist::clear()
{
    clearItems(selectedItems());
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

QDragObject *Playlist::dragObject(QWidget *parent)
{
    PlaylistItemList items = selectedItems();
    KURL::List urls;
    for(PlaylistItem *i = items.first(); i; i = items.next()) {
	KURL url;
	url.setPath(i->absFilePath());
	urls.append(url);
    }
    
    KURLDrag *drag = new KURLDrag(urls, parent, "Playlist Items");
    drag->setPixmap(SmallIcon("sound"));

    return drag;
}

QDragObject *Playlist::dragObject()
{
    return dragObject(this);
}

bool Playlist::canDecode(QMimeSource *s)
{
    KURL::List urls;
    return KURLDrag::decode(s, urls) && !urls.isEmpty();
}

void Playlist::decode(QMimeSource *s)
{
    KURL::List urls;
    
    if(!KURLDrag::decode(s, urls) || urls.isEmpty())
	return;
    
    QStringList fileList;
    
    for(KURL::List::Iterator it = urls.begin(); it != urls.end(); it++)
	fileList.append((*it).path());
    
    if(splitter)
	splitter->add(fileList, this);
}

bool Playlist::eventFilter(QObject* watched, QEvent* e)
{
    if(watched->inherits("QHeader")) { // Gotcha!
	
	if(e->type() == QEvent::MouseButtonPress) {
	    
	    QMouseEvent *me = static_cast<QMouseEvent*>(e);

	    if(me->button() == Qt::RightButton) {
		headerMenu->popup(QCursor::pos());
		return true;
	    }
	}
    }

    return KListView::eventFilter(watched, e);
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
    else
	decode(e);
}

void Playlist::contentsDragMoveEvent(QDragMoveEvent *e)
{
    if(KURLDrag::canDecode(e))
	e->accept(true);
    else
	e->accept(false);
}

void Playlist::showEvent(QShowEvent *e)
{
    SharedSettings::instance()->restoreColumnOrder(this);
    KListView::showEvent(e);
}

PlaylistItem *Playlist::createItem(const QFileInfo &file, QListViewItem *after)
{
    QString filePath = file.absFilePath();

    CollectionListItem *item = CollectionList::instance()->lookup(filePath);

    if(!item && CollectionList::instance())
	item = new CollectionListItem(file, filePath);
    
    if(item && !members.insert(filePath) || allowDuplicates) {
	PlaylistItem *i;
	if(after)
	    i = new PlaylistItem(item, this, after);
	else
	    i = new PlaylistItem(item, this);
	emit(numberOfItemsChanged(this));
	connect(item, SIGNAL(destroyed()), i, SLOT(deleteLater()));
	return i;
    }
    else
	return 0;
}

void Playlist::hideColumn(int c)
{
    headerMenu->setItemChecked(c, false);

    setColumnWidthMode(c, Manual);
    setColumnWidth(c, 0);
    setResizeMode(QListView::LastColumn);
    triggerUpdate();

    if(c == leftColumn) {
	if(playingItem) {
	    playingItem->setPixmap(leftColumn, QPixmap(0, 0));
	    playingItem->setPixmap(leftMostVisibleColumn(), QPixmap(UserIcon("playing")));
	}
	leftColumn = leftMostVisibleColumn();
    }
}

void Playlist::showColumn(int c)
{
    headerMenu->setItemChecked(c, true);

    setColumnWidthMode(c, Maximum);
    
    int w = 0;
    QListViewItemIterator it(this);
    for (; it.current(); ++it ) 
	w = QMAX(it.current()->width(fontMetrics(), this, c), w);
    
    setColumnWidth(c, w);
    triggerUpdate();

    if(c == leftMostVisibleColumn()) {
	if(playingItem) {
	    playingItem->setPixmap(leftColumn, QPixmap(0, 0));
	    playingItem->setPixmap(leftMostVisibleColumn(), QPixmap(UserIcon("playing")));
	}
	leftColumn = leftMostVisibleColumn();
    }
}

bool Playlist::isColumnVisible(int c) const
{
    return columnWidth(c) != 0;
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

    // These settings aren't really respected in KDE < 3.1.1, fixed in CVS

    setRenameable(PlaylistItem::TrackColumn, true);
    setRenameable(PlaylistItem::ArtistColumn, true);
    setRenameable(PlaylistItem::AlbumColumn, true);
    setRenameable(PlaylistItem::TrackNumberColumn, true);
    setRenameable(PlaylistItem::GenreColumn, true);
    setRenameable(PlaylistItem::YearColumn, true);

    setAllColumnsShowFocus(true);
    setSelectionMode(QListView::Extended);
    setShowSortIndicator(true);
    setDropVisualizer(true);
    setItemMargin(3);


    setSorting(1);
	
    installEventFilter(header());
	
    //////////////////////////////////////////////////
    // setup header RMB menu
    //////////////////////////////////////////////////

    _columnVisibleAction = new KActionMenu(i18n("Show Columns"), this, "showColumns");
    headerMenu = _columnVisibleAction->popupMenu();
    headerMenu->insertTitle(i18n("Show"));
    headerMenu->setCheckable(true);

    for(int i =0; i < header()->count(); ++i) {

	headerMenu->insertItem(header()->label(i), i);

	headerMenu->setItemChecked(i, true);
    }

    connect(headerMenu, SIGNAL(activated(int)), this, SIGNAL(signalToggleColumnVisible(int)));

    //////////////////////////////////////////////////
    // setup playlist RMB menu
    //////////////////////////////////////////////////

    rmbMenu = new KPopupMenu(this);

    rmbMenu->insertItem(SmallIcon("editcut"), i18n("Cut"), this, SLOT(cut()));
    rmbMenu->insertItem(SmallIcon("editcopy"), i18n("Copy"), this, SLOT(copy()));
    rmbPasteID = rmbMenu->insertItem(SmallIcon("editpaste"), i18n("Paste"), this, SLOT(paste()));
    rmbMenu->insertItem(SmallIcon("editclear"), i18n("Clear"), this, SLOT(clear()));

    rmbMenu->insertSeparator();

    rmbMenu->insertItem(SmallIcon("editdelete"), i18n("Remove From Disk"), this, SLOT(removeSelectedItems()));

    rmbMenu->insertSeparator();

    rmbEditID = rmbMenu->insertItem(SmallIcon("edittool"), i18n("Edit"), this, SLOT(renameTag()));

    connect(this, SIGNAL(selectionChanged()), this, SLOT(emitSelected()));
    connect(this, SIGNAL(doubleClicked(QListViewItem *)), this, SLOT(emitDoubleClicked(QListViewItem *)));
    connect(this, SIGNAL(contextMenuRequested( QListViewItem *, const QPoint&, int)),
	    this, SLOT(showRMBMenu(QListViewItem *, const QPoint &, int)));
    connect(this, SIGNAL(itemRenamed(QListViewItem *, const QString &, int)),
	    this, SLOT(applyTags(QListViewItem *, const QString &, int)));

    //////////////////////////////////////////////////
    
    addColumn(QString::null);
    setResizeMode(QListView::LastColumn);

    setAcceptDrops(true);
    allowDuplicates = false;

    connect(header(), SIGNAL(indexChange(int, int, int)), this, SLOT(columnOrderChanged(int, int, int)));
}

void Playlist::setPlaying(PlaylistItem *item, bool playing)
{
    if(playing) {
	playingItem = item;
	item->setPixmap(leftColumn, QPixmap(UserIcon("playing")));
    }
    else {
	playingItem = 0;
	item->setPixmap(leftColumn, QPixmap(0, 0));
    }
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::showRMBMenu(QListViewItem *item, const QPoint &point, int column)
{
    if(!item)
	return;

    rmbMenu->setItemEnabled(rmbPasteID, canDecode(kapp->clipboard()->data()));

    bool showEdit = 
	(column == PlaylistItem::TrackColumn) || 
	(column == PlaylistItem::ArtistColumn) || 
	(column == PlaylistItem::AlbumColumn) ||
	(column == PlaylistItem::TrackNumberColumn) ||
	(column == PlaylistItem::GenreColumn) ||
	(column == PlaylistItem::YearColumn);

    rmbMenu->setItemEnabled(rmbEditID, showEdit);

    rmbMenu->popup(point);
    currentColumn = column;
}

void Playlist::renameTag()
{
    // setup completions and validators
    
    CollectionList *list = CollectionList::instance();

    KLineEdit *edit = renameLineEdit();

    switch(currentColumn)
    {
    case PlaylistItem::TrackColumn:
	edit->completionObject()->setItems(list->artists());
	break;
    case PlaylistItem::AlbumColumn:
	edit->completionObject()->setItems(list->albums());
	break;
    case PlaylistItem::GenreColumn:
	QStringList genreStrings;
	GenreList genres = GenreListList::ID3v1List();
	for(GenreList::Iterator it = genres.begin(); it != genres.end(); ++it)
	    genreStrings.append(*it);
	edit->completionObject()->setItems(genreStrings);
	break;
    }
	
    edit->setCompletionMode(KGlobalSettings::CompletionAuto);
	
    rename(currentItem(), currentColumn);
}

void Playlist::applyTags(QListViewItem *item, const QString &text, int column)
{
    // kdDebug() << "Applying " << text << " at column " << column << ", replacing \"" << item->text(column) << "\"" << endl;

    PlaylistItem *i = static_cast<PlaylistItem *>(item);

    switch(column)
    {
    case PlaylistItem::TrackColumn:
	i->tag()->setTrack(text);
	break;
    case PlaylistItem::ArtistColumn:
	i->tag()->setArtist(text);
	break;
    case PlaylistItem::AlbumColumn:
	i->tag()->setAlbum(text);
	break;
    case PlaylistItem::TrackNumberColumn:
    {
	bool ok;
	int value = text.toInt(&ok);
	if(ok)
	    i->tag()->setTrackNumber(value);
	break;
    }
    case PlaylistItem::GenreColumn:
	i->tag()->setGenre(text);
	break;
    case PlaylistItem::YearColumn:
    {
	bool ok;
	int value = text.toInt(&ok);
	if(ok)
	    i->tag()->setYear(value);
	break;
    }
    }

    i->tag()->save();
    i->refresh();
}

void Playlist::columnOrderChanged(int, int from, int to)
{
    // kdDebug() << /* "section: " << section << */ " from: " << from << " to: " << to << endl;
    
    if(from == 0 || to == 0) {
	if(playingItem) {
	    playingItem->setPixmap(leftColumn, QPixmap(0, 0));
	    playingItem->setPixmap(leftMostVisibleColumn(), QPixmap(UserIcon("playing")));
	}
	leftColumn = leftMostVisibleColumn();
    }

    SharedSettings::instance()->setColumnOrder(this);
}

int Playlist::leftMostVisibleColumn() const
{
    int i = 0;
    while(!isColumnVisible(header()->mapToSection(i)) && i < PlaylistItem::lastColumn())
	i++;
    
    return header()->mapToSection(i);
}

QDataStream &operator<<(QDataStream &s, const Playlist &p)
{
    s << p.name();
    s << p.fileName();
    s << p.files();
    
    return s;
}

QDataStream &operator>>(QDataStream &s, Playlist &p)
{
    QString buffer;

    s >> buffer;
    p.setName(buffer);

    s >> buffer;
    p.setFileName(buffer);

    QStringList files;
    s >> files;

    PlaylistItem *after = 0;

    p.setSorting(p.columns() + 1);

    for(QStringList::Iterator it = files.begin(); it != files.end(); ++it ) {
	QFileInfo info(*it);
	after = p.createItem(info, after);
    }
    
    return s;
}

#include "playlist.moc"
