/***************************************************************************
    begin                : Sat Feb 16 2002
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

#include <kconfig.h>
#include <kmessagebox.h>
#include <kurldrag.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <klocale.h>
#include <kdebug.h>
#include <kinputdialog.h>
#include <kfiledialog.h>
#include <kglobalsettings.h>
#include <kurl.h>
#include <kio/netaccess.h>
#include <kio/job.h>

#include <qheader.h>
#include <qcursor.h>
#include <qdir.h>
#include <qeventloop.h>
#include <qtooltip.h>
#include <qwidgetstack.h>
#include <qfile.h>

#include <id3v1genres.h>

#include <time.h>
#include <math.h>
#include <dirent.h>

#include "playlist.h"
#include "playlistitem.h"
#include "playlistcollection.h"
#include "playlistsearch.h"
#include "mediafiles.h"
#include "collectionlist.h"
#include "filerenamer.h"
#include "actioncollection.h"
#include "tracksequencemanager.h"
#include "juk.h"
#include "tag.h"
#include "k3bexporter.h"
#include "painteater.h"
#include "upcomingplaylist.h"
#include "deletedialog.h"
#include "googlefetcher.h"
#include "coverinfo.h"
#include "tagtransactionmanager.h"

using namespace ActionCollection;

/**
 * Just a shortcut of sorts.
 */

static bool manualResize()
{
    return action<KToggleAction>("resizeColumnsManually")->isChecked();
}

/**
 * A tooltip specialized to show full filenames over the file name column.
 */

class PlaylistToolTip : public QToolTip
{
public:
    PlaylistToolTip(QWidget *parent, Playlist *playlist) :
	QToolTip(parent), m_playlist(playlist) {}

    virtual void maybeTip(const QPoint &p)
    {
	PlaylistItem *item = static_cast<PlaylistItem *>(m_playlist->itemAt(p));

	if(!item)
	    return;

	QPoint contentsPosition = m_playlist->viewportToContents(p);

	int column = m_playlist->header()->sectionAt(contentsPosition.x());

	if(column == m_playlist->columnOffset() + PlaylistItem::FileNameColumn ||
	   item->cachedWidths()[column] > m_playlist->columnWidth(column) ||
	   (column == m_playlist->columnOffset() + PlaylistItem::CoverColumn &&
	    item->file().coverInfo()->hasCover()))
	{
	    QRect r = m_playlist->itemRect(item);
	    int headerPosition = m_playlist->header()->sectionPos(column);
	    r.setLeft(headerPosition);
	    r.setRight(headerPosition + m_playlist->header()->sectionSize(column));

	    if(column == m_playlist->columnOffset() + PlaylistItem::FileNameColumn)
		tip(r, item->file().absFilePath());
	    else if(column == m_playlist->columnOffset() + PlaylistItem::CoverColumn) {
		QMimeSourceFactory *f = QMimeSourceFactory::defaultFactory();
	        f->setImage("coverThumb",
			    QImage(item->file().coverInfo()->pixmap(CoverInfo::Thumbnail).convertToImage()));
	        tip(r, "<center><img source=\"coverThumb\"/></center>");
	    }
	    else
		tip(r, item->text(column));
	}
    }

private:
    Playlist *m_playlist;
};

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
    void sync() { writeConfig(); }

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
    l->slotColumnResizeModeChanged();
}

////////////////////////////////////////////////////////////////////////////////
// Playlist::ShareSettings protected members
////////////////////////////////////////////////////////////////////////////////

Playlist::SharedSettings::SharedSettings()
{
    KConfigGroup config(KGlobal::config(), "PlaylistShared");

    bool resizeColumnsManually = config.readBoolEntry("ResizeColumnsManually", false);
    action<KToggleAction>("resizeColumnsManually")->setChecked(resizeColumnsManually);

    // save column order
    m_columnOrder = config.readIntListEntry("ColumnOrder");

    QValueList<int> l = config.readIntListEntry("VisibleColumns");

    if(l.isEmpty()) {

	// Provide some default values for column visibility if none were
	// read from the configuration file.

	for(int i = 0; i <= PlaylistItem::lastColumn(); i++) {
	    switch(i) {
	    case PlaylistItem::BitrateColumn:
	    case PlaylistItem::CommentColumn:
	    case PlaylistItem::FileNameColumn:
	    case PlaylistItem::FullPathColumn:
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
	config.readNumEntry("InlineCompletionMode", KGlobalSettings::CompletionAuto));
}

////////////////////////////////////////////////////////////////////////////////
// Playlist::SharedSettings private members
////////////////////////////////////////////////////////////////////////////////

void Playlist::SharedSettings::writeConfig()
{
    KConfigGroup config(KGlobal::config(), "PlaylistShared");
    config.writeEntry("ColumnOrder", m_columnOrder);

    QValueList<int> l;
    for(uint i = 0; i < m_columnsVisible.size(); i++)
	l.append(int(m_columnsVisible[i]));

    config.writeEntry("VisibleColumns", l);
    config.writeEntry("InlineCompletionMode", m_inlineCompletion);

    config.writeEntry("ResizeColumnsManually", manualResize());

    KGlobal::config()->sync();
}

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

PlaylistItemList Playlist::m_history;
PlaylistItem *Playlist::m_playingItem = 0;
UpcomingPlaylist *Playlist::m_upcomingPlaylist = 0;
QMap<int, PlaylistItem *> Playlist::m_backMenuItems;
int Playlist::m_leftColumn = 0;

Playlist::Playlist(PlaylistCollection *collection, const QString &name,
		   const QString &iconName) :
    KListView(collection->playlistStack(), name.latin1()),
    m_collection(collection),
    m_selectedCount(0),
    m_allowDuplicates(false),
    m_polished(false),
    m_applySharedSettings(true),
    m_mousePressed(false),
    m_disableColumnWidthUpdates(true),
    m_widthsDirty(true),
    m_searchEnabled(true),
    m_lastSelected(0),
    m_playlistName(name),
    m_rmbMenu(0),
    m_toolTip(0)
{
    setup();
    collection->setupPlaylist(this, iconName);
}

Playlist::Playlist(PlaylistCollection *collection, const PlaylistItemList &items,
		   const QString &name, const QString &iconName) :
    KListView(collection->playlistStack(), name.latin1()),
    m_collection(collection),
    m_selectedCount(0),
    m_allowDuplicates(false),
    m_polished(false),
    m_applySharedSettings(true),
    m_mousePressed(false),
    m_disableColumnWidthUpdates(true),
    m_widthsDirty(true),
    m_searchEnabled(true),
    m_lastSelected(0),
    m_playlistName(name),
    m_rmbMenu(0),
    m_toolTip(0)
{
    setup();
    collection->setupPlaylist(this, iconName);
    createItems(items);
}

Playlist::Playlist(PlaylistCollection *collection, const QFileInfo &playlistFile,
		   const QString &iconName) :
    KListView(collection->playlistStack()),
    m_collection(collection),
    m_selectedCount(0),
    m_allowDuplicates(false),
    m_polished(false),
    m_applySharedSettings(true),
    m_mousePressed(false),
    m_disableColumnWidthUpdates(true),
    m_widthsDirty(true),
    m_searchEnabled(true),
    m_lastSelected(0),
    m_fileName(playlistFile.absFilePath()),
    m_rmbMenu(0),
    m_toolTip(0)
{
    setup();
    loadFile(m_fileName, playlistFile);
    collection->setupPlaylist(this, iconName);
}

Playlist::Playlist(PlaylistCollection *collection, bool delaySetup) :
    KListView(collection->playlistStack()),
    m_collection(collection),
    m_selectedCount(0),
    m_allowDuplicates(false),
    m_polished(false),
    m_applySharedSettings(true),
    m_mousePressed(false),
    m_disableColumnWidthUpdates(true),
    m_widthsDirty(true),
    m_searchEnabled(true),
    m_lastSelected(0),
    m_rmbMenu(0),
    m_toolTip(0)
{
    setup();

    if(!delaySetup)
	collection->setupPlaylist(this, "midi");
}

Playlist::~Playlist()
{
    // clearItem() will take care of removing the items from the history,
    // so call clearItems() to make sure it happens.

    clearItems(items());
    if(m_playingItem && m_playingItem->playlist() == this)
	m_playingItem = 0;

    delete m_toolTip;

    // Select a different playlist if we're the selected one

    if(isVisible() && this != CollectionList::instance())
	m_collection->raise(CollectionList::instance());
}

QString Playlist::name() const
{
    if(m_playlistName.isNull())
	return m_fileName.section(QDir::separator(), -1).section('.', 0, -2);
    else
	return m_playlistName;
}

FileHandle Playlist::currentFile() const
{
    return m_playingItem ? m_playingItem->file() : FileHandle::null();
}

int Playlist::time() const
{
    int time = 0;
    QListViewItemIterator it(const_cast<Playlist *>(this));
    while (it.current()) {
	PlaylistItem *item = static_cast<PlaylistItem *>(it.current());
	time += item->file().tag()->seconds();
	it++;
    }
    return time;
}

void Playlist::playFirst()
{
    TrackSequenceManager::instance()->setNextItem(static_cast<PlaylistItem *>(
	QListViewItemIterator(const_cast<Playlist *>(this), QListViewItemIterator::Visible).current()));
    action("forward")->activate();
}

void Playlist::playNext()
{
    TrackSequenceManager::instance()->setCurrentPlaylist(this);
    setPlaying(TrackSequenceManager::instance()->nextItem());
}

void Playlist::stop()
{
    m_history.clear();
    setPlaying(0);
}

void Playlist::playPrevious()
{
    if(!m_playingItem)
	return;

    bool random = action("randomPlay") && action<KToggleAction>("randomPlay")->isChecked();

    PlaylistItem *previous = 0;

    if(random && !m_history.isEmpty()) {
        PlaylistItemList::Iterator last = m_history.fromLast();
        previous = *last;
        m_history.remove(last);
    }
    else {
	m_history.clear();
	previous = TrackSequenceManager::instance()->previousItem();
    }

    if(!previous)
	previous = static_cast<PlaylistItem *>(m_playingItem->itemAbove());

    setPlaying(previous, false);
}

void Playlist::setName(const QString &n)
{
    m_collection->addName(n);
    m_collection->removeName(m_playlistName);

    m_playlistName = n;
    emit signalNameChanged(m_playlistName);
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
    m_search.clearItem(item);

    if(!m_randomList.isEmpty() && !m_visibleChanged)
        m_randomList.remove(item);
    m_history.remove(item);

    delete item;
    if(emitChanged)
	dataChanged();
}

void Playlist::clearItems(const PlaylistItemList &items)
{
    for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it)
	clearItem(*it, false);

    dataChanged();
}

QStringList Playlist::files() const
{
    QStringList list;

    for(QListViewItemIterator it(const_cast<Playlist *>(this)); it.current(); ++it)
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

PlaylistItem *Playlist::firstChild() const
{
    return static_cast<PlaylistItem *>(KListView::firstChild());
}

void Playlist::updateLeftColumn()
{
    int newLeftColumn = leftMostVisibleColumn();

    if(m_leftColumn != newLeftColumn) {
	if(m_playingItem)
            m_playingItem->listView()->triggerUpdate();

	m_leftColumn = newLeftColumn;
    }
}

void Playlist::setItemsVisible(const PlaylistItemList &items, bool visible) // static
{
    m_visibleChanged = true;
    for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it)
	(*it)->setVisible(visible);
}

void Playlist::setSearch(const PlaylistSearch &s)
{
    m_search = s;

    if(!m_searchEnabled)
	return;

    // Make sure that we only play visible items.

    m_randomList.clear();

    setItemsVisible(s.matchedItems(), true);
    setItemsVisible(s.unmatchedItems(), false);
}

void Playlist::setSearchEnabled(bool enabled)
{
    if(m_searchEnabled == enabled)
	return;

    m_searchEnabled = enabled;

    if(enabled) {
	setItemsVisible(m_search.matchedItems(), true);
	setItemsVisible(m_search.unmatchedItems(), false);
    }
    else
	setItemsVisible(items(), true);
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
    for(PlaylistItemList::Iterator it = l.begin(); it != l.end(); ++it) {
	(*it)->refreshFromDisk();

	if(!(*it)->file().tag()) {
	    kdDebug(65432) << "Error while trying to refresh the tag.  "
			   << "This file has probably been removed."
			   << endl;
	    delete (*it)->collectionItem();
	}

	processEvents();
    }
    KApplication::restoreOverrideCursor();
}

void Playlist::slotRenameFile()
{
    FileRenamer renamer;
    PlaylistItemList items = selectedItems();

    if(items.isEmpty())
        return;

    emit signalEnableDirWatch(false);

    renamer.rename(items);
    dataChanged();

    emit signalEnableDirWatch(true);
}

void Playlist::slotViewCover()
{
    PlaylistItemList items = selectedItems();
    if (items.isEmpty())
        return;
    for(PlaylistItemList::Iterator it = items.begin(); it != items.end(); ++it)
        (*it)->file().coverInfo()->popupLargeCover();
}

void Playlist::slotRemoveCover()
{
    PlaylistItemList items = selectedItems();
    if(items.isEmpty())
        return;
    int button = KMessageBox::warningContinueCancel(this,
						    i18n("Are you sure you want to delete these covers?"),
						    QString::null,
						    i18n("&Delete Covers"));
    if(button == KMessageBox::Continue)
	refreshAlbums(items);
}

void Playlist::slotAddCover(bool retrieveLocal)
{
    PlaylistItemList items = selectedItems();

    if(items.isEmpty())
        return;

    QImage image;

    if(retrieveLocal) {
        KURL file = KFileDialog::getImageOpenURL(
	    ":homedir", this, i18n("Select cover image file - JuK"));
        image = QImage(file.directory() + "/" + file.fileName());
    }
    else {
        PlaylistItemList::Iterator it=items.begin();
        image = GoogleFetcher((*it)->file()).pixmap().convertToImage();;
    }

    if(image.isNull())
        return;

    refreshAlbums(items, image);
}

void Playlist::slotGuessTagInfo(TagGuesser::Type type)
{
    KApplication::setOverrideCursor(Qt::waitCursor);
    PlaylistItemList items = selectedItems();
    setCanDeletePlaylist(false);
    for(PlaylistItemList::Iterator it = items.begin(); it != items.end(); ++it) {
        (*it)->guessTagInfo(type);

	processEvents();
    }

    // MusicBrainz queries automatically commit at this point.  What would
    // be nice is having a signal emitted when the last query is completed.

    if(type == TagGuesser::FileName)
	TagTransactionManager::instance()->commit();

    dataChanged();
    setCanDeletePlaylist(true);
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

void Playlist::slotShowPlaying()
{
    if(!m_playingItem)
	return;

    Playlist *l = m_playingItem->playlist();

    l->clearSelection();
    l->setSelected(m_playingItem, true);
    l->ensureItemVisible(m_playingItem);
    m_collection->raise(l);
}

void Playlist::slotColumnResizeModeChanged()
{
    if(manualResize())
	setHScrollBarMode(Auto);
    else
	setHScrollBarMode(AlwaysOff);

    if(!manualResize())
	slotUpdateColumnWidths();

    SharedSettings::instance()->sync();
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

	DeleteDialog dialog(this);
	if(dialog.confirmDeleteList(files)) {
	    bool shouldDelete = dialog.shouldDelete();
	    QStringList errorFiles;

	    for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it) {
		if(m_playingItem == *it)
		    action("forward")->activate();

		QString removePath = (*it)->file().absFilePath();
		if((!shouldDelete && KIO::NetAccess::synchronousRun(KIO::trash(removePath), this)) ||
		   (shouldDelete && QFile::remove(removePath))) {
                    if(!m_randomList.isEmpty() && !m_visibleChanged)
                        m_randomList.remove(*it);
		    CollectionList::instance()->clearItem((*it)->collectionItem());

		    // Get Orwellian and erase the song from history. :-)

		    m_history.remove(*it);
		}
		else
		    errorFiles.append((*it)->file().absFilePath());
	    }

	    if(!errorFiles.isEmpty()) {
		QString errorMsg = shouldDelete ? 
			i18n("Could not delete these files") :
			i18n("Could not move these files to the Trash");
		KMessageBox::errorList(this, errorMsg, errorFiles);
	    }
	}
	dataChanged();
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

void Playlist::decode(QMimeSource *s, PlaylistItem *item)
{
    KURL::List urls;

    if(!KURLDrag::decode(s, urls) || urls.isEmpty())
	return;

    // handle dropped images

    if(!MediaFiles::isMediaFile(urls.front().path())) {

	QString file;

	if(urls.front().isLocalFile())
	    file = urls.front().path();
	else
	    KIO::NetAccess::download(urls.front(), file, 0);

	KMimeType::Ptr mimeType = KMimeType::findByPath(file);

	if(item && mimeType->name().startsWith("image/")) {
	    item->file().coverInfo()->setCover(QImage(file));
	    refreshAlbum(item->file().tag()->artist(),
			 item->file().tag()->album());
	}
    }

    QStringList fileList;

    for(KURL::List::Iterator it = urls.begin(); it != urls.end(); ++it)
	fileList.append((*it).path());

    addFiles(fileList, m_collection->importPlaylists(), item);
}

bool Playlist::eventFilter(QObject *watched, QEvent *e)
{
    if(watched == header()) {
	switch(e->type()) {
	case QEvent::MouseButtonPress:
	{
	    switch(static_cast<QMouseEvent *>(e)->button()) {
	    case RightButton:
		m_headerMenu->popup(QCursor::pos());
		break;
	    case LeftButton:
		m_mousePressed = true;
		break;
	    default:
		break;
	    }
	    break;
	}
	case QEvent::MouseButtonRelease:
	{
	    if(static_cast<QMouseEvent *>(e)->button() == LeftButton)
		m_mousePressed = false;

	    if(!manualResize() && m_widthsDirty)
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
    PlaylistItem *item = static_cast<PlaylistItem *>(itemAt(vp));

    // When dropping on the upper half of an item, insert before this item.
    // This is what the user expects, and also allows the insertion at
    // top of the list

    if(!item)
	item = static_cast<PlaylistItem *>(lastItem());
    else if(vp.y() < item->itemPos() + item->height() / 2)
	item = static_cast<PlaylistItem *>(item->itemAbove());

    if(e->source() == this) {

	// Since we're trying to arrange things manually, turn off sorting.

	setSorting(columns() + 1);

	QPtrList<QListViewItem> items = KListView::selectedItems();

	for(QPtrListIterator<QListViewItem> it(items); it.current(); ++it) {
	    if(!item) {

		// Insert the item at the top of the list.  This is a bit ugly,
		// but I don't see another way.

		takeItem(it.current());
		insertItem(it.current());
	    }
	    else
		it.current()->moveItem(item);

	    item = static_cast<PlaylistItem *>(it.current());
	}
    }
    else
	decode(e, item);

    dataChanged();
    emit signalPlaylistItemsDropped(this);
    KListView::contentsDropEvent(e);
}

void Playlist::contentsMouseDoubleClickEvent(QMouseEvent *e)
{
    if(e->button() == LeftButton)
	KListView::contentsMouseDoubleClickEvent(e);
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

void Playlist::read(QDataStream &s)
{
    QString buffer;

    s >> m_playlistName
      >> m_fileName;

    QStringList files;
    s >> files;

    QListViewItem *after = 0;
    for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it)
        after = createItem(FileHandle(*it), after, false);

    dataChanged();
    m_collection->setupPlaylist(this, "midi");
}

void Playlist::viewportPaintEvent(QPaintEvent *pe)
{
    // If there are columns that need to be updated, well, update them.

    if(!m_weightDirty.isEmpty() && !manualResize())
    {
	calculateColumnWeights();
	slotUpdateColumnWidths();
    }

    KListView::viewportPaintEvent(pe);
}

void Playlist::viewportResizeEvent(QResizeEvent *re)
{
    // If the width of the view has changed, manually update the column
    // widths.

    if(re->size().width() != re->oldSize().width() && !manualResize())
	slotUpdateColumnWidths();

    KListView::viewportResizeEvent(re);
}

void Playlist::addColumn(const QString &label)
{
    slotWeightDirty(columns());
    KListView::addColumn(label, 30);
}

PlaylistItem *Playlist::createItem(const FileHandle &file,
				   QListViewItem *after, bool emitChanged)
{
    return createItem<PlaylistItem, CollectionListItem, CollectionList>(file, after, emitChanged);
}

void Playlist::createItems(const PlaylistItemList &siblings, PlaylistItem *after)
{
    createItems<CollectionListItem, PlaylistItem, PlaylistItem>(siblings, after);
}

void Playlist::addFiles(const QStringList &files, bool importPlaylists,
			PlaylistItem *after)
{
    if(!after)
	after = static_cast<PlaylistItem *>(lastItem());

    KApplication::setOverrideCursor(Qt::waitCursor);

    PaintEater pe(this);

    for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it)
        after = addFile(*it, importPlaylists, after);

    slotWeightDirty();
    dataChanged();

    KApplication::restoreOverrideCursor();
}

void Playlist::refreshAlbums(const PlaylistItemList &items, const QImage &image)
{
    QValueList< QPair<QString, QString> > albums;

    for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it) {
	if(albums.find(qMakePair((*it)->file().tag()->artist(),
				 (*it)->file().tag()->album())) == albums.end())
	{
	    albums.append(qMakePair((*it)->file().tag()->artist(),
				    (*it)->file().tag()->album()));				    
	    if(image.isNull())
		(*it)->file().coverInfo()->clearCover();
	    else
		(*it)->file().coverInfo()->setCover(image);
	}
	else
	    (*it)->file().coverInfo()->setCover();
    }

    for(QValueList< QPair<QString, QString> >::ConstIterator it = albums.begin();
	it != albums.end(); ++it)
    {
	refreshAlbum((*it).first, (*it).second);
    }
}

void Playlist::refreshAlbum(const QString &artist, const QString &album)
{
    ColumnList columns;
    columns.append(PlaylistItem::ArtistColumn);
    PlaylistSearch::Component artistComponent(artist, false, columns,
					      PlaylistSearch::Component::Exact);

    columns.clear();
    columns.append(PlaylistItem::AlbumColumn);
    PlaylistSearch::Component albumComponent(album, false, columns,
					     PlaylistSearch::Component::Exact);

    PlaylistSearch::ComponentList components;
    components.append(artist);
    components.append(album);

    PlaylistList playlists;
    playlists.append(CollectionList::instance());

    PlaylistSearch search(playlists, components);
    PlaylistItemList matches = search.matchedItems();

    for(PlaylistItemList::Iterator it = matches.begin(); it != matches.end(); ++it)
	(*it)->refresh();
}

void Playlist::hideColumn(int c, bool updateSearch)
{
    m_headerMenu->setItemChecked(c, false);

    if(!isColumnVisible(c))
	return;

    setColumnWidthMode(c, Manual);
    setColumnWidth(c, 0);

    // Moving the column to the end seems to prevent it from randomly
    // popping up.

    header()->moveSection(c, header()->count());
    header()->setResizeEnabled(false, c);

    if(c == m_leftColumn) {
	if(m_playingItem)
            m_playingItem->listView()->triggerUpdate();

	m_leftColumn = leftMostVisibleColumn();
    }

    if(!manualResize()) {
	slotUpdateColumnWidths();
	triggerUpdate();
    }

    if(this != CollectionList::instance())
	CollectionList::instance()->hideColumn(c, false);

    if(updateSearch)
	redisplaySearch();
}

void Playlist::showColumn(int c, bool updateSearch)
{
    m_headerMenu->setItemChecked(c, true);

    if(isColumnVisible(c))
	return;

    // Just set the width to one to mark the column as visible -- we'll update
    // the real size in the next call.

    setColumnWidth(c, 1);
    header()->setResizeEnabled(true, c);
    header()->moveSection(c, c); // Approximate old position

    if(c == leftMostVisibleColumn()) {
	if(m_playingItem)
            m_playingItem->listView()->triggerUpdate();
	m_leftColumn = leftMostVisibleColumn();
    }

    if(!manualResize()) {
	slotUpdateColumnWidths();
	triggerUpdate();
    }

    if(this != CollectionList::instance())
	CollectionList::instance()->showColumn(c, false);

    if(updateSearch)
	redisplaySearch();
}

bool Playlist::isColumnVisible(int c) const
{
    return columnWidth(c) != 0;
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
    addColumn(i18n("Cover"));
    addColumn(i18n("Track"));
    addColumn(i18n("Genre"));
    addColumn(i18n("Year"));
    addColumn(i18n("Length"));
    addColumn(i18n("Bitrate"));
    addColumn(i18n("Comment"));
    addColumn(i18n("File Name"));
    addColumn(i18n("File Name (full path)"));

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
	if(i == PlaylistItem::FileNameColumn)
	    m_headerMenu->insertSeparator();
	m_headerMenu->insertItem(header()->label(i), i);
	m_headerMenu->setItemChecked(i, true);
	adjustColumn(i);
    }

    connect(m_headerMenu, SIGNAL(activated(int)), this, SLOT(slotToggleColumnVisible(int)));

    connect(this, SIGNAL(contextMenuRequested(QListViewItem *, const QPoint &, int)),
	    this, SLOT(slotShowRMBMenu(QListViewItem *, const QPoint &, int)));
    connect(this, SIGNAL(itemRenamed(QListViewItem *, const QString &, int)),
	    this, SLOT(slotInlineEditDone(QListViewItem *, const QString &, int)));
    connect(this, SIGNAL(doubleClicked(QListViewItem *)),
	    this, SLOT(slotPlayCurrent()));
    connect(this, SIGNAL(returnPressed(QListViewItem *)),
	    this, SLOT(slotPlayCurrent()));

    connect(header(), SIGNAL(sizeChange(int, int, int)),
	    this, SLOT(slotColumnSizeChanged(int, int, int)));

    connect(renameLineEdit(), SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
	    this, SLOT(slotInlineCompletionModeChanged(KGlobalSettings::Completion)));

    connect(action("resizeColumnsManually"), SIGNAL(activated()),
	    this, SLOT(slotColumnResizeModeChanged()));

    if(action<KToggleAction>("resizeColumnsManually")->isChecked())
	setHScrollBarMode(Auto);
    else
	setHScrollBarMode(AlwaysOff);

    setAcceptDrops(true);
    setDropVisualizer(true);

    m_disableColumnWidthUpdates = false;

    setShowToolTips(false);
    m_toolTip = new PlaylistToolTip(viewport(), this);
}

void Playlist::setupItem(PlaylistItem *item)
{
    if(!m_search.isEmpty())
	item->setVisible(m_search.checkItem(item));

    if(childCount() <= 2 && !manualResize()) {
	slotWeightDirty();
	slotUpdateColumnWidths();
	triggerUpdate();
    }
}

void Playlist::setCanDeletePlaylist(bool canDelete)
{
    m_collection->setCanDeletePlaylist(canDelete);
}

////////////////////////////////////////////////////////////////////////////////
// protected slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::slotPopulateBackMenu() const
{
    if(!m_playingItem)
	return;

    KPopupMenu *menu = action<KToolBarPopupAction>("back")->popupMenu();
    menu->clear();
    m_backMenuItems.clear();

    int count = 0;
    PlaylistItemList::ConstIterator it = m_history.end();

    while(it != m_history.begin() && count < 10) {
	++count;
	--it;
	int index = menu->insertItem((*it)->file().tag()->title());
	m_backMenuItems[index] = *it;
    }
}

void Playlist::slotPlayFromBackMenu(int number) const
{
    if(!m_backMenuItems.contains(number))
	return;

    TrackSequenceManager::instance()->setNextItem(m_backMenuItems[number]);
    action("forward")->activate();
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void Playlist::setup()
{
    setItemMargin(3);

    connect(header(), SIGNAL(indexChange(int, int, int)), this, SLOT(slotColumnOrderChanged(int, int, int)));
    setSorting(1);
}

PlaylistItem *Playlist::nextItem(PlaylistItem *current) const
{
    if(current) {
	QListViewItemIterator it(current, QListViewItemIterator::Visible);
	++it;
	return static_cast<PlaylistItem *>(it.current());
    }
    else {
	QListViewItemIterator it(const_cast<Playlist *>(this),
				 QListViewItemIterator::Selected |
				 QListViewItemIterator::Visible);
	if(!it.current()) {
	    it = QListViewItemIterator(const_cast<Playlist *>(this),
				       QListViewItemIterator::Visible);
	}
	return static_cast<PlaylistItem *>(it.current());
    }
}

void Playlist::loadFile(const QString &fileName, const QFileInfo &fileInfo)
{
    QFile file(fileName);
    if(!file.open(IO_ReadOnly))
	return;

    QTextStream stream(&file);

    // Turn off non-explicit sorting.

    setSorting(PlaylistItem::lastColumn() + columnOffset() + 1);

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
		after = createItem(FileHandle(item, item.absFilePath()), after, false);
	    else
		after = createItem(FileHandle(item, item.absFilePath()), 0, false);
	}
    }

    file.close();

    dataChanged();

    m_disableColumnWidthUpdates = false;
}

void Playlist::setPlaying(PlaylistItem *item, bool addToHistory)
{
    if(m_playingItem == item)
	return;

    if(m_playingItem) {
	m_playingItem->setPlaying(false);
        m_playingItem->listView()->triggerUpdate();

	if(addToHistory) {
	    if(m_playingItem->playlist() == m_upcomingPlaylist)
		m_history.append(m_playingItem->collectionItem());
	    else
		m_history.append(m_playingItem);
	}

	m_playingItem = 0;
    }

    TrackSequenceManager::instance()->setCurrent(item);

    if(!item)
	return;

    m_playingItem = item;
    item->setPlaying(true);

    bool enableBack = !m_history.isEmpty();
    action<KToolBarPopupAction>("back")->popupMenu()->setEnabled(enableBack);
    item->listView()->triggerUpdate();
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
	    averageWidth[*columnIt] += pow(double(cachedWidth[*columnIt]), 2.0) / itemCount;
    }

    m_columnWeights.resize(columns(), -1);

    for(columnIt = m_weightDirty.begin(); columnIt != m_weightDirty.end(); ++columnIt) {
	m_columnWeights[*columnIt] = int(sqrt(averageWidth[*columnIt]) + 0.5);

	//  kdDebug(65432) << k_funcinfo << "m_columnWeights[" << *columnIt << "] == "
	//                 << m_columnWeights[*columnIt] << endl;
    }

    m_weightDirty.clear();
}

PlaylistItem *Playlist::addFile(const QString &file, bool importPlaylists,
				PlaylistItem *after)
{
    if(processEvents())
	m_collection->dataChanged();

    const QFileInfo fileInfo = QDir::cleanDirPath(file);

    if(!fileInfo.exists())
	return after;

    if(fileInfo.isFile() && fileInfo.isReadable()) {
	if(MediaFiles::isMediaFile(file))
	    return createItem(FileHandle(fileInfo, fileInfo.absFilePath()), after, false);

	if(importPlaylists && MediaFiles::isPlaylistFile(file) &&
	   !m_collection->containsPlaylistFile(fileInfo.absFilePath()))
	{
	    new Playlist(m_collection, fileInfo);
	    return after;
	}
    }

    if(fileInfo.isDir()) {

	// Resorting to the POSIX API because QDir::listEntries() stats every
	// file and blocks while it's doing so.

	DIR *dir = ::opendir(QFile::encodeName(fileInfo.filePath()));

	if(dir) {
	    struct dirent *dirEntry;

	    for(dirEntry = ::readdir(dir); dirEntry; dirEntry = ::readdir(dir)) {
		if(strcmp(dirEntry->d_name, ".") != 0 && strcmp(dirEntry->d_name, "..") != 0)
		    after = addFile(fileInfo.filePath() + QDir::separator() +
				    QFile::decodeName(dirEntry->d_name), importPlaylists, after);
	    }
	    ::closedir(dir);
	}
	else
	{
	    kdWarning(65432) << "Unable to open directory "
	                     << fileInfo.filePath()
			     << ", make sure it is readable.\n";
	}
    }

    return after;
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::slotUpdateColumnWidths()
{
    if(m_disableColumnWidthUpdates || manualResize())
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

void Playlist::slotAddToUpcoming()
{
    m_collection->setUpcomingPlaylistEnabled(true);
    m_upcomingPlaylist->appendItems(selectedItems());
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

	m_rmbUpcomingID = m_rmbMenu->insertItem(SmallIcon("today"),
	    i18n("Add to Play Queue"), this, SLOT(slotAddToUpcoming()));
	m_rmbMenu->insertSeparator();

	if(!readOnly()) {
	    action("edit_cut")->plug(m_rmbMenu);
	    action("edit_copy")->plug(m_rmbMenu);
	    action("edit_paste")->plug(m_rmbMenu);
	    action("edit_clear")->plug(m_rmbMenu);
	}
	else
	    action("edit_copy")->plug(m_rmbMenu);

	m_rmbEditID = m_rmbMenu->insertItem(
	    i18n("Edit"), this, SLOT(slotRenameTag()));

	action("refresh")->plug(m_rmbMenu);
	action("removeItem")->plug(m_rmbMenu);

	m_rmbMenu->insertSeparator();

	action("guessTag")->plug(m_rmbMenu);
	action("renameFile")->plug(m_rmbMenu);

	action("coverManager")->plug(m_rmbMenu);

	m_rmbMenu->insertSeparator();

	m_rmbMenu->insertItem(
	    SmallIcon("folder_new"), i18n("Create Playlist From Selected Items"), this, SLOT(slotCreateGroup()));

	K3bExporter *exporter = new K3bExporter(this);
	KAction *k3bAction = exporter->action();
	if(k3bAction)
	    k3bAction->plug(m_rmbMenu);
    }

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

    // Disable edit menu if only one file is selected, and it's read-only

    QFileInfo fi(static_cast<PlaylistItem*>(item)->file().absFilePath());
    bool enableEdit = true;

    if(!fi.isWritable() && selectedItems().count() == 1)
	enableEdit = false;

    m_rmbMenu->setItemEnabled(m_rmbEditID, enableEdit);

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

bool Playlist::editTag(PlaylistItem *item, const QString &text, int column)
{
    Tag *newTag = TagTransactionManager::duplicateTag(item->file().tag());

    switch(column - columnOffset())
    {
    case PlaylistItem::TrackColumn:
	newTag->setTitle(text);
	break;
    case PlaylistItem::ArtistColumn:
	newTag->setArtist(text);
	break;
    case PlaylistItem::AlbumColumn:
	newTag->setAlbum(text);
	break;
    case PlaylistItem::TrackNumberColumn:
    {
	bool ok;
	int value = text.toInt(&ok);
	if(ok)
	    newTag->setTrack(value);
	break;
    }
    case PlaylistItem::GenreColumn:
	newTag->setGenre(text);
	break;
    case PlaylistItem::YearColumn:
    {
	bool ok;
	int value = text.toInt(&ok);
	if(ok)
	    newTag->setYear(value);
	break;
    }
    }

    TagTransactionManager::instance()->changeTagOnItem(item, newTag);
    return true;
}

void Playlist::slotInlineEditDone(QListViewItem *, const QString &, int column)
{
    QString text = renameLineEdit()->text();
    bool changed = false;

    PlaylistItemList l = selectedItems();

    // See if any of the files have a tag different from the input.

    for(PlaylistItemList::ConstIterator it = l.begin(); it != l.end() && !changed; ++it)
	if((*it)->text(column - columnOffset()) != text)
	    changed = true;

    if(!changed ||
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

    for(PlaylistItemList::ConstIterator it = l.begin(); it != l.end(); ++it)
	editTag(*it, text, column);

    TagTransactionManager::instance()->commit();

    CollectionList::instance()->dataChanged();
    dataChanged();
    update();
}

void Playlist::slotColumnOrderChanged(int, int from, int to)
{
    if(from == 0 || to == 0) {
	if(m_playingItem)
            m_playingItem->listView()->triggerUpdate();

	m_leftColumn = header()->mapToSection(0);
    }

    SharedSettings::instance()->setColumnOrder(this);
}

void Playlist::slotToggleColumnVisible(int column)
{
    if(!isColumnVisible(column)) {
	int fileNameColumn = PlaylistItem::FileNameColumn + columnOffset();
	int fullPathColumn = PlaylistItem::FullPathColumn + columnOffset();

	if(column == fileNameColumn && isColumnVisible(fullPathColumn)) {
	    hideColumn(fullPathColumn, false);
	    SharedSettings::instance()->toggleColumnVisible(fullPathColumn);
	}
	if(column == fullPathColumn && isColumnVisible(fileNameColumn)) {
	    hideColumn(fileNameColumn, false);
	    SharedSettings::instance()->toggleColumnVisible(fileNameColumn);
	}
    }

    if(isColumnVisible(column))
	hideColumn(column);
    else
	showColumn(column);

    SharedSettings::instance()->toggleColumnVisible(column - columnOffset());
}

void Playlist::slotCreateGroup()
{
    QString name = m_collection->playlistNameDialog(i18n("Create New Playlist"));

    if(!name.isEmpty())
	new Playlist(m_collection, selectedItems(), name);
}

void Playlist::slotColumnSizeChanged(int column, int, int newSize)
{
    m_widthsDirty = true;
    m_columnFixedWidths[column] = newSize;

    if(m_mousePressed && !action<KToggleAction>("resizeColumnsManually")->isChecked()) {
	KMessageBox::information(this,
				 i18n("Manual column widths have been enabled.  You can "
				      "switch back to automatic column sizes in the view "
				      "menu."),
				 i18n("Manual Column Widths Enabled"),
				 "ShowManualColumnWidthInformation");
	
	action<KToggleAction>("resizeColumnsManually")->setChecked(true);
	slotColumnResizeModeChanged();
    }
}

void Playlist::slotInlineCompletionModeChanged(KGlobalSettings::Completion mode)
{
    SharedSettings::instance()->setInlineCompletionMode(mode);
}

void Playlist::slotPlayCurrent()
{
    QListViewItemIterator it(this, QListViewItemIterator::Selected);
    PlaylistItem *next = static_cast<PlaylistItem *>(it.current());
    TrackSequenceManager::instance()->setNextItem(next);
    action("forward")->activate();
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
    p.read(s);
    return s;
}

bool processEvents()
{
    static QTime time = QTime::currentTime();

    if(time.elapsed() > 200) {
	time.restart();
	kapp->processEvents();
	return true;
    }
    return false;
}

#include "playlist.moc"
