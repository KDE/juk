/***************************************************************************
                          playlist.cpp  -  description
                             -------------------
    begin                : Sat Feb 16 2002
    copyright            : (C) 2002, 2003 by Scott Wheeler
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
#include <kpopupmenu.h>
#include <klocale.h>
#include <kdebug.h>

#include <qheader.h>
#include <qcursor.h>
#include <qdir.h>
#include <qeventloop.h>

#include <id3v1genres.h>

#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <math.h>

#include "playlist.h"
#include "playlistitem.h"
#include "playlistsearch.h"
#include "mediafiles.h"
#include "collectionlist.h"
#include "filerenamer.h"
#include "actioncollection.h"

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
    void setInlineCompletionMode(KGlobalSettings::Completion mode);

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
    KGlobalSettings::Completion m_inlineCompletion;
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

void Playlist::SharedSettings::setInlineCompletionMode(KGlobalSettings::Completion mode)
{
    m_inlineCompletion = mode;
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
	    l->showColumn(i + offset, false);
	else if(!m_columnsVisible[i] && l->isColumnVisible(i + offset))
	    l->hideColumn(i + offset, false);
    }

    l->updateLeftColumn();
    l->renameLineEdit()->setCompletionMode(m_inlineCompletion);
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

	if(l.isEmpty()) {

	    // Provide some default values for column visibility if none were
	    // read from the configuration file.

	    for(int i = 0; i <= PlaylistItem::lastColumn(); i++) {
		switch(i) {
		case PlaylistItem::CommentColumn:
		case PlaylistItem::FileNameColumn:
		    m_columnsVisible.append(false);
		    break;
		default:
		    m_columnsVisible.append(true);
		}
	    }
	}
	else {
	    // Convert the int list into a bool list.

	    m_columnsVisible.resize(l.size(), true);
	    uint i = 0;
	    for(QValueList<int>::Iterator it = l.begin(); it != l.end(); ++it) {
		if(! bool(*it))
		    m_columnsVisible[i] = bool(*it);
		i++;
	    }
	}

	m_inlineCompletion = KGlobalSettings::Completion(
	    config->readNumEntry("InlineCompletionMode", KGlobalSettings::CompletionAuto));
    }
    CollectionList::instance()->emitVisibleColumnsChanged();
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
	config->writeEntry("InlineCompletionMode", m_inlineCompletion);
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
    m_selectedCount(0),
    m_allowDuplicates(false),
    m_polished(false),
    m_applySharedSettings(true),
    m_disableColumnWidthUpdates(true),
    m_widthsDirty(true),
    m_lastSelected(0),
    m_playlistName(name),
    m_rmbMenu(0)
{
    setup();
}

Playlist::Playlist(const QFileInfo &playlistFile, QWidget *parent, const QString &name) :
    KListView(parent, name.latin1()),
    m_selectedCount(0),
    m_allowDuplicates(false),
    m_polished(false),
    m_applySharedSettings(true),
    m_disableColumnWidthUpdates(true),
    m_widthsDirty(true),
    m_lastSelected(0),
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
    m_members.remove(item->file().absFilePath());
    if(!m_randomList.isEmpty() && !m_visibleChanged)
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

    kapp->eventLoop()->processEvents(QEventLoop::ExcludeUserInput);

    emit signalCountChanged(this);
}

QStringList Playlist::files()
{
    QStringList list;

    for(QListViewItemIterator it(this); it.current(); ++it)
	list.append(static_cast<PlaylistItem *>(*it)->file().absFilePath());

    return list;
}

PlaylistItemList Playlist::items()
{
    return items(QListViewItemIterator::IteratorFlag(0));
}

PlaylistItemList Playlist::visibleItems()
{
    return items(QListViewItemIterator::Visible);
}

PlaylistItemList Playlist::selectedItems()
{
    PlaylistItemList list;

    switch(m_selectedCount) {
    case 0:
	break;
	// case 1:
	// list.append(m_lastSelected);
	// break;
    default:
	list = items(QListViewItemIterator::IteratorFlag(QListViewItemIterator::Selected |
							 QListViewItemIterator::Visible));
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
    PlaylistItem *i = 0;

    if(random) {
	if(m_randomList.count() <= 1 || m_visibleChanged) {
	    m_randomList = visibleItems();
	    m_visibleChanged = false;
	}

	if(current) {
	    m_randomList.remove(current);
	    m_history.append(current);
	}

        i = current;
	if(!m_randomList.isEmpty()) {
	    while(i == current)
		i = m_randomList[KApplication::random() % m_randomList.count()];
	}
    }
    else {
        m_history.clear();

	if(current)
	    i = static_cast<PlaylistItem *>(current->itemBelow());
	else {
	    QListViewItemIterator it(this, QListViewItemIterator::Visible);
	    i = static_cast<PlaylistItem *>(it.current());
	}
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

int Playlist::totalTime()
{
    int time = 0;
    QListViewItemIterator it(this);
    while (it.current()) {
	PlaylistItem *item = static_cast<PlaylistItem *>(it.current());
	if(item->file().current())
	    time += item->file().tag()->seconds();
	it++;
    }
    return time;
}

void Playlist::updateLeftColumn()
{
    int newLeftColumn = leftMostVisibleColumn();

    if(m_leftColumn != newLeftColumn) {
	if(m_playingItem) {
	    m_playingItem->setPixmap(m_leftColumn, QPixmap(0, 0));
	    m_playingItem->setPixmap(newLeftColumn, UserIcon("playing"));
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

	if(!(*it)->file().tag()) {
	    kdDebug(65432) << "Error while trying to refresh the tag.  "
			   << "This file has probably been removed."
			   << endl;
	    delete (*it)->collectionItem();
	}

	if(j % 5 == 0)
	    kapp->processEvents();
	j = j % 5 + 1;
    }
    KApplication::restoreOverrideCursor();
}

void Playlist::slotRenameFile()
{
    FileRenamer renamer;
    PlaylistItemList items = selectedItems();
    if(items.count() == 1) {
        renamer.rename(items[0]);
    } else {
        renamer.rename(items);
    }
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

void Playlist::slotWeightDirty(int column)
{
    if(column < 0) {
	m_weightDirty.clear();
	for(int i = 0; i < columns(); i++) {
	    if(isColumnVisible(i))
		m_weightDirty.append(i);
	}
	return;
    }

    if(m_weightDirty.find(column) == m_weightDirty.end())
	m_weightDirty.append(column);
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void Playlist::removeFromDisk(const PlaylistItemList &items)
{
    if(isVisible() && !items.isEmpty()) {

        QStringList files;
	for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it)
            files.append((*it)->file().absFilePath());

	QString message;

	if(files.count() == 1)
	    message = i18n("Do you really want to delete this item from your disk?");
	else
	    message = i18n("Do you really want to delete these %1 items from your disk?").arg(QString::number(files.count()));

	if(KMessageBox::warningYesNoList(this, message, files) == KMessageBox::Yes) {
	    for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it) {
		if(QFile::remove((*it)->file().absFilePath())) {
                    if(!m_randomList.isEmpty() && !m_visibleChanged)
                        m_randomList.remove(*it);
		    CollectionList::instance()->clearItem((*it)->collectionItem());
		}
		else
		    KMessageBox::sorry(this, i18n("Could not delete ") + (*it)->file().absFilePath() + ".");
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
	url.setPath((*it)->file().absFilePath());
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
	switch(e->type()) {
	case QEvent::MouseButtonPress:
	{
	    if(static_cast<QMouseEvent *>(e)->button() == RightButton)
		m_headerMenu->popup(QCursor::pos());
	    break;
	}
	case QEvent::MouseButtonRelease:
	{
	    if(m_widthsDirty)
		QTimer::singleShot(0, this, SLOT(slotUpdateColumnWidths()));
	    break;
	}

	default:
	    break;
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

    KListView::contentsDropEvent(e);
}

void Playlist::showEvent(QShowEvent *e)
{
    if(m_applySharedSettings) {
	SharedSettings::instance()->apply(this);
	m_applySharedSettings = false;
    }
    KListView::showEvent(e);
}

void Playlist::applySharedSettings()
{
    m_applySharedSettings = true;
}

void Playlist::viewportPaintEvent(QPaintEvent *pe)
{
    // If there are columns that need to be updated, well, update them.

    if(!m_weightDirty.isEmpty()) {
	calculateColumnWeights();
	slotUpdateColumnWidths();
    }

    KListView::viewportPaintEvent(pe);
}

void Playlist::viewportResizeEvent(QResizeEvent *re)
{
    // If the width of the view has changed, manually update the column
    // widths.

    if(re->size().width() != re->oldSize().width())
	slotUpdateColumnWidths();

    KListView::viewportResizeEvent(re);
}

void Playlist::addColumn(const QString &label)
{
    slotWeightDirty(columns());
    KListView::addColumn(label, 30);
}

PlaylistItem *Playlist::createItem(const QFileInfo &file, const QString &absFilePath,
				   QListViewItem *after, bool emitChanged)
{
    return createItem<PlaylistItem, CollectionListItem, CollectionList>(file, absFilePath, after, emitChanged);
}

void Playlist::createItems(const PlaylistItemList &siblings)
{
    createItems<CollectionListItem, PlaylistItem, PlaylistItem>(siblings);
}

void Playlist::hideColumn(int c, bool emitChanged)
{
    m_headerMenu->setItemChecked(c, false);

    if(!isColumnVisible(c))
	return;

    setColumnWidthMode(c, Manual);
    setColumnWidth(c, 0);

    if(c == m_leftColumn) {
	if(m_playingItem) {
	    m_playingItem->setPixmap(m_leftColumn, QPixmap(0, 0));
	    m_playingItem->setPixmap(leftMostVisibleColumn(), UserIcon("playing"));
	}
	m_leftColumn = leftMostVisibleColumn();
    }

    slotUpdateColumnWidths();
    triggerUpdate();

    if(this != CollectionList::instance())
	CollectionList::instance()->hideColumn(c, false);

    if(emitChanged)
	CollectionList::instance()->emitVisibleColumnsChanged();
}

void Playlist::showColumn(int c, bool emitChanged)
{
    m_headerMenu->setItemChecked(c, true);

    if(isColumnVisible(c))
	return;

    // Just set the width to one to mark the column as visible -- we'll update
    // the real size in the next call.

    setColumnWidth(c, 1);

    if(c == leftMostVisibleColumn()) {
	if(m_playingItem) {
	    m_playingItem->setPixmap(m_leftColumn, QPixmap(0, 0));
	    m_playingItem->setPixmap(leftMostVisibleColumn(), UserIcon("playing"));
	}
	m_leftColumn = leftMostVisibleColumn();
    }

    slotUpdateColumnWidths();
    triggerUpdate();

    if(this != CollectionList::instance())
	CollectionList::instance()->showColumn(c, false);

    if(emitChanged)
	CollectionList::instance()->emitVisibleColumnsChanged();
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

    m_columnFixedWidths.resize(columns(), 0);

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
	adjustColumn(i);
    }

    connect(m_headerMenu, SIGNAL(activated(int)), this, SLOT(slotToggleColumnVisible(int)));

    connect(this, SIGNAL(selectionChanged()),
	    this, SLOT(slotEmitSelected()));
    connect(this, SIGNAL(contextMenuRequested(QListViewItem *, const QPoint &, int)),
	    this, SLOT(slotShowRMBMenu(QListViewItem *, const QPoint &, int)));
    connect(this, SIGNAL(itemRenamed(QListViewItem *, const QString &, int)),
	    this, SLOT(slotInlineEditDone(QListViewItem *, const QString &, int)));

    connect(header(), SIGNAL(sizeChange(int, int, int)),
	    this, SLOT(slotColumnSizeChanged(int, int, int)));

    connect(renameLineEdit(), SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
	    this, SLOT(slotInlineCompletionModeChanged(KGlobalSettings::Completion)));

    setHScrollBarMode(AlwaysOff);

    setAcceptDrops(true);
    setDropVisualizer(true);

    m_disableColumnWidthUpdates = false;
}

void Playlist::setupItem(PlaylistItem *item)
{
    if(!m_search.isEmpty())
	item->setVisible(m_search.checkItem(item));
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void Playlist::setup()
{
    setItemMargin(3);

    connect(header(), SIGNAL(indexChange(int, int, int)), this, SLOT(slotColumnOrderChanged(int, int, int)));
    connect(this, SIGNAL(signalDataChanged()), this, SIGNAL(signalChanged()));
    connect(this, SIGNAL(signalCountChanged(Playlist *)), this, SIGNAL(signalChanged()));

    setSorting(1);
}

void Playlist::loadFile(const QString &fileName, const QFileInfo &fileInfo)
{
    QFile file(fileName);
    if(!file.open(IO_ReadOnly))
	return;

    QTextStream stream(&file);

    // Turn off non-explicit sorting.

    setSorting(columns() + 1);

    PlaylistItem *after = 0;

    m_disableColumnWidthUpdates = true;

    while(!stream.atEnd()) {
	QString itemName = stream.readLine().stripWhiteSpace();

	QFileInfo item(itemName);

	if(item.isRelative())
	    item.setFile(QDir::cleanDirPath(fileInfo.dirPath(true) + "/" + itemName));

	if(item.exists() && item.isFile() && item.isReadable() &&
	   MediaFiles::isMediaFile(item.fileName()))
	{
	    if(after)
		after = createItem(item, QString::null, after, false);
	    else
		after = createItem(item, QString::null, 0, false);
	}
    }

    file.close();

    emit signalCountChanged(this);

    m_disableColumnWidthUpdates = false;
}

void Playlist::setPlaying(PlaylistItem *item, bool p)
{
    if(p) {
	m_playingItem = item;
	item->setPixmap(m_leftColumn, UserIcon("playing"));
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

int Playlist::leftMostVisibleColumn() const
{
    int i = 0;
    while(!isColumnVisible(header()->mapToSection(i)) && i < PlaylistItem::lastColumn())
	i++;

    return header()->mapToSection(i);
}

PlaylistItemList Playlist::items(QListViewItemIterator::IteratorFlag flags)
{
    PlaylistItemList list;

    for(QListViewItemIterator it(this, flags); it.current(); ++it)
	list.append(static_cast<PlaylistItem *>(it.current()));

    return list;
}

void Playlist::calculateColumnWeights()
{
    if(m_disableColumnWidthUpdates)
	return;

    PlaylistItemList l = items();
    QValueListConstIterator<int> columnIt;

    QValueVector<double> averageWidth(columns(), 0);
    double itemCount = l.size();

    QValueVector<int> cachedWidth;

    // Here we're not using a real average, but averaging the squares of the
    // column widths and then using the square root of that value.  This gives
    // a nice weighting to the longer columns without doing something arbitrary
    // like adding a fixed amount of padding.

    for(PlaylistItemList::ConstIterator it = l.begin(); it != l.end(); ++it) {
	cachedWidth = (*it)->cachedWidths();
	for(columnIt = m_weightDirty.begin(); columnIt != m_weightDirty.end(); ++columnIt)
	    averageWidth[*columnIt] += pow(double(cachedWidth[*columnIt]) , 2.0) / itemCount;
    }

    m_columnWeights.resize(columns(), -1);

    for(columnIt = m_weightDirty.begin(); columnIt != m_weightDirty.end(); ++columnIt) {
	m_columnWeights[*columnIt] = int(sqrt(averageWidth[*columnIt]) + 0.5);

	//  kdDebug(65432) << k_funcinfo << "m_columnWeights[" << *columnIt << "] == "
	//                 << m_columnWeights[*columnIt] << endl;
    }

    m_weightDirty.clear();
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::slotUpdateColumnWidths()
{
    if(m_disableColumnWidthUpdates)
	return;

    // Make sure that the column weights have been initialized before trying to
    // update the columns.

    QValueList<int> visibleColumns;
    for(int i = 0; i < columns(); i++) {
	if(isColumnVisible(i))
	    visibleColumns.append(i);
    }

    QValueListConstIterator<int> it;

    if(count() == 0) {
	for(it = visibleColumns.begin(); it != visibleColumns.end(); ++it)
	    setColumnWidth(*it, header()->fontMetrics().width(header()->label(*it)) + 10);

	return;
    }

    if(m_columnWeights.isEmpty())
	return;

    // First build a list of minimum widths based on the strings in the listview
    // header.  We won't let the width of the column go below this width.

    QValueVector<int> minimumWidth(columns(), 0);
    int minimumWidthTotal = 0;

    // Also build a list of either the minimum *or* the fixed width -- whichever is
    // greater.

    QValueVector<int> minimumFixedWidth(columns(), 0);
    int minimumFixedWidthTotal = 0;

    for(it = visibleColumns.begin(); it != visibleColumns.end(); ++it) {
	int column = *it;
	minimumWidth[column] = header()->fontMetrics().width(header()->label(column)) + 10;
	minimumWidthTotal += minimumWidth[column];

	minimumFixedWidth[column] = QMAX(minimumWidth[column], m_columnFixedWidths[column]);
	minimumFixedWidthTotal += minimumFixedWidth[column];
    }

    // Make sure that the width won't get any smaller than this.  We have to
    // account for the scrollbar as well.  Since this method is called from the
    // resize event this will set a pretty hard lower bound on the size.

    setMinimumWidth(minimumWidthTotal + verticalScrollBar()->width());

    // If we've got enough room for the fixed widths (larger than the minimum
    // widths) then instead use those for our "minimum widths".

    if(minimumFixedWidthTotal < visibleWidth()) {
	minimumWidth = minimumFixedWidth;
	minimumWidthTotal = minimumFixedWidthTotal;
    }

    // We've got a list of columns "weights" based on some statistics gathered
    // about the widths of the items in that column.  We need to find the total
    // useful weight to use as a divisor for each column's weight.

    double totalWeight = 0;
    for(it = visibleColumns.begin(); it != visibleColumns.end(); ++it)
	totalWeight += m_columnWeights[*it];

    // Computed a "weighted width" for each visible column.  This would be the
    // width if we didn't have to handle the cases of minimum and maximum widths.

    QValueVector<int> weightedWidth(columns(), 0);
    for(it = visibleColumns.begin(); it != visibleColumns.end(); ++it)
	weightedWidth[*it] = int(double(m_columnWeights[*it]) / totalWeight * visibleWidth() + 0.5);

    // The "extra" width for each column.  This is the weighted width less the
    // minimum width or zero if the minimum width is greater than the weighted
    // width.

    QValueVector<int> extraWidth(columns(), 0);

    // This is used as an indicator if we have any columns where the weighted
    // width is less than the minimum width.  If this is false then we can
    // just use the weighted width with no problems, otherwise we have to
    // "readjust" the widths.

    bool readjust = false;

    // If we have columns where the weighted width is less than the minimum width
    // we need to steal that space from somewhere.  The amount that we need to
    // steal is the "neededWidth".

    int neededWidth = 0;

    // While we're on the topic of stealing -- we have to have somewhere to steal
    // from.  availableWidth is the sum of the amount of space beyond the minimum
    // width that each column has been allocated -- the sum of the values of
    // extraWidth[].

    int availableWidth = 0;

    // Fill in the values discussed above.

    for(it = visibleColumns.begin(); it != visibleColumns.end(); ++it) {
	if(weightedWidth[*it] < minimumWidth[*it]) {
	    readjust = true;
	    extraWidth[*it] = 0;
	    neededWidth += minimumWidth[*it] - weightedWidth[*it];
	}
	else {
	    extraWidth[*it] = weightedWidth[*it] - minimumWidth[*it];
	    availableWidth += extraWidth[*it];
	}
    }

    // The adjustmentRatio is the amount of the "extraWidth[]" that columns will
    // actually be given.

    double adjustmentRatio = (double(availableWidth) - double(neededWidth)) / double(availableWidth);

    // This will be the sum of the total space that we actually use.  Because of
    // rounding error this won't be the exact available width.

    int usedWidth = 0;

    // Now set the actual column widths.  If the weighted widths are all greater
    // than the minimum widths, just use those, otherwise use the "reajusted
    // weighted width".

    for(it = visibleColumns.begin(); it != visibleColumns.end(); ++it) {
	int width;
	if(readjust) {
	    int adjustedExtraWidth = int(double(extraWidth[*it]) * adjustmentRatio + 0.5);
	    width = minimumWidth[*it] + adjustedExtraWidth;
	}
	else
	    width = weightedWidth[*it];

	setColumnWidth(*it, width);
	usedWidth += width;
    }

    // Fill the remaining gap for a clean fit into the available space.

    int remainingWidth = visibleWidth() - usedWidth;
    setColumnWidth(visibleColumns.back(), columnWidth(visibleColumns.back()) + remainingWidth);

    m_widthsDirty = false;
}

void Playlist::slotShowRMBMenu(QListViewItem *item, const QPoint &point, int column)
{
    if(!item)
	return;

    // Create the RMB menu on demand.

    if(!m_rmbMenu) {

	// A bit of a hack to get a pointer to the action collection.
	// Probably more of these actions should be ported over to using KActions.

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
	ActionCollection::action("guessTag")->plug(m_rmbMenu);

    if(!readOnly())
	ActionCollection::action("renameFile")->plug(m_rmbMenu);

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
		i18n("Edit '%1'").arg(columnText(column)));

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
    case PlaylistItem::ArtistColumn:
	edit->completionObject()->setItems(list->uniqueSet(CollectionList::Artists));
	break;
    case PlaylistItem::AlbumColumn:
	edit->completionObject()->setItems(list->uniqueSet(CollectionList::Albums));
	break;
    case PlaylistItem::GenreColumn:
    {
	QStringList genreList;
	TagLib::StringList genres = TagLib::ID3v1::genreList();
	for(TagLib::StringList::ConstIterator it = genres.begin(); it != genres.end(); ++it)
	    genreList.append(TStringToQString((*it)));
	edit->completionObject()->setItems(genreList);
	break;
    }
    default:
	edit->completionObject()->clear();
	break;
    }

    m_editText = currentItem()->text(m_currentColumn);

    rename(currentItem(), m_currentColumn);
}

void Playlist::editTag(PlaylistItem *item, const QString &text, int column)
{
    switch(column - columnOffset())
    {
    case PlaylistItem::TrackColumn:
	item->file().tag()->setTitle(text);
	break;
    case PlaylistItem::ArtistColumn:
	item->file().tag()->setArtist(text);
	break;
    case PlaylistItem::AlbumColumn:
	item->file().tag()->setAlbum(text);
	break;
    case PlaylistItem::TrackNumberColumn:
    {
	bool ok;
	int value = text.toInt(&ok);
	if(ok)
	    item->file().tag()->setTrack(value);
	break;
    }
    case PlaylistItem::GenreColumn:
	item->file().tag()->setGenre(text);
	break;
    case PlaylistItem::YearColumn:
    {
	bool ok;
	int value = text.toInt(&ok);
	if(ok)
	    item->file().tag()->setYear(value);
	break;
    }
    }

    item->file().tag()->save();
    item->slotRefresh();
}

void Playlist::slotInlineEditDone(QListViewItem *, const QString &, int column)
{
    QString text = renameLineEdit()->text();

    PlaylistItemList l = selectedItems();
    if(text == m_editText ||
       (l.count() > 1 && KMessageBox::warningYesNo(
	   0,
	   i18n("This will edit multiple files. Are you sure?"),
	   QString::null,
	   KStdGuiItem::yes(),
	   KStdGuiItem::no(),
	   "DontWarnMultipleTags") == KMessageBox::No))
    {
	return;
    }

    for(PlaylistItemList::ConstIterator it = l.begin(); it != l.end(); ++it) {
	editTag(*it, text, column);
	kapp->processEvents();
    }
    slotEmitSelected();
}

void Playlist::slotColumnOrderChanged(int, int from, int to)
{
    if(from == 0 || to == 0) {
	if(m_playingItem) {
	    m_playingItem->setPixmap(m_leftColumn, QPixmap(0, 0));
	    m_playingItem->setPixmap(header()->mapToSection(0), UserIcon("playing"));
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

void Playlist::slotColumnSizeChanged(int column, int, int newSize)
{
    m_widthsDirty = true;
    m_columnFixedWidths[column] = newSize;
}

void Playlist::slotInlineCompletionModeChanged(KGlobalSettings::Completion mode)
{
    SharedSettings::instance()->setInlineCompletionMode(mode);
}

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, Playlist &p)
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

    p.setColumnWidthUpdatesDisabled(true);

    for(QStringList::Iterator it = files.begin(); it != files.end(); ++it ) {
	QFileInfo info(*it);
	after = p.createItem(info, *it, after, false);
    }

    p.emitCountChanged();
    p.setColumnWidthUpdatesDisabled(false);

    return s;
}

#include "playlist.moc"
