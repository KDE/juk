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

#include <kconfig.h>
#include <kmessagebox.h>
#include <kurldrag.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kaction.h>
#include <kmainwindow.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <kdebug.h>

#include <qheader.h>
#include <qcursor.h>
#include <qdir.h>
#include <qeventloop.h>

#include <stdlib.h>
#include <limits.h>
#include <time.h>

#include "playlist.h"
#include "playlistitem.h"
#include "playlistsearch.h"
#include "genrelistlist.h"
#include "collectionlist.h"
#include "mediafiles.h"

////////////////////////////////////////////////////////////////////////////////
// Playlist::SharedSettings definition
////////////////////////////////////////////////////////////////////////////////

bool Playlist::m_visibleChanged = false;

/**
 * Shared settings between the playlists.
 */

class Playlist::SharedSettings
{
public:
    static SharedSettings *instance();
    /**
     * Sets the default column order to that of Playlist @param p.
     */
    void setColumnOrder(const Playlist *l);
    void toggleColumnVisible(int column);

    /**
     * Apply the settings.
     */
    void apply(Playlist *l) const;

protected:
    SharedSettings();
    ~SharedSettings() {}

private:
    void writeConfig();

    static SharedSettings *m_instance;
    QValueList<int> m_columnOrder;
    QValueVector<bool> m_columnsVisible;
};

Playlist::SharedSettings *Playlist::SharedSettings::m_instance = 0;

////////////////////////////////////////////////////////////////////////////////
// Playlist::SharedSettings public members
////////////////////////////////////////////////////////////////////////////////

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

    for(int i = l->columnOffset(); i < l->columns(); ++i)
	m_columnOrder.append(l->header()->mapToIndex(i));

    writeConfig();
}

void Playlist::SharedSettings::toggleColumnVisible(int column)
{
    if(column >= int(m_columnsVisible.size()))
	m_columnsVisible.resize(column + 1, true);

    m_columnsVisible[column] = !m_columnsVisible[column];

    writeConfig();
}

void Playlist::SharedSettings::apply(Playlist *l) const
{
    if(!l)
	return;

    int offset = l->columnOffset();
    int i = 0;
    for(QValueListConstIterator<int> it = m_columnOrder.begin(); it != m_columnOrder.end(); ++it)
	l->header()->moveSection(i++ + offset, *it + offset);

    for(uint i = 0; i < m_columnsVisible.size(); i++) {
	if(m_columnsVisible[i] && !l->isColumnVisible(i + offset))
	    l->showColumn(i + offset);
	else if(!m_columnsVisible[i] && l->isColumnVisible(i + offset))
	    l->hideColumn(i + offset);
    }

    l->updateLeftColumn();
}

////////////////////////////////////////////////////////////////////////////////
// Playlist::ShareSettings protected members
////////////////////////////////////////////////////////////////////////////////

Playlist::SharedSettings::SharedSettings()
{
    KConfig *config = kapp->config();
    {
	KConfigGroupSaver saver(config, "PlaylistShared");

	// save column order
	m_columnOrder = config->readIntListEntry("ColumnOrder");

	QValueList<int> l = config->readIntListEntry("VisibleColumns");
	m_columnsVisible.resize(l.size(), true);

	// save visible columns
	uint i = 0;
	for(QValueList<int>::Iterator it = l.begin(); it != l.end(); ++it) {
	    if(! bool(*it))
		m_columnsVisible[i] = bool(*it);
	    i++;
	}
    }
}

////////////////////////////////////////////////////////////////////////////////
// Playlist::SharedSettings private members
////////////////////////////////////////////////////////////////////////////////

void Playlist::SharedSettings::writeConfig()
{
    KConfig *config = kapp->config();

    {
	KConfigGroupSaver saver(config, "PlaylistShared");
	config->writeEntry("ColumnOrder", m_columnOrder);

	QValueList<int> l;
	for(uint i = 0; i < m_columnsVisible.size(); i++)
	    l.append(int(m_columnsVisible[i]));

	config->writeEntry("VisibleColumns", l);
    }

    config->sync();
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

PlaylistItem *Playlist::m_playingItem = 0;
int Playlist::m_leftColumn = 0;

Playlist::Playlist(QWidget *parent, const QString &name) :
    KListView(parent, name.latin1()),
    m_playlistName(name),
    m_rmbMenu(0),
    m_selectedCount(0),
    m_lastSelected(0)

{
    setup();
}

Playlist::Playlist(const QFileInfo &playlistFile, QWidget *parent, const QString &name) : 
    KListView(parent, name.latin1()),
    m_fileName(playlistFile.absFilePath()),
    m_rmbMenu(0)
{
    setup();
    loadFile(m_fileName, playlistFile);
}

Playlist::~Playlist()
{

}

void Playlist::save()
{
    if(m_fileName.isEmpty())
	return saveAs();

    QFile file(m_fileName);

    if(!file.open(IO_WriteOnly))
	return KMessageBox::error(this, i18n("Could not save to file %1.").arg(m_fileName));

    QTextStream stream(&file);

    QStringList fileList = files();

    for(QStringList::Iterator it = fileList.begin(); it != fileList.end(); ++it)
	stream << *it << endl;

    file.close();
}

void Playlist::saveAs()
{
    m_fileName = MediaFiles::savePlaylistDialog(name(), this);

    if(!m_fileName.isEmpty()) {
	// If there's no playlist name set, use the file name.
	if(m_playlistName.isEmpty())
	    emit signalNameChanged(name());

	save();
    }
}

void Playlist::clearItem(PlaylistItem *item, bool emitChanged)
{
    emit signalAboutToRemove(item);
    m_members.remove(item->absFilePath());
    if (!m_randomList.isEmpty() && !m_visibleChanged)
        m_randomList.remove(item);
    item->deleteLater();
    if(emitChanged)
	emit signalCountChanged(this);
}

void Playlist::clearItems(const PlaylistItemList &items)
{
    for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it)
	clearItem(*it, false);

    // Since we're using deleteLater() in the above call and calls to this will expect
    // those items to have actually gone away.

    // kapp->processEvents();
    kapp->eventLoop()->processEvents(QEventLoop::ExcludeUserInput);

    emit signalCountChanged(this);
}

QStringList Playlist::files() const
{
    QStringList list;
    PlaylistItem *i = static_cast<PlaylistItem *>(firstChild());
    for(; i; i = static_cast<PlaylistItem *>(i->itemBelow()))
	list.append(i->absFilePath());

    return list;
}

PlaylistItemList Playlist::items()
{
    PlaylistItemList list;

    for(QListViewItemIterator it(this); it.current(); ++it)
	list.append(static_cast<PlaylistItem *>(it.current()));

    return list;
}

PlaylistItemList Playlist::visibleItems() const
{
    PlaylistItemList list;
    PlaylistItem *i = static_cast<PlaylistItem *>(firstChild());
    for(; i; i = static_cast<PlaylistItem *>(i->itemBelow())) {

        // This check should be removed at some point since those items should 
	// all be already visible at the time of writing there's a bug that 
	// leaves some invisible items in the list

        if(i->isVisible())
            list.append(i);
        else
            kdDebug(65432) << "File shouldn't be in the list" << i->fileName() << endl;
    }

    return list;
}

PlaylistItemList Playlist::selectedItems() const
{
    PlaylistItemList list;

    switch(m_selectedCount) {
    case 0:
	break;
    case 1:
	list.append(m_lastSelected);
	break;
    default:
	for(PlaylistItem *i = static_cast<PlaylistItem *>(firstChild());
	    i; 
	    i = static_cast<PlaylistItem *>(i->itemBelow()))
	{
	    if(i->isSelected())
		list.append(i);
	}
	break;
    }

    return list;
}

PlaylistItemList Playlist::historyItems(PlaylistItem *current, bool random) const
{
    PlaylistItemList list;

    if (random && !m_history.isEmpty()) {
        PlaylistItemList::ConstIterator it = m_history.end();
        --it;

        int j = 0;
        for(; it != m_history.begin() && j < 10; --it, ++j)
            list.append(*it);

        if (j < 10)
            list.append(*it);
    }
    else if(current) {
        current = static_cast<PlaylistItem *>(current->itemAbove());
        for(int j = 0; current && j < 10; ++j) {
            list.append(current);
            current = static_cast<PlaylistItem *>(current->itemAbove());
	}
    }

    return list;
}

PlaylistItem *Playlist::nextItem(PlaylistItem *current, bool random)
{
    if(!current)
	return 0;

    PlaylistItem *i;

    if(random) {
        if(m_randomList.count() <= 1 || m_visibleChanged) {
            m_randomList = visibleItems();
            m_visibleChanged = false; // got the change
        }

        m_randomList.remove(current);

        m_history.append(current);

        i = current;
        if(!m_randomList.isEmpty()) {
	    while(i == current)
		i = m_randomList[KApplication::random() % m_randomList.count()];
	}
    }
    else
    {
        m_history.clear();
	i = static_cast<PlaylistItem *>(current->itemBelow());
    }

    return i;
}

PlaylistItem *Playlist::previousItem(PlaylistItem *current, bool random)
{
    if(!current)
        return 0;

    if(random && !m_history.isEmpty()) {
        PlaylistItemList::Iterator last = m_history.fromLast();
        PlaylistItem *item = *last;
        m_history.remove(last);
        return item;
    }

    m_history.clear();
    if(!current->itemAbove())
        return current;

    return static_cast<PlaylistItem *>(current->itemAbove());
}

QString Playlist::name() const
{
    if(m_playlistName.isNull())
	return m_fileName.section(QDir::separator(), -1).section('.', 0, -2);
    else
	return m_playlistName;
}

void Playlist::setName(const QString &n)
{
    m_playlistName = n;
    emit signalNameChanged(m_playlistName);
}

void Playlist::updateLeftColumn()
{
    int newLeftColumn = leftMostVisibleColumn();

    if(m_leftColumn != newLeftColumn) {
	if(m_playingItem) {
	    m_playingItem->setPixmap(m_leftColumn, QPixmap(0, 0));
	    m_playingItem->setPixmap(newLeftColumn, QPixmap(UserIcon("playing")));
	}
	m_leftColumn = newLeftColumn;
    }
}

void Playlist::setItemsVisible(const PlaylistItemList &items, bool visible) // static
{
    m_visibleChanged = true;
    for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it)
	(*it)->setVisible(visible);
}

void Playlist::markItemSelected(PlaylistItem *item, bool selected)
{
    if(selected && !item->isSelected()) {
	m_selectedCount++;
	m_lastSelected = item;
    }
    else if(!selected && item->isSelected())
	m_selectedCount--;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::slotSetNext()
{
    if(!selectedItems().isEmpty())
	emit signalSetNext(selectedItems().first());
}

void Playlist::copy()
{
    kapp->clipboard()->setData(dragObject(0), QClipboard::Clipboard);
}

void Playlist::paste()
{
    decode(kapp->clipboard()->data(), static_cast<PlaylistItem *>(currentItem()));
}

void Playlist::clear()
{
    PlaylistItemList l = selectedItems();
    if(l.isEmpty())
	l = items();

    clearItems(l);
}

void Playlist::slotRefresh()
{
    PlaylistItemList l = selectedItems();
    if(l.isEmpty())
	l = visibleItems();

    KApplication::setOverrideCursor(Qt::waitCursor);
    int j = 0;
    for(PlaylistItemList::Iterator it = l.begin(); it != l.end(); ++it) {
	(*it)->slotRefreshFromDisk();
	if(j % 5 == 0)
	    kapp->processEvents();
	j = j % 5 + 1;
    }
    KApplication::restoreOverrideCursor();
}

void Playlist::slotRenameFile()
{
    KApplication::setOverrideCursor(Qt::waitCursor);
    PlaylistItemList items = selectedItems();
    for(PlaylistItemList::Iterator it = items.begin(); it != items.end(); ++it)
        (*it)->renameFile();
    KApplication::restoreOverrideCursor();
}

void Playlist::slotGuessTagInfo(TagGuesser::Type type)
{
    KApplication::setOverrideCursor(Qt::waitCursor);
    PlaylistItemList items = selectedItems();
    for(PlaylistItemList::Iterator it = items.begin(); it != items.end(); ++it)
        (*it)->guessTagInfo(type);
    KApplication::restoreOverrideCursor();
}

void Playlist::slotReload()
{
    QFileInfo fileInfo(m_fileName);
    if(!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable())
	return;

    clearItems(items());
    loadFile(m_fileName, fileInfo);
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void Playlist::removeFromDisk(const PlaylistItemList &items)
{
    if(isVisible() && !items.isEmpty()) {

        QStringList files;
	for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it)
            files.append((*it)->fileName());

	QString message;

	if(files.count() == 1)
	    message = i18n("Do you really want to delete this item from your disk?");
	else
	    message = i18n("Do you really want to delete these %1 items from your disk?").arg(QString::number(files.count()));

	if(KMessageBox::questionYesNoList(this, message, files) == KMessageBox::Yes) {
	    for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it) {
		if(QFile::remove((*it)->filePath())) {
		    emit signalAboutToRemove(*it);
                    if(!m_randomList.isEmpty() && !m_visibleChanged)
                        m_randomList.remove(*it);
		    delete *it;
		}
		else
		    KMessageBox::sorry(this, i18n("Could not delete ") + (*it)->fileName() + ".");
	    }

	}
	emit signalCountChanged(this);
    }
}

QDragObject *Playlist::dragObject(QWidget *parent)
{
    PlaylistItemList items = selectedItems();
    KURL::List urls;
    for(PlaylistItemList::Iterator it = items.begin(); it != items.end(); ++it) {
	KURL url;
	url.setPath((*it)->absFilePath());
	urls.append(url);
    }

    KURLDrag *drag = new KURLDrag(urls, parent, "Playlist Items");
    drag->setPixmap(BarIcon("sound"));

    return drag;
}

bool Playlist::canDecode(QMimeSource *s)
{
    KURL::List urls;
    return KURLDrag::decode(s, urls) && !urls.isEmpty();
}

void Playlist::decode(QMimeSource *s, PlaylistItem *after)
{
    KURL::List urls;

    if(!KURLDrag::decode(s, urls) || urls.isEmpty())
	return;

    QStringList fileList;

    for(KURL::List::Iterator it = urls.begin(); it != urls.end(); it++)
	fileList.append((*it).path());

    emit signalFilesDropped(fileList, this, after);
}

bool Playlist::eventFilter(QObject *watched, QEvent *e)
{
    if(watched == header()) {

	if(e->type() == QEvent::MouseButtonPress) {

	    QMouseEvent *me = static_cast<QMouseEvent*>(e);

	    switch(me->button()) {
	    case LeftButton:
		// If this is the empty column at the end of the listview, ignore
		// the click.
		if(header()->sectionAt(me->x()) == columns() - 1)
		    return true;
		break;
	    case RightButton:
		m_headerMenu->popup(QCursor::pos());
		return true;
		break;
	    default:
		break;
	    }
	}
    }

    return KListView::eventFilter(watched, e);
}

void Playlist::contentsDropEvent(QDropEvent *e)
{
    QPoint vp = contentsToViewport(e->pos());
    PlaylistItem *moveAfter = static_cast<PlaylistItem *>(itemAt(vp));

    // When dropping on the upper half of an item, insert before this item.
    // This is what the user expects, and also allows the insertion at
    // top of the list

    if(!moveAfter)
	moveAfter = static_cast<PlaylistItem *>(lastItem());
    else if(vp.y() < moveAfter->itemPos() + moveAfter->height() / 2)
	moveAfter = static_cast<PlaylistItem *>(moveAfter->itemAbove());

    if(e->source() == this) {

	// Since we're trying to arrange things manually, turn off sorting.

	setSorting(columns() + 1);

	QPtrList<QListViewItem> items = KListView::selectedItems();

	for(QPtrListIterator<QListViewItem> it(items); it.current(); ++it) {
	    if(!moveAfter) {

		// Insert the item at the top of the list.  This is a bit ugly,
		// but I don't see another way.

		takeItem(it.current());
		insertItem(it.current());
	    }
	    else
		it.current()->moveItem(moveAfter);

	    moveAfter = static_cast<PlaylistItem *>(it.current());
	}
    }
    else
	decode(e, moveAfter);
}

void Playlist::showEvent(QShowEvent *e)
{
    SharedSettings::instance()->apply(this);
    KListView::showEvent(e);
}

PlaylistItem *Playlist::createItem(const QFileInfo &file, const QString &absFilePath,
				   QListViewItem *after, bool emitChanged)
{
    QString filePath;

    if(absFilePath.isNull())
	filePath = resolveSymLinks(file);
    else
	filePath = absFilePath;

    CollectionListItem *item = CollectionList::instance()->lookup(filePath);

    if(!item) {
	item = new CollectionListItem(file, filePath);

	// If a valid tag was not created, destroy the CollectionListItem.
	if(!item->isValid()) {
	    kdError() << "Playlist::createItem() -- A valid tag was not created for \""
		      << file.filePath() << "\"" << endl;
	    delete item;
	    return 0;
	}
    }

    if(item && !m_members.insert(filePath) || m_allowDuplicates) {
	PlaylistItem *i;
	if(after)
	    i = new PlaylistItem(item, this, after);
	else
	    i = new PlaylistItem(item, this);
        if(!m_randomList.isEmpty() && !m_visibleChanged)
            m_randomList.append(i);
	emit signalCountChanged(this);
	connect(item, SIGNAL(destroyed()), i, SLOT(deleteLater()));

	if(emitChanged)
	    emit signalCountChanged(this);

	return i;
    }
    else
	return 0;
}

void Playlist::createItems(const PlaylistItemList &siblings)
{
    createItems<CollectionListItem, PlaylistItem, PlaylistItem>(siblings);
}

void Playlist::hideColumn(int c)
{
    m_headerMenu->setItemChecked(c, false);

    setColumnWidthMode(c, Manual);
    setColumnWidth(c, 0);
    setResizeMode(QListView::LastColumn);
    triggerUpdate();

    if(c == m_leftColumn) {
	if(m_playingItem) {
	    m_playingItem->setPixmap(m_leftColumn, QPixmap(0, 0));
	    m_playingItem->setPixmap(leftMostVisibleColumn(), QPixmap(UserIcon("playing")));
	}
	m_leftColumn = leftMostVisibleColumn();
    }
    emit signalVisibleColumnsChanged();
}

void Playlist::showColumn(int c)
{
    m_headerMenu->setItemChecked(c, true);

    setColumnWidthMode(c, Maximum);

    int w = 0;
    QListViewItemIterator it(this);
    for (; it.current(); ++it )
	w = QMAX(it.current()->width(fontMetrics(), this, c), w);

    setColumnWidth(c, w);
    triggerUpdate();

    if(c == leftMostVisibleColumn()) {
	if(m_playingItem) {
	    m_playingItem->setPixmap(m_leftColumn, QPixmap(0, 0));
	    m_playingItem->setPixmap(leftMostVisibleColumn(), QPixmap(UserIcon("playing")));
	}
	m_leftColumn = leftMostVisibleColumn();
    }
    emit signalVisibleColumnsChanged();
}

bool Playlist::isColumnVisible(int c) const
{
    return columnWidth(c) != 0;
}

// Though it's somewhat obvious, this function will stat the file, so only use it when
// you're out of a performance critical loop.

QString Playlist::resolveSymLinks(const QFileInfo &file) // static
{
    char real[PATH_MAX];
    if(file.exists() && realpath(QFile::encodeName(file.absFilePath()).data(), real))
	return QFile::decodeName(real);
    else
	return file.filePath();

}

void Playlist::polish()
{
    KListView::polish();

    if(m_polished)
	return;

    m_polished = true;

    addColumn(i18n("Track Name"));
    addColumn(i18n("Artist"));
    addColumn(i18n("Album"));
    addColumn(i18n("Track"));
    addColumn(i18n("Genre"));
    addColumn(i18n("Year"));
    addColumn(i18n("Length"));
    addColumn(i18n("Comment"));
    addColumn(i18n("File Name"));

    setSorting(1);

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

    //////////////////////////////////////////////////
    // setup header RMB menu
    //////////////////////////////////////////////////

    m_columnVisibleAction = new KActionMenu(i18n("&Show Columns"), this, "showColumns");

    m_headerMenu = m_columnVisibleAction->popupMenu();
    m_headerMenu->insertTitle(i18n("Show"));
    m_headerMenu->setCheckable(true);

    for(int i = 0; i < header()->count(); ++i) {
	m_headerMenu->insertItem(header()->label(i), i);
	m_headerMenu->setItemChecked(i, true);

#if QT_VERSION >= 0x030200
	adjustColumn(i);
#endif
    }

    connect(m_headerMenu, SIGNAL(activated(int)), this, SLOT(slotToggleColumnVisible(int)));

    //////////////////////////////////////////////////
    // hide some columns by default
    //////////////////////////////////////////////////

    hideColumn(PlaylistItem::CommentColumn + columnOffset());
    hideColumn(PlaylistItem::FileNameColumn + columnOffset());

    connect(this, SIGNAL(selectionChanged()),
	    this, SLOT(slotEmitSelected()));
    connect(this, SIGNAL(contextMenuRequested(QListViewItem *, const QPoint &, int)),
	    this, SLOT(slotShowRMBMenu(QListViewItem *, const QPoint &, int)));
    connect(this, SIGNAL(itemRenamed(QListViewItem *, const QString &, int)),
	    this, SLOT(slotApplyModification(QListViewItem *, const QString &, int)));

    addColumn(QString::null);
    setResizeMode(QListView::LastColumn);

    setAcceptDrops(true);
    setDropVisualizer(true);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void Playlist::setup()
{
    m_polished = false;
    m_allowDuplicates = false;

    setItemMargin(3);

    connect(header(), SIGNAL(indexChange(int, int, int)), this, SLOT(slotColumnOrderChanged(int, int, int)));
    connect(this, SIGNAL(signalDataChanged()), this, SIGNAL(signalChanged()));
    connect(this, SIGNAL(signalCountChanged(Playlist *)), this, SIGNAL(signalChanged()));
}

void Playlist::loadFile(const QString &fileName, const QFileInfo &fileInfo)
{
    QFile file(fileName);
    if(!file.open(IO_ReadOnly))
	return;

    QTextStream stream(&file);

    // turn off non-explicit sorting
    setSorting(columns() + 1);

    PlaylistItem *after = 0;

    while(!stream.atEnd()) {
	QString itemName = stream.readLine().stripWhiteSpace();

	QFileInfo item(itemName);

	if(item.isRelative())
	    item.setFile(QDir::cleanDirPath(fileInfo.dirPath(true) + "/" + itemName));

	if(MediaFiles::isMediaFile(item.fileName()) && item.exists() && item.isFile() && item.isReadable()) {
	    if(after)
		after = createItem(item, QString::null, after, false);
	    else
		after = createItem(item, QString::null, 0, false);
	}
    }

    file.close();

    emit signalCountChanged(this);
}

void Playlist::setPlaying(PlaylistItem *item, bool p)
{
    if(p) {
	m_playingItem = item;
	item->setPixmap(m_leftColumn, QPixmap(UserIcon("playing")));
    }
    else {
	m_playingItem = 0;
	item->setPixmap(m_leftColumn, QPixmap(0, 0));
    }

    item->setPlaying(p);
}

bool Playlist::playing() const
{
    return m_playingItem && this == static_cast<Playlist *>(m_playingItem->listView());
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::slotShowRMBMenu(QListViewItem *item, const QPoint &point, int column)
{
    if(!item)
	return;

    // Create the RMB menu on demand.

    if(!m_rmbMenu) {

	// A bit of a hack to get a pointer to the action collection.
	// Probably more of these actions should be ported over to using KActions.

	KActionCollection *actionCollection =
	    static_cast<KMainWindow *>(kapp->mainWidget())->actionCollection();

	m_rmbMenu = new KPopupMenu(this);

	m_rmbMenu->insertItem(SmallIconSet("player_play"), i18n("Play Next"), this, SLOT(slotSetNext()));
	m_rmbMenu->insertSeparator();

	if(!readOnly())
	    m_rmbMenu->insertItem(SmallIconSet("editcut"), i18n("Cut"), this, SLOT(cut()));

	m_rmbMenu->insertItem(SmallIconSet("editcopy"), i18n("Copy"), this, SLOT(copy()));

	if(!readOnly()) {
	    m_rmbPasteID = m_rmbMenu->insertItem(SmallIconSet("editpaste"), i18n("Paste"), this, SLOT(paste()));
	    m_rmbMenu->insertItem(SmallIconSet("editclear"), i18n("Clear"), this, SLOT(clear()));

	    m_rmbMenu->insertSeparator();
	}

	m_rmbEditID = m_rmbMenu->insertItem(SmallIconSet("edittool"), i18n("Edit"), this, SLOT(slotRenameTag()));

	if(!readOnly()) {
	    m_rmbMenu->insertItem(SmallIconSet("reload"), i18n("Refresh Items"), this, SLOT(slotRefresh()));
	    m_rmbMenu->insertItem(SmallIconSet("editdelete"), i18n("Remove From Disk"), this, SLOT(slotRemoveSelectedItems()));
	}

	m_rmbMenu->insertSeparator();
	actionCollection->action("guessTag")->plug(m_rmbMenu);

	m_rmbMenu->insertSeparator();
	m_rmbMenu->insertItem(SmallIcon("new"), i18n("Create Playlist From Selected Items"), this, SLOT(slotCreateGroup()));
    }

    if(!readOnly())
	m_rmbMenu->setItemEnabled(m_rmbPasteID, canDecode(kapp->clipboard()->data()));

    // Ignore any columns added by subclasses.

    column -= columnOffset();

    bool showEdit =
	(column == PlaylistItem::TrackColumn) ||
	(column == PlaylistItem::ArtistColumn) ||
	(column == PlaylistItem::AlbumColumn) ||
	(column == PlaylistItem::TrackNumberColumn) ||
	(column == PlaylistItem::GenreColumn) ||
	(column == PlaylistItem::YearColumn);

    if(showEdit)
	m_rmbMenu->changeItem(m_rmbEditID,
		i18n("Edit %1").arg(columnText(column)));

    m_rmbMenu->setItemVisible(m_rmbEditID, showEdit);

    m_rmbMenu->popup(point);
    m_currentColumn = column;
}

void Playlist::slotRenameTag()
{
    // kdDebug(65432) << "Playlist::slotRenameTag()" << endl;

    // setup completions and validators

    CollectionList *list = CollectionList::instance();

    KLineEdit *edit = renameLineEdit();

    switch(m_currentColumn - columnOffset())
    {
    case PlaylistItem::TrackColumn:
	edit->completionObject()->setItems(list->uniqueSet(CollectionList::Artists));
	break;
    case PlaylistItem::AlbumColumn:
	edit->completionObject()->setItems(list->uniqueSet(CollectionList::Albums));
	break;
    case PlaylistItem::GenreColumn:
	QStringList genreStrings;
	GenreList genres = GenreListList::ID3v1List();
	for(GenreList::Iterator it = genres.begin(); it != genres.end(); ++it)
	    genreStrings.append((*it).name());
	edit->completionObject()->setItems(genreStrings);
	break;
    }

    edit->setCompletionMode(KGlobalSettings::CompletionAuto);

    m_editText = currentItem()->text(m_currentColumn);

    rename(currentItem(), m_currentColumn);
}

void Playlist::applyTag(QListViewItem *item, const QString &text, int column)
{
    PlaylistItem *i = static_cast<PlaylistItem *>(item);

    switch(column - columnOffset())
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
	i->tag()->setGenre(Genre(text));
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
    i->slotRefresh();
}

void Playlist::slotApplyModification(QListViewItem *, const QString &text, int column)
{
    QPtrList<QListViewItem> selectedSongs = KListView::selectedItems();
    if(text == m_editText || 
       (selectedSongs.count() > 1 && KMessageBox::warningYesNo(
	    0,
	    i18n("This will edit multiple files! Are you sure?"),
	    QString::null,
	    KStdGuiItem::yes(),
	    KStdGuiItem::no(),
	    "DontWarnMultipleTags") == KMessageBox::No))
    {
	return;
    }

    QPtrListIterator<QListViewItem> it(selectedSongs);
    for(; it.current(); ++it) {
	applyTag((*it), text, column);
	kapp->processEvents();
    }
}

void Playlist::slotColumnOrderChanged(int, int from, int to)
{
    if(from == 0 || to == 0) {
	if(m_playingItem) {
	    m_playingItem->setPixmap(m_leftColumn, QPixmap(0, 0));
	    m_playingItem->setPixmap(header()->mapToSection(0), QPixmap(UserIcon("playing")));
	}
	m_leftColumn = header()->mapToSection(0);
    }

    SharedSettings::instance()->setColumnOrder(this);
}

void Playlist::slotToggleColumnVisible(int column)
{
    if(isColumnVisible(column))
	hideColumn(column);
    else
	showColumn(column);

    SharedSettings::instance()->toggleColumnVisible(column - columnOffset());
}

int Playlist::leftMostVisibleColumn() const
{
    int i = 0;
    while(!isColumnVisible(header()->mapToSection(i)) && i < PlaylistItem::lastColumn())
	i++;

    return header()->mapToSection(i);
}

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

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
	after = p.createItem(info, *it, after, false);
    }

    p.emitCountChanged();

    return s;
}

#include "playlist.moc"
