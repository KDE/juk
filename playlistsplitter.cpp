/***************************************************************************
                          playlistsplitter.cpp  -  description
                             -------------------
    begin                : Fri Sep 13 2002
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

#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kinputdialog.h>
#include <kcmdlineargs.h>
#include <kdebug.h>

#include <qpopupmenu.h>
#include <qlayout.h>

#include "playlistsplitter.h"
#include "searchwidget.h"
#include "directorylist.h"
#include "playlistsearch.h"
#include "dynamicplaylist.h"
#include "searchplaylist.h"
#include "historyplaylist.h"
#include "mediafiles.h"
#include "advancedsearchdialog.h"

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

void processEvents()
{
    static int processed = 0;
    if(processed == 0)
        kapp->processEvents();
    processed = (processed + 1) % 5;
}

////////////////////////////////////////////////////////////////////////////////
// helper class
////////////////////////////////////////////////////////////////////////////////

// Painting is slow, so we want to be able to ignore the fact that QListView
// likes to do it so much.  Specifically while loading -- when without a bit of
// hackery it takes more time to paint the new items than it does to read them.
// This helper class operates on a Playlist while loading items and throws out
// most of the repaint events that are issued.

class PaintEater : public QObject
{
public:
    PaintEater(Playlist *list) : QObject(list), m_list(list),
				 m_allowOne(false), m_previousHeight(0)
    {
        // We want to catch paint events for both the contents and the frame of
        // our listview.

	list->installEventFilter(this);
	list->viewport()->installEventFilter(this);
    }

private:
    virtual bool eventFilter(QObject *o, QEvent *e)
    {
	if(e->type() == QEvent::Paint) {

            // There are two cases where we want to let our viewport repaint
            // itself -- if the actual contents have changed as indicated by
            // m_allowOne being true, or if the height has changed indicating
            // that we've either scrolled or resized the widget.

	    if(o == m_list->viewport()) {
                if(m_allowOne) {
                    m_allowOne = false;
                    return false;
                }

                int newHeight = static_cast<QPaintEvent *>(e)->rect().top();

                if(m_previousHeight != newHeight) {
                    m_previousHeight = newHeight;
                    return false;
                }
            }
            else
		m_allowOne = true;

	    if(m_list->count() < 20)
                m_list->slotWeightDirty();

	    return true;
	}

	return false;
    }

    Playlist *m_list;
    bool m_allowOne;
    int m_previousHeight;
};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistSplitter::PlaylistSplitter(QWidget *parent, const char *name) :
    QSplitter(Qt::Horizontal, parent, name),
    m_playingItem(0), m_searchWidget(0), m_history(0),
    m_dynamicList(0), m_nextPlaylistItem(0)
{
#ifndef NO_DEBUG
    m_restore = KCmdLineArgs::parsedArgs()->isSet("restore");
#else
    m_restore = true;
#endif

    setupLayout();
    readConfig();

    m_editor->slotUpdateCollection();
}

PlaylistSplitter::~PlaylistSplitter()
{
    saveConfig();
}

QString PlaylistSplitter::uniquePlaylistName(const QString &startingWith, bool useParenthesis)
{
    if(!m_playlistBox)
	return QString::null;

    QStringList names = m_playlistBox->names();

    int playlistNumber = 1;

    // while the list contains more than zero instances of the generated
    // string...

    if(useParenthesis) {
	while(names.find(startingWith + " (" + QString::number(playlistNumber) + ")") != names.end())
	    playlistNumber++;

	return startingWith + " (" + QString::number(playlistNumber) + ")";
    }
    else {
	while(names.find(startingWith + ' ' + QString::number(playlistNumber)) != names.end())
	    playlistNumber++;

	return startingWith + " " + QString::number(playlistNumber);
    }
}

QString PlaylistSplitter::nextFile()
{
    PlaylistItem *i = 0;

    if(m_nextPlaylistItem && m_nextPlaylistItem != m_playingItem) {
        i = m_nextPlaylistItem;
        m_nextPlaylistItem = 0;
    }
    else if(m_playingItem) {
        i = m_playingItem->playlist()->nextItem(m_playingItem);
	// TODO: check to see if we're in loop / random mode
    }

    return play(i);
}

QString PlaylistSplitter::currentFile()
{
    PlaylistItem *i = 0;
    const bool random = false; // TODO: should check this

    
    if(m_nextPlaylistItem && m_nextPlaylistItem != m_playingItem) {
        i = m_nextPlaylistItem;
        m_nextPlaylistItem = 0;
    }

    // Play the selected item.

    else if(playlistSelection().size() > 0) {
        i = playlistSelection().first();
        if(!i)
	    i = m_playingItem->playlist()->nextItem(0, random);
    }

    // Play the first item in the list.

    else {
	kdDebug(65432) << k_funcinfo << "Playing the first item in the current list." << endl;
	i = visiblePlaylist()->nextItem(0, random);
    }

    return play(i);
}

QString PlaylistSplitter::playNextFile(bool random, bool loopPlaylist)
{
    PlaylistItem *i;

    // Four basic cases here:  (1) We've asked for a specific next item, (2) play
    // the item that's after the currently playing item, (3) play the selected
    // item or (4) play the first item in the list.

    // (1) we've asked for a specific next item
    if(m_nextPlaylistItem && m_nextPlaylistItem != m_playingItem) {
	kdDebug(65432) << k_funcinfo << "Playing the requested 'next' item." << endl;

        i = m_nextPlaylistItem;
        m_nextPlaylistItem = 0;
    }
    // (2) play the item after the currently selected item
    else if(m_playingItem) {
	Playlist *p = m_playingItem->playlist();
        i = p->nextItem(m_playingItem, random);
        if(!i && loopPlaylist) {
	    PlaylistItemList visibleItems = p->visibleItems();
            i = visibleItems.isEmpty() ? 0 : visibleItems.front();
	}
    }
    // (3) play the selected item
    else if(playlistSelection().size() > 0) {
        i = playlistSelection().first();
        if(!i)
	    i = m_playingItem->playlist()->nextItem(0, random);
    }
    // (4) Play the first item in the list.
    else
	i = visiblePlaylist()->nextItem(0, random);

    return play(i);
}

QString PlaylistSplitter::playPreviousFile(bool random)
{
    if(!m_playingItem)
	return QString::null;

    Playlist *p = m_playingItem->playlist();
    PlaylistItem *i = p->previousItem(m_playingItem, random);

    return play(i);
}

void PlaylistSplitter::populatePlayHistoryMenu(QPopupMenu *menu, bool random)
{
    Playlist *p = m_playingItem->playlist();
    PlaylistItemList list = p->historyItems(m_playingItem, random);
    menu->clear();
    int i = 0;
    for(PlaylistItemList::Iterator it = list.begin(); it != list.end(); ++it)
        menu->insertItem((*it)->tag()->title(), ++i);
}

QString PlaylistSplitter::playSelectedFile()
{
    if(playlistSelection().isEmpty())
	return QString::null;
    else
	return play(playlistSelection().first());
}

QString PlaylistSplitter::playFirstFile()
{
    Playlist *p = visiblePlaylist();
    PlaylistItem *i = static_cast<PlaylistItem *>(p->firstChild());

    return play(i);
}

QString PlaylistSplitter::playRandomFile()
{
    Playlist *p = visiblePlaylist();
    PlaylistItem *i = static_cast<PlaylistItem *>(p->firstChild());

    // Not exactly random (the first item won't be taken into account)
    return play(p->nextItem(i, true));
}

void PlaylistSplitter::stop()
{
    m_nextPlaylistItem = 0;

    if(!m_playingItem)
	return;

    Playlist *p = m_playingItem->playlist();

    if(p)
	p->setPlaying(m_playingItem, false);

    m_playingItem = 0;
}

QString PlaylistSplitter::playingArtist() const
{
    if(m_playingItem) {
	int offset =  m_playingItem->playlist()->columnOffset();
	return m_playingItem->text(PlaylistItem::ArtistColumn + offset);
    }
    else
	return QString::null;
}

QString PlaylistSplitter::playingTrack() const
{
    if(m_playingItem) {
	int offset =  m_playingItem->playlist()->columnOffset();
	return m_playingItem->text(PlaylistItem::TrackColumn + offset);
    }
    else
	return QString::null;
}

QString PlaylistSplitter::playingList() const
{
    if(m_playingItem)
	return m_playingItem->playlist()->name();
    else
	return QString::null;
}

void PlaylistSplitter::open(const QString &file)
{
    if(file.isEmpty())
	return;

    if(visiblePlaylist() == m_collection ||
       KMessageBox::questionYesNo(this,
				  i18n("Do you want to add this item to the current list or to the collection list?"),
				  QString::null,
				  KGuiItem(i18n("Current")),
				  KGuiItem(i18n("Collection"))) == KMessageBox::No)
    {
	slotAddToPlaylist(file, m_collection);
    }
    else
	slotAddToPlaylist(file, visiblePlaylist());
}

void PlaylistSplitter::open(const QStringList &files)
{
    if(files.isEmpty())
	return;

    if(visiblePlaylist() == m_collection ||
       KMessageBox::questionYesNo(this,
				  i18n("Do you want to add these items to the current list or to the collection list?"),
				  QString::null,
				  KGuiItem(i18n("Current")),
				  KGuiItem(i18n("Collection"))) == KMessageBox::No)
    {
	slotAddToPlaylist(files, m_collection);
    }
    else
	slotAddToPlaylist(files, visiblePlaylist());
}

Playlist *PlaylistSplitter::createPlaylist(const QString &name)
{
    Playlist *p = new Playlist(m_playlistStack, name);
    setupPlaylist(p, true);
    return p;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::slotOpen()
{
    open(MediaFiles::openDialog(this));
}

void PlaylistSplitter::slotOpenDirectory()
{
    DirectoryList *l = new DirectoryList(m_directoryList, this, "directoryList");

    m_directoryQueue.clear();
    m_directoryQueueRemove.clear();

    connect(l, SIGNAL(signalDirectoryAdded(const QString &)),
	    this, SLOT(slotQueueDirectory(const QString &)));
    connect(l, SIGNAL(signalDirectoryRemoved(const QString &)),
	    this, SLOT(slotQueueDirectoryRemove(const QString &)));

    if(l->exec() == QDialog::Accepted) {
	open(m_directoryQueue);
	for(QStringList::Iterator it = m_directoryQueue.begin(); it !=  m_directoryQueue.end(); it++)
	    m_dirWatch.addDir(*it, false, true);

	m_directoryList += m_directoryQueue;

	QStringList::Iterator it = m_directoryQueueRemove.begin();
	for(; it !=  m_directoryQueueRemove.end(); it++) {
	    m_dirWatch.removeDir(*it);
	    m_directoryList.remove(*it);
	}
    }
}

Playlist *PlaylistSplitter::slotCreatePlaylist(const QString &name_)
{
    bool ok;

    // If this text is changed, please also change it in PlaylistBox::duplicate().

    QString name = KInputDialog::getText(i18n("Create New Playlist"),
	i18n("Please enter a name for the new playlist:"),
	name_.isNull() ? uniquePlaylistName() : name_, &ok);
    if(ok)
	return createPlaylist(name);
    else
	return 0;
}

Playlist *PlaylistSplitter::slotCreatePlaylistFromDir()
{
    const QString dirName = KFileDialog::getExistingDirectory();
    if(dirName.isEmpty())
        return 0;

    Playlist *playlist = slotCreatePlaylist(dirName.mid(dirName.findRev('/') + 1));
    if(!playlist)
        return 0;

    slotAddToPlaylist(dirName, playlist);

    return playlist;
}

void PlaylistSplitter::slotSelectPlaying()
{
    if(!m_playingItem)
	return;

    Playlist *l = m_playingItem->playlist();

    if(!l)
	return;

    l->clearSelection();
    l->setSelected(m_playingItem, true);
    l->ensureItemVisible(m_playingItem);

    if(l != visiblePlaylist())
	m_playlistBox->raise(l);

    m_playlistBox->ensureCurrentVisible();
}

void PlaylistSplitter::slotDeleteSelectedItems()
{
    Playlist *p = visiblePlaylist();
    if(p)
	p->slotRemoveSelectedItems();
}

void PlaylistSplitter::slotAddToPlaylist(const QString &file, Playlist *list, PlaylistItem *after)
{
    if(!after)
	after = static_cast<PlaylistItem *>(list->lastItem());

    KApplication::setOverrideCursor(Qt::waitCursor);
    PaintEater pe(list);
    addImpl(file, list, after);
    list->slotWeightDirty();
    list->emitCountChanged();
    KApplication::restoreOverrideCursor();

    if(m_editor)
	m_editor->slotUpdateCollection();
}

void PlaylistSplitter::slotDeletePlaylist()
{
    if(m_playingItem && m_playingItem->listView() == visiblePlaylist())
	m_playingItem = 0;

    m_playlistBox->deleteItems();
}

void PlaylistSplitter::slotAddToPlaylist(const QStringList &files, Playlist *list, PlaylistItem *after)
{
    if(!after)
	after = static_cast<PlaylistItem *>(list->lastItem());

    KApplication::setOverrideCursor(Qt::waitCursor);

    PaintEater pe(list);

    for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it)
        after = addImpl(*it, list, after);

    list->slotWeightDirty();
    list->emitCountChanged();

    KApplication::restoreOverrideCursor();

    if(m_editor)
	m_editor->slotUpdateCollection();
}

void PlaylistSplitter::slotSetSearchVisible(bool visible)
{
    m_searchWidget->setShown(visible);
    redisplaySearch();
}

void PlaylistSplitter::slotSetHistoryVisible(bool visible)
{
    if(visible && !m_history) {
	m_history = new HistoryPlaylist(m_playlistStack);
	setupPlaylist(m_history, false, "history", true);
	return;
    }

    if(!visible && m_history) {
	m_playlistBox->deleteItem(m_history);
	m_history = 0;
    }
}

void PlaylistSplitter::slotAdvancedSearch()
{
    AdvancedSearchDialog *d =
	new AdvancedSearchDialog(uniquePlaylistName(i18n("Search Playlist")), this);
    AdvancedSearchDialog::Result r = d->exec();
    delete d;

    if(r.result == AdvancedSearchDialog::Accepted) {
	SearchPlaylist *p = new SearchPlaylist(m_playlistStack, r.search, r.playlistName);
	setupPlaylist(p, true, "find");
    }
}

void PlaylistSplitter::slotGuessTagInfo(TagGuesser::Type type)
{
    visiblePlaylist()->slotGuessTagInfo(type);
    if(m_editor)
        m_editor->slotRefresh();
}

void PlaylistSplitter::slotRenameFile()
{
    visiblePlaylist()->slotRenameFile();
    if(m_editor)
        m_editor->slotRefresh();
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::setupLayout()
{
    setOpaqueResize(false);

    m_playlistBox = new PlaylistBox(this, "playlistBox");

    connect(m_playlistBox, SIGNAL(signalCreateSearchList(const PlaylistSearch &, const QString &, const QString &)),
            this, SLOT(slotCreateSearchList(const PlaylistSearch &, const QString &, const QString &)));

    connect(m_playlistBox, SIGNAL(signalCreatePlaylist(const QStringList &)),
	    this, SLOT(slotCreatePlaylist(const QStringList &)));

    // Create a splitter to go between the playlists and the editor.

    QSplitter *editorSplitter = new QSplitter(Qt::Vertical, this, "editorSplitter");

    // Create the playlist and the editor.

    QWidget *top = new QWidget(editorSplitter);
    QVBoxLayout *topLayout = new QVBoxLayout(top);

    m_playlistStack = new QWidgetStack(top, "playlistStack");
    m_editor = new TagEditor(editorSplitter, "tagEditor");

    // Make the editor as small as possible (or at least as small as recommended)

    editorSplitter->setResizeMode(m_editor, QSplitter::FollowSizeHint);

    // Make the connection that will update the selected playlist when a
    // selection is made in the playlist box.

    connect(m_playlistBox, SIGNAL(signalCurrentChanged(const PlaylistList &)),
	    this, SLOT(slotChangePlaylist(const PlaylistList &)));

    connect(m_playlistBox, SIGNAL(signalDoubleClicked()), this, SIGNAL(signalListBoxDoubleClicked()));

    // Create the collection list; this should always exist.  This has a
    // slightly different creation process than normal playlists (since it in
    // fact is a subclass) so it is created here rather than by using
    // slotCreatePlaylist().

    CollectionList::initialize(m_playlistStack, m_restore);
    m_collection = CollectionList::instance();
    setupPlaylist(m_collection, true, "folder_sound", true);
    connect(m_collection, SIGNAL(signalCollectionChanged()), m_editor, SLOT(slotUpdateCollection()));

    // Create the search widget -- this must be done after the CollectionList is created.

    m_searchWidget = new SearchWidget(top, "searchWidget");
    connect(m_searchWidget, SIGNAL(signalQueryChanged()), this, SLOT(slotShowSearchResults()));
    connect(m_searchWidget, SIGNAL(signalAdvancedSearchClicked()), this, SLOT(slotAdvancedSearch()));

    connect(CollectionList::instance(), SIGNAL(signalVisibleColumnsChanged()),
	    this, SLOT(slotVisibleColumnsChanged()));

    topLayout->addWidget(m_searchWidget);
    topLayout->addWidget(m_playlistStack);

    // Show the collection on startup.
    m_playlistBox->setSelected(0, true);
}

void PlaylistSplitter::readConfig()
{
    KConfig *config = KGlobal::config();
    { // block for Playlists group
	KConfigGroupSaver saver(config, "Playlists");

	QValueList<int> splitterSizes = config->readIntListEntry("PlaylistSplitterSizes");
	if(splitterSizes.isEmpty()) {
	    splitterSizes.append(100);
	    splitterSizes.append(640);
	}
	setSizes(splitterSizes);

	if(m_restore) {

	    readPlaylists();

	    m_directoryList = config->readPathListEntry("DirectoryList");
	    QTimer::singleShot(0, this, SLOT(slotScanDirectories()));

	    connect(&m_dirWatch, SIGNAL(dirty(const QString &)),
		    this, SLOT(slotDirChanged(const QString &)));

	    QStringList::Iterator it = m_directoryList.begin();
            for(; it != m_directoryList.end(); ++it)
		m_dirWatch.addDir(*it, false, true);

	    m_dirWatch.startScan();
	}

	if(m_collection) {

	    // Restore the list of hidden and shown columns

	    for(int i = 0; i < m_collection->columns(); i++)
		m_columnNames.append(m_collection->columnText(i));

	    // Restore the collection list's sort column -- the other playlists
	    // are hanlded in the serialization code.

	    m_collection->setSortColumn(config->readNumEntry("CollectionListSortColumn", 1));
	}

    }
}

void PlaylistSplitter::saveConfig()
{
    KConfig *config = KGlobal::config();

    // Save the list of open playlists.

    if(m_restore && m_playlistBox) {

	savePlaylists();

	{ // block for Playlists group
	    KConfigGroupSaver saver(config, "Playlists");
	    config->writePathEntry("DirectoryList", m_directoryList);
	    config->writeEntry("SortColumn", m_collection->sortColumn());
	    config->writeEntry("PlaylistSplitterSizes", sizes());
	    config->writeEntry("CollectionListSortColumn", m_collection->sortColumn());
	}
    }
}

PlaylistItem *PlaylistSplitter::addImpl(const QString &file, Playlist *list, PlaylistItem *after)
{
    processEvents();

    const QFileInfo fileInfo = QDir::cleanDirPath(file);

    if(!fileInfo.exists())
	return after;

    if(MediaFiles::isMediaFile(file))
	return list->createItem(fileInfo, QString::null, after, false);

    if(MediaFiles::isPlaylistFile(file)) {
	openPlaylist(fileInfo.absFilePath());
	return after;
    }

    if(fileInfo.isDir()) {

	QDir dir = fileInfo.filePath();
	QStringList dirContents = dir.entryList();

	for(QStringList::Iterator it = dirContents.begin(); it != dirContents.end(); ++it) {
	    if(*it != "." && *it != "..")
		after = addImpl(fileInfo.filePath() + QDir::separator() + *it, list, after);
	}
    }

    return after;
}

void PlaylistSplitter::setupPlaylist(Playlist *p, bool raise, const char *icon, bool sortedFirst)
{
    connect(p, SIGNAL(signalSelectionChanged(const PlaylistItemList &)),
	    m_editor, SLOT(slotSetItems(const PlaylistItemList &)));

    connect(p, SIGNAL(doubleClicked(QListViewItem *)),
	    this, SIGNAL(signalActivated()));

    connect(p, SIGNAL(returnPressed(QListViewItem *)), 
	    this, SIGNAL(signalActivated()));

    connect(p, SIGNAL(signalCountChanged(Playlist *)),
	    this, SLOT(slotPlaylistCountChanged(Playlist *)));

    connect(p, SIGNAL(signalAboutToRemove(PlaylistItem *)),
	    this, SLOT(slotPlaylistItemRemoved(PlaylistItem *)));

    connect(p, SIGNAL(signalFilesDropped(const QStringList &, Playlist *, PlaylistItem *)),
	    this, SLOT(slotAddToPlaylist(const QStringList &, Playlist *, PlaylistItem *)));

    connect(p, SIGNAL(signalSetNext(PlaylistItem *)),
	    this, SLOT(slotSetNextItem(PlaylistItem *)));

    connect(p, SIGNAL(signalCreatePlaylist(const PlaylistItemList &)),
	    this, SLOT(slotCreatePlaylist(const PlaylistItemList &)));

    if(icon)
	m_playlistBox->createItem(p, icon, raise, sortedFirst);

    if(raise) {
	PlaylistList l;
	l.append(p);
	slotChangePlaylist(l);
    }
}

Playlist *PlaylistSplitter::openPlaylist(const QString &file)
{
    QFileInfo fileInfo(file);
    if(!fileInfo.exists() ||
       !fileInfo.isFile() ||
       !fileInfo.isReadable() ||
       m_playlistFiles.insert(fileInfo.absFilePath()))
    {
	return 0;
    }

    Playlist *p = new Playlist(file, m_playlistStack, fileInfo.baseName(true));
    setupPlaylist(p);
    return p;
}

QString PlaylistSplitter::play(PlaylistItem *item)
{
    stop();

    if(!item) {
	kdDebug(65432) << k_funcinfo << "The current item is null." << endl;
	return QString::null;
    }

    Playlist *p = item->playlist();

    if(!p)
	return QString::null;

    p->setPlaying(item, true);

    m_playingItem = item;

    if(m_history && p != m_history) {
	PlaylistItemList l;
	l.append(item);
	m_history->createItems(l);
    }

    return item->absFilePath();
}

void PlaylistSplitter::redisplaySearch()
{
    // kdDebug(65432) << k_funcinfo << endl;

    if(!m_searchWidget->isVisible() || visiblePlaylist()->search().isEmpty())
	visiblePlaylist()->setItemsVisible(visiblePlaylist()->items(), true);
    else {
	Playlist::setItemsVisible(visiblePlaylist()->search().matchedItems(), true);
	Playlist::setItemsVisible(visiblePlaylist()->search().unmatchedItems(), false);
    }
}

void PlaylistSplitter::readPlaylists()
{
    QString playlistsFile = KGlobal::dirs()->saveLocation("appdata") + "playlists";

    QFile f(playlistsFile);

    if(!f.open(IO_ReadOnly))
	return;

    QDataStream fs(&f);

    Q_INT32 version;
    fs >> version;

    switch(version) {
    case 1:
    case 2:
    {
	// Our checksum is only for the values after the version and checksum so
	// we want to get a byte array with just the checksummed data.

	QByteArray data;
	Q_UINT16 checksum;
	fs >> checksum >> data;

	if(checksum != qChecksum(data.data(), data.size()))
	    return;

	// Create a new stream just based on the data.

	QDataStream s(data, IO_ReadOnly);

	while(!s.atEnd()) {

	    Q_INT32 playlistType;
	    s >> playlistType;

	    Playlist *playlist;

	    switch(playlistType) {
	    case Search:
	    {
		SearchPlaylist *p = new SearchPlaylist(m_playlistStack);
		s >> *p;
		setupPlaylist(p, false, "find");

		playlist = p;

		break;
	    }
	    case History:
	    {
		slotSetHistoryVisible(true);
		s >> *m_history;

		playlist = m_history;
		break;
	    }
	    default:
		Playlist *p = new Playlist(m_playlistStack);
		s >> *p;

		if(!p->fileName().isEmpty() && m_playlistFiles.insert(p->fileName())) {
		    delete p;
		    p = 0;
		}
		else
		    setupPlaylist(p);

		playlist = p;

		break;
	    }
	    if(version == 2) {
		Q_INT32 sortColumn;
		s >> sortColumn;
		playlist->setSorting(sortColumn);
	    }
	}
	break;
    }
    default:
    {
	// Because the original version of the playlist cache did not contain a
	// version number, we want to revert to the beginning of the file before
	// reading the data.

	f.reset();

	while(!fs.atEnd()) {
	    Playlist *p = new Playlist(m_playlistStack);
	    fs >> *p;

	    // check to see if we've alredy loaded this item before continuing

	    if(p->fileName().isEmpty() || !m_playlistFiles.insert(p->fileName()))
		setupPlaylist(p);
	    else
		delete p;
	}
	break;
    }
    }

    f.close();
}

void PlaylistSplitter::savePlaylists()
{
    QString playlistsFile = KGlobal::dirs()->saveLocation("appdata") + "playlists";
    QFile f(playlistsFile);

    if(!f.open(IO_WriteOnly))
	return;

    QByteArray data;
    QDataStream s(data, IO_WriteOnly);

    PlaylistList l = m_playlistBox->playlists();

    for(PlaylistList::Iterator it = l.begin(); it != l.end(); it++) {
	if(*it) {
	    if(*it == m_history) {
		s << Q_INT32(History)
		  << *m_history;
	    }
	    else if(dynamic_cast<SearchPlaylist *>(*it)) {
		s << Q_INT32(Search)
		  << *static_cast<SearchPlaylist *>(*it);
	    }
	    else {
		s << Q_INT32(Normal)
		  << *(*it);
	    }
	    s << Q_INT32((*it)->sortColumn());
	}
    }

    QDataStream fs(&f);
    fs << Q_INT32(playlistCacheVersion);
    fs << qChecksum(data.data(), data.size());

    fs << data;
    f.close();
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::slotChangePlaylist(const PlaylistList &l)
{
    if(l.isEmpty()) {
	emit signalPlaylistChanged();
	return;
    }
    if(l.count() == 1 && l.first() == visiblePlaylist())
	return;
    
    // Save the current dynamic list so that we can delete it when we're done
    // showing the next list.  The two situations are that we're either showing
    // an existing, non-dynamic list or that we're creating a dynamic list; in 
    // both cases we want to get rid of the current one.
    //
    // If in fact the currently visible list *is not* a dynamic list, then
    // m_dyanmicList will simply be zero, making deleting it at the end of this
    // method just a no-op.
    //
    // And finally, because we will end up doing a recursive call to this method
    // to show the dynamic playlist (second case calls the first case), we want
    // to make sure that in that case we don't delete the very thing we're
    // being asked to show.  (Hence the conditional assignment.)

    Playlist *current = l.first() != m_dynamicList ? m_dynamicList : 0;

    m_nextPlaylistItem = 0;

    // First case:  We're just showing one, currently existing list.

    if(l.count() == 1) {

	l.first()->applySharedSettings();
	m_playlistStack->raiseWidget(l.first());
	m_editor->slotSetItems(playlistSelection());

	if(m_dynamicList != l.first())
	   m_dynamicList = 0;

	if(m_searchWidget)
	    m_searchWidget->setSearch(l.first()->search());
    }

    // Second case: There are multiple playlists in our list, so we need to create
    // a new "dynamic list" that is the union of these playlists.

    else {
	m_dynamicList = new DynamicPlaylist(l, m_playlistStack, i18n("Dynamic List"));

	// Note that this call will end up making a recursive call to this
	// method, but in that call since there will only be one list, it will
	// take the "first case" above.
	
	setupPlaylist(m_dynamicList, true, 0);
	m_dynamicList->applySharedSettings();
    }

    if(current) {
	m_playingItem = 0;
	delete current;
    }

    emit signalPlaylistChanged();
}

void PlaylistSplitter::slotPlaylistCountChanged(Playlist *p)
{
    if(p && p == m_playlistStack->visibleWidget()) {
	emit signalSelectedPlaylistCountChanged(p->childCount());
	emit signalSelectedPlaylistTimeChanged(p->totalTime());
    }
}

void PlaylistSplitter::slotPlaylistItemRemoved(PlaylistItem *item)
{
    if(item == m_playingItem)
	m_playingItem = 0;

    if(item == m_nextPlaylistItem)
	m_nextPlaylistItem = 0;
}

void PlaylistSplitter::slotCreatePlaylist(const QStringList &files)
{
    Playlist *p = slotCreatePlaylist();
    if(p)
	slotAddToPlaylist(files, p);
}


void PlaylistSplitter::slotCreatePlaylist(const PlaylistItemList &items)
{
    if(items.isEmpty())
	return;

    Playlist *playlist = slotCreatePlaylist();

    if(!playlist)
        return;

    playlist->createItems(items);
}

void PlaylistSplitter::slotShowSearchResults()
{
    PlaylistList playlists;
    playlists.append(visiblePlaylist());

    PlaylistSearch search = m_searchWidget->search(playlists);

    visiblePlaylist()->setSearch(search);
    redisplaySearch();
}

void PlaylistSplitter::slotVisibleColumnsChanged()
{
    slotShowSearchResults();
}

void PlaylistSplitter::slotCreateSearchList(const PlaylistSearch &search, 
					    const QString &searchCategory,
					    const QString &name)
{
    SearchPlaylist *p = new SearchPlaylist(m_playlistStack, search, name);
    m_playlistBox->createSearchItem(p, searchCategory);
    setupPlaylist(p, false, 0);
}

#include "playlistsplitter.moc"


// vim:ts=8
