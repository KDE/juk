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
#include <kpopupmenu.h>
#include <kdebug.h>

#include <qfileinfo.h>
#include <qdir.h>
#include <qtimer.h>
#include <qapplication.h>
#include <qheader.h>
#include <qcursor.h>
#include <qclipboard.h>

#include <stdlib.h>
#include <limits.h>
#include <time.h>

#include "playlist.h"
#include "collectionlist.h"
#include "playlistbox.h"
#include "playlistsplitter.h"
#include "tag.h"
#include "genrelistlist.h"

////////////////////////////////////////////////////////////////////////////////
// Playlist::SharedSettings definition
////////////////////////////////////////////////////////////////////////////////

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

    for(int i = 0; i < l->columns(); ++i)
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

    int i = 0;
    for(QValueListConstIterator<int> it = m_columnOrder.begin(); it != m_columnOrder.end(); ++it)
	l->header()->moveSection(i++, *it);

    for(uint i = 0; i < m_columnsVisible.size(); i++) {
	if(m_columnsVisible[i] && ! l->isColumnVisible(i))
	    l->showColumn(i);
	else if(! m_columnsVisible[i] && l->isColumnVisible(i))
	    l->hideColumn(i);
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
	KConfigGroupSaver(config, "PlaylistShared");

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
	KConfigGroupSaver(config, "PlaylistShared");
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

Playlist::Playlist(QWidget *parent, const QString &name) : KListView(parent, name.latin1()),
							   m_playlistName(name), m_playingItem(0), m_leftColumn(0)

{
    setup();
}

Playlist::Playlist(const QFileInfo &playlistFile, QWidget *parent, const char *name) : KListView(parent, name),
										       m_fileName(playlistFile.absFilePath()),
										       m_playingItem(0), m_leftColumn(0)
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
    QStringList extensions = PlaylistSplitter::playlistExtensions();

    m_fileName = KFileDialog::getSaveFileName(name() + "." + PlaylistSplitter::playlistExtensions().first(),
						      PlaylistSplitter::extensionsString(extensions, i18n("Playlists")));
    m_fileName = m_fileName.stripWhiteSpace();

    if(m_fileName != QString::null) {
	if(extensions.find(m_fileName.section('.', -1)) == extensions.end())
	    m_fileName.append('.' + extensions.first());

	if(m_playlistName.isEmpty())
	    emit signalNameChanged(name());

	save();
    }
}

void Playlist::refresh()
{
    PlaylistItemList l = selectedItems();
    if(l.isEmpty())
	l = items();

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

void Playlist::clearItem(PlaylistItem *item, bool emitChanged)
{
    emit signalAboutToRemove(item);
    m_members.remove(item->absFilePath());
    item->deleteLater();
    if(emitChanged)
	emit signalNumberOfItemsChanged(this);
}

void Playlist::clearItems(const PlaylistItemList &items)
{
    for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it)
	clearItem(*it, false);

    emit signalNumberOfItemsChanged(this);
}

QStringList Playlist::files() const
{
    QStringList list;
    for(PlaylistItem *i = static_cast<PlaylistItem *>(firstChild()); i; i = static_cast<PlaylistItem *>(i->itemBelow()))
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

PlaylistItem *Playlist::nextItem(PlaylistItem *current, bool random)
{
    if(!current)
	return 0;

    PlaylistItem *i;

    if(random) {
	PlaylistItemList l = visibleItems();
	if(l.count() > 1) {
	    m_history.push(current);

	    srand(time(0));
	    i = current;
	    while(i == current)
		i = l[rand() % l.count()];
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

    if(random && !m_history.isEmpty())
        return m_history.pop();

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

void Playlist::setItemsVisible(const PlaylistItemList &items, bool visible)
{
    for(PlaylistItemList::ConstIterator it = items.begin(); it != items.end(); ++it)
	(*it)->setVisible(visible);
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
    decode(kapp->clipboard()->data());
}

void Playlist::clear()
{
    PlaylistItemList l = selectedItems();
    if(l.isEmpty())
	l = items();

    clearItems(l);
}

void Playlist::slotGuessTagInfo()
{
    KApplication::setOverrideCursor(Qt::waitCursor);
    PlaylistItemList items = selectedItems();
    for(PlaylistItemList::Iterator it = items.begin(); it != items.end(); ++it)
        (*it)->guessTagInfo();
    KApplication::restoreOverrideCursor();
}

void Playlist::slotReload()
{
    QFileInfo fileInfo(m_fileName);
    if(!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable())
	return;
    
    clear();
    loadFile(m_fileName, fileInfo);
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void Playlist::deleteFromDisk(const PlaylistItemList &items)
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
		    delete *it;
		}
		else
		    KMessageBox::sorry(this, i18n("Could not delete ") + (*it)->fileName() + ".");
	    }

	}
	emit signalNumberOfItemsChanged(this);
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
    drag->setPixmap(SmallIcon("sound"));

    return drag;
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

    emit signalFilesDropped(fileList, this);
}

bool Playlist::eventFilter(QObject* watched, QEvent* e)
{
    if(watched->inherits("QHeader")) { // Gotcha!

	if(e->type() == QEvent::MouseButtonPress) {

	    QMouseEvent *me = static_cast<QMouseEvent*>(e);

	    if(me->button() == Qt::RightButton) {
		m_headerMenu->popup(QCursor::pos());
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
    e->accept(KURLDrag::canDecode(e));
}

void Playlist::showEvent(QShowEvent *e)
{
    SharedSettings::instance()->apply(this);
    KListView::showEvent(e);
}

PlaylistItem *Playlist::createItem(const QFileInfo &file, QListViewItem *after)
{
    QString filePath = resolveSymLinks(file);

    CollectionListItem *item = CollectionList::instance()->lookup(filePath);

    if(!item) {
	item = new CollectionListItem(file, filePath);
	
	// If a valid tag was not created, destroy the CollectionListItem.
	if(!item->isValid()) {
	    kdError() << "Playlist::createItem() -- A valid tag was not created for \"" << file.filePath() << "\"" << endl;
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
	emit signalNumberOfItemsChanged(this);
	connect(item, SIGNAL(destroyed()), i, SLOT(deleteLater()));
	return i;
    }
    else
	return 0;
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

QString Playlist::resolveSymLinks(const QFileInfo &file)
{
    char real[PATH_MAX];
    if(file.exists() && realpath(QFile::encodeName(file.absFilePath()).data(), real))
	return QFile::decodeName(real);
    else
	return QString::null;
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
    addColumn(i18n("Comment"));
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

    m_columnVisibleAction = new KActionMenu(i18n("&Show Columns"), this, "showColumns");
    m_headerMenu = m_columnVisibleAction->popupMenu();
    m_headerMenu->insertTitle(i18n("Show"));
    m_headerMenu->setCheckable(true);

    for(int i =0; i < header()->count(); ++i) {

	m_headerMenu->insertItem(header()->label(i), i);

	m_headerMenu->setItemChecked(i, true);
    }

    connect(m_headerMenu, SIGNAL(activated(int)), this, SLOT(slotToggleColumnVisible(int)));

    //////////////////////////////////////////////////
    // hide some columns by default
    //////////////////////////////////////////////////

    hideColumn(PlaylistItem::CommentColumn);
    hideColumn(PlaylistItem::FileNameColumn);

    //////////////////////////////////////////////////
    // setup playlist RMB menu
    //////////////////////////////////////////////////

    m_rmbMenu = new KPopupMenu(this);

    m_rmbMenu->insertItem(SmallIcon("player_play"), i18n("Play Next"), this, SLOT(slotSetNext()));
    m_rmbMenu->insertSeparator();
    m_rmbMenu->insertItem(SmallIcon("editcut"), i18n("Cut"), this, SLOT(cut()));
    m_rmbMenu->insertItem(SmallIcon("editcopy"), i18n("Copy"), this, SLOT(copy()));
    m_rmbPasteID = m_rmbMenu->insertItem(SmallIcon("editpaste"), i18n("Paste"), this, SLOT(paste()));
    m_rmbMenu->insertItem(SmallIcon("editclear"), i18n("Clear"), this, SLOT(clear()));

    m_rmbMenu->insertSeparator();

    m_rmbMenu->insertItem(SmallIcon("editdelete"), i18n("Remove From Disk"), this, SLOT(slotDeleteSelectedItems()));

    m_rmbEditID = m_rmbMenu->insertItem(SmallIcon("edittool"), i18n("Edit"), this, SLOT(slotRenameTag()));

    connect(this, SIGNAL(selectionChanged()),
	    this, SLOT(slotEmitSelected()));
    connect(this, SIGNAL(doubleClicked(QListViewItem *)),
	    this, SLOT(slotEmitDoubleClicked(QListViewItem *)));
    connect(this, SIGNAL(contextMenuRequested( QListViewItem *, const QPoint&, int)),
	    this, SLOT(slotShowRMBMenu(QListViewItem *, const QPoint &, int)));
    connect(this, SIGNAL(itemRenamed(QListViewItem *, const QString &, int)),
	    this, SLOT(slotApplyModification(QListViewItem *, const QString &, int)));

    //////////////////////////////////////////////////

    addColumn(QString::null);
    setResizeMode(QListView::LastColumn);

    setAcceptDrops(true);
    m_allowDuplicates = false;

    connect(header(), SIGNAL(indexChange(int, int, int)), this, SLOT(slotColumnOrderChanged(int, int, int)));
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
	QString itemName = (stream.readLine()).stripWhiteSpace();

	QFileInfo item(itemName);

	if(item.isRelative())
	    item.setFile(QDir::cleanDirPath(fileInfo.dirPath(true) + "/" + itemName));

	if(item.exists() && item.isFile() && item.isReadable()) {
	    if(after)
		after = createItem(item, after);
	    else
		after = createItem(item);
	}
    }

    file.close();
}

void Playlist::setPlaying(PlaylistItem *item, bool playing)
{
    if(playing) {
	m_playingItem = item;
	item->setPixmap(m_leftColumn, QPixmap(UserIcon("playing")));
    }
    else {
	m_playingItem = 0;
	item->setPixmap(m_leftColumn, QPixmap(0, 0));
    }

    item->setPlaying(playing);
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::slotShowRMBMenu(QListViewItem *item, const QPoint &point, int column)
{
    if(!item)
	return;

    m_rmbMenu->setItemEnabled(m_rmbPasteID, canDecode(kapp->clipboard()->data()));

    bool showEdit =
	(column == PlaylistItem::TrackColumn) ||
	(column == PlaylistItem::ArtistColumn) ||
	(column == PlaylistItem::AlbumColumn) ||
	(column == PlaylistItem::TrackNumberColumn) ||
	(column == PlaylistItem::GenreColumn) ||
	(column == PlaylistItem::YearColumn);

    m_rmbMenu->setItemEnabled(m_rmbEditID, showEdit);

    m_rmbMenu->popup(point);
    m_currentColumn = column;
}

void Playlist::slotRenameTag()
{
    // setup completions and validators

    CollectionList *list = CollectionList::instance();

    KLineEdit *edit = renameLineEdit();

    switch(m_currentColumn)
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

    rename(currentItem(), m_currentColumn);
}

void Playlist::applyTag(QListViewItem *item, const QString &text, int column)
{
    //kdDebug() << "Applying " << text << " at column " << column << ", replacing \"" << item->text(column) << "\"" << endl;

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
    i->slotRefresh();
}

void Playlist::slotApplyModification(QListViewItem *item, const QString &text, int column)
{
	if (item->text(column) == text)
		return;

    QPtrList<QListViewItem> selectedSongs = KListView::selectedItems();
    if (selectedSongs.count() > 1)
    {
        if (KMessageBox::warningYesNo(0,
				      i18n("This will rename multiple files! Are you sure?"), 
				      QString::null, 
				      KStdGuiItem::yes(), 
				      KStdGuiItem::no(), 
				      "DontWarnMultipleTags") == KMessageBox::No) 
	{
	    return;
	}

        QPtrListIterator<QListViewItem> it(selectedSongs);
        for(; it.current(); ++it)
           applyTag((*it), text, column);
    }
    else
	applyTag(item, text, column);
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

    SharedSettings::instance()->toggleColumnVisible(column);
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
	after = p.createItem(info, after);
    }

    return s;
}

#include "playlist.moc"
