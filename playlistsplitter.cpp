/***************************************************************************
                          playlistsplitter.cpp  -  description
                             -------------------
    begin                : Fri Sep 13 2002
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

#include <klocale.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kdirwatch.h>
#include <kdebug.h>

#include <qinputdialog.h>
#include <qtimer.h>

#include "playlistitem.h"
#include "playlistsplitter.h"
#include "collectionlist.h"
#include "directorylist.h"
#include "playlist.h"

QStringList *PlaylistSplitter::m_mediaExtensions = 0;
QStringList *PlaylistSplitter::m_listExtensions = 0;

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
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistSplitter::PlaylistSplitter(QWidget *parent, bool restore, const char *name) : QSplitter(Qt::Horizontal, parent, name), 
										      m_playingItem(0), m_restore(restore), m_nextPlaylistItem(0)
{
    if(!m_mediaExtensions && !m_listExtensions) {
	m_mediaExtensions = new QStringList();
	m_listExtensions = new QStringList();

	m_mediaExtensions->append("mp3");
	m_mediaExtensions->append("ogg");
	m_listExtensions->append("m3u");
    }

    setupLayout();
    readConfig();

    m_editor->slotUpdateCollection();
}

PlaylistSplitter::~PlaylistSplitter()
{
    delete m_dirWatch;
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
	while(names.contains(startingWith + " (" + QString::number(playlistNumber) + ")") != 0)
	    playlistNumber++;
	
	return startingWith + " (" + QString::number(playlistNumber) + ")";	
    }
    else
    {
	while(names.contains(startingWith + ' ' + QString::number(playlistNumber)) != 0)
	    playlistNumber++;
	
	return startingWith + " " + QString::number(playlistNumber);
    }
}

QString PlaylistSplitter::playNextFile(bool random)
{
    PlaylistItem *i;

    // Four basic cases here:  (1) We've asked for a specific next item, (2) play
    // the item that's after the currently playing item, (3) play the selected 
    // item or (4) play the first item in the list.

    if(m_nextPlaylistItem && m_nextPlaylistItem != m_playingItem) {
        i = m_nextPlaylistItem;
        m_nextPlaylistItem = 0;
    }
    else if(m_playingItem) {
        Playlist *p = static_cast<Playlist *>(m_playingItem->listView());
        i = p->nextItem(m_playingItem, random);
    }
    else {
        i = playlistSelection().getFirst();
        if(!i)
            i = static_cast<PlaylistItem *>(visiblePlaylist()->firstChild());
    }

    return play(i);
}

QString PlaylistSplitter::playPreviousFile(bool random)
{
    if(!m_playingItem)
	return QString::null;

    Playlist *p = static_cast<Playlist *>(m_playingItem->listView());
    PlaylistItem *i = p->previousItem(m_playingItem, random);

    return play(i);
}

QString PlaylistSplitter::playFirstFile()
{
    Playlist *p = visiblePlaylist();
    PlaylistItem *i = static_cast<PlaylistItem *>(p->firstChild());

    return play(i);
}

void PlaylistSplitter::stop()
{
    m_nextPlaylistItem = 0;

    if(!m_playingItem)
	return;

    Playlist *p = static_cast<Playlist *>(m_playingItem->listView());

    p->setPlaying(m_playingItem, false);
    m_playingItem = 0;
}

QString PlaylistSplitter::playingArtist() const
{
    if(m_playingItem)
	return m_playingItem->text(PlaylistItem::ArtistColumn);
    else
	return QString::null;
}

QString PlaylistSplitter::playingTrack() const
{
    if(m_playingItem)
	return m_playingItem->text(PlaylistItem::TrackColumn);
    else
	return QString::null;
}

QString PlaylistSplitter::playingList() const
{
    if(m_playingItem)
	return static_cast<Playlist *>(m_playingItem->listView())->name();
    else
	return QString::null;
}

QString PlaylistSplitter::extensionsString(const QStringList &extensions, const QString &type) // static
{
    QStringList l;

    for(QStringList::ConstIterator it = extensions.begin(); it != extensions.end(); ++it)
	l.append(QString("*." + (*it)));

    // i.e. "*.m3u, *.mp3|Media Files"

    QString s = l.join(" ");

    if(type != QString::null)
	s += "|" + type + " (" + l.join(", ") + ")";

    return s;
}

void PlaylistSplitter::open(const QString &file) 
{
    if(file.isEmpty())
	return;

    if(visiblePlaylist() == m_collection || 
       KMessageBox::questionYesNo(this, i18n("Do you want to add this item to the current list or to the collection list?"), 
				  QString::null, KGuiItem(i18n("Current")), KGuiItem(i18n("Collection"))) == KMessageBox::No)
	slotAddToPlaylist(file, m_collection);
    else
	slotAddToPlaylist(file, visiblePlaylist());
}

void PlaylistSplitter::open(const QStringList &files) 
{
    if(files.isEmpty())
	return;
    
    if(visiblePlaylist() == m_collection || 
       KMessageBox::questionYesNo(this, i18n("Do you want to add these items to the current list or to the collection list?"), 
				  QString::null, KGuiItem(i18n("Current")), KGuiItem(i18n("Collection"))) == KMessageBox::No)
	slotAddToPlaylist(files, m_collection);
    else
	slotAddToPlaylist(files, visiblePlaylist());
}

Playlist *PlaylistSplitter::createPlaylist(const QString &name)
{
    Playlist *p = new Playlist(m_playlistStack, name.latin1());
    setupPlaylist(p, true);
    return p;
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::slotOpen()
{
    QStringList files = KFileDialog::getOpenFileNames(QString::null, 
						      extensionsString((*m_mediaExtensions + *m_listExtensions), i18n("Media Files")));
    open(files);
}

void PlaylistSplitter::slotOpenDirectory()
{ 
    DirectoryList *l = new DirectoryList(m_directoryList, this, "directoryList");

    m_directoryQueue.clear();
    m_directoryQueueRemove.clear();

    connect(l, SIGNAL(signalDirectoryAdded(const QString &)), this, SLOT(slotQueueDirectory(const QString &)));
    connect(l, SIGNAL(signalDirectoryRemoved(const QString &)), this, SLOT(slotQueueDirectoryRemove(const QString &)));

    if(l->exec() == QDialog::Accepted) {
	open(m_directoryQueue);
	for(QStringList::Iterator it = m_directoryQueue.begin(); it !=  m_directoryQueue.end(); it++)
	    m_dirWatch->addDir(*it, false, true);
	    
	m_directoryList += m_directoryQueue;
	for(QStringList::Iterator it = m_directoryQueueRemove.begin(); it !=  m_directoryQueueRemove.end(); it++) {
	    m_dirWatch->removeDir(*it);
	    m_directoryList.remove(*it);
	}
    }
}
void PlaylistSplitter::slotSetEditorVisible(bool visible)
{
    if(visible)
	m_editor->show();
    else
	m_editor->hide();
}

Playlist *PlaylistSplitter::slotCreatePlaylist()
{
    bool ok;

    // If this text is changed, please also change it in PlaylistBox::duplicate().

    QString name = QInputDialog::getText(i18n("New Playlist..."), i18n("Please enter a name for the new playlist:"),
					 QLineEdit::Normal, uniquePlaylistName(), &ok);
    if(ok)
	return createPlaylist(name);
    else
	return 0;
}

void PlaylistSplitter::slotSelectPlaying()
{
    if(!m_playingItem)
	return;

    Playlist *l = static_cast<Playlist *>(m_playingItem->listView());
	
    if(!l)
	return;

    l->clearSelection();
    l->setSelected(m_playingItem, true);
    l->ensureItemVisible(m_playingItem);
    
    m_playlistBox->raise(l);
}

void PlaylistSplitter::slotDeleteSelectedItems()
{
    Playlist *p = visiblePlaylist();
    if(p)
	p->slotDeleteSelectedItems();
}

void PlaylistSplitter::slotAddToPlaylist(const QString &file, Playlist *list)
{
    KApplication::setOverrideCursor(Qt::waitCursor);
    addImpl(file, list);
    KApplication::restoreOverrideCursor();
    
    if(m_editor)
	m_editor->slotUpdateCollection();
}

void PlaylistSplitter::slotAddToPlaylist(const QStringList &files, Playlist *list)
{
    KApplication::setOverrideCursor(Qt::waitCursor);
    for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it)
        addImpl(*it, list);
    KApplication::restoreOverrideCursor();

    if(m_editor)
	m_editor->slotUpdateCollection();
}

void PlaylistSplitter::slotToggleColumnVisible(int column)
{
    m_visibleColumns[column] = ! m_visibleColumns[column];
    if(visiblePlaylist())
	setupColumns(visiblePlaylist());
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::setupLayout()
{
    m_playlistBox = new PlaylistBox(this, "playlistBox");

    // Create a splitter to go between the playlists and the editor.

    QSplitter *editorSplitter = new QSplitter(Qt::Vertical, this, "editorSplitter");

    // Create the playlist and the editor.

    m_playlistStack = new QWidgetStack(editorSplitter, "playlistStack");
    m_editor = new TagEditor(editorSplitter, "tagEditor");

    // Make the editor as small as possible (or at least as small as recommended)

    editorSplitter->setResizeMode(m_editor, QSplitter::FollowSizeHint);

    // Make the connection that will update the selected playlist when a 
    // selection is made in the playlist box.

    connect(m_playlistBox, SIGNAL(signalCurrentChanged(Playlist *)), 
	    this, SLOT(slotChangePlaylist(Playlist *)));

    connect(m_playlistBox, SIGNAL(signalDoubleClicked()), this, SIGNAL(signalListBoxDoubleClicked()));

    // Create the collection list; this should always exist.  This has a 
    // slightly different creation process than normal playlists (since it in
    // fact is a subclass) so it is created here rather than by using 
    // slotCreatePlaylist().

    CollectionList::initialize(m_playlistStack, m_restore);
    m_collection = CollectionList::instance();
    setupPlaylist(m_collection, true, "folder_sound");
    connect(m_collection, SIGNAL(signalCollectionChanged()), m_editor, SLOT(slotUpdateCollection()));

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

	    QString playlistsFile = KGlobal::dirs()->saveLocation("appdata") + "playlists";

	    QFile f(playlistsFile);
	    
	    if(f.open(IO_ReadOnly)) {
		QDataStream s(&f);
		while(!s.atEnd()) {
		    Playlist *p = new Playlist(m_playlistStack);
		    s >> *p;

		    // check to see if we've alredy loaded this item before continuing

		    if(p->fileName().isEmpty() || !m_playlistFiles.insert(p->fileName()))
			setupPlaylist(p);
		    else
			delete p;
		}
	    }

	    m_directoryList = config->readListEntry("DirectoryList");
	    QTimer::singleShot(0, this, SLOT(slotScanDirectories()));

	    m_dirWatch = new KDirWatch();
	    connect(m_dirWatch, SIGNAL(dirty(const QString &)), this, SLOT(slotDirChanged(const QString &)));

            for(QStringList::Iterator it = m_directoryList.begin(); it != m_directoryList.end(); ++it)
		m_dirWatch->addDir(*it, false, true);

	    m_dirWatch->startScan();
	}

	// restore the list of hidden and shown columns

	if(m_collection) {
	    // the last column is just a filler
	    m_visibleColumns.resize(m_collection->columns() - 1, true);
	    QValueList<int> l = config->readIntListEntry("VisibleColumns");
	    m_collection->setSorting( config->readNumEntry("SortColumn", 1) );

	    uint i = 0;
	    for(QValueList<int>::Iterator it = l.begin(); it != l.end(); ++it) {
		if(! bool(*it)) {
		    m_visibleColumns[i] = bool(*it);
		    m_collection->hideColumn(i);
		}

		// while we're looping go ahead and populate m_columnNames
		
		m_columnNames.append(m_collection->columnText(i));

		i++;
	    }
	    setupColumns(m_collection);
	}
    }
}

void PlaylistSplitter::saveConfig()
{
    KConfig *config = KGlobal::config();

    // Save the list of open playlists.
    
    if(m_restore && m_playlistBox) {

	// Start at item 1.  We want to skip the collection list.

	QString playlistsFile = KGlobal::dirs()->saveLocation("appdata") + "playlists";
	QFile f(playlistsFile);
	
	if(f.open(IO_WriteOnly)) {

	    QDataStream s(&f);

	    QPtrList<Playlist> l = m_playlistBox->playlists();

	    for(Playlist *p = l.first(); p; p = l.next())
		s << *p;

	    f.close();
	}
	{ // block for Playlists group
	    KConfigGroupSaver saver(config, "Playlists");
	    config->writeEntry("DirectoryList", m_directoryList);

	    QValueList<int> l;
	    for(uint i = 0; i < m_visibleColumns.size(); i++)
		l.append(int(m_visibleColumns[i]));
	    
	    config->writeEntry("VisibleColumns", l);
	    config->writeEntry("SortColumn", m_collection->sortColumn());

	    config->writeEntry("PlaylistSplitterSizes", sizes());
	}
    }
}

void PlaylistSplitter::addImpl(const QString &file, Playlist *list)
{
    processEvents();
    QFileInfo fileInfo(QDir::cleanDirPath(file));
    if(fileInfo.exists()) {
        if(fileInfo.isDir()) {
            QDir dir(fileInfo.filePath());
            QStringList dirContents=dir.entryList();
            for(QStringList::Iterator it = dirContents.begin(); it != dirContents.end(); ++it)
                if(*it != "." && *it != "..")
                    addImpl(fileInfo.filePath() + QDir::separator() + *it, list);
        }
        else {
            QString extension = fileInfo.extension(false);
            if(m_mediaExtensions->contains(extension) > 0)
		list->createItem(fileInfo);
	    else if(m_listExtensions->contains(extension) > 0)
		openPlaylist(fileInfo.absFilePath());
        }
    }    
}

void PlaylistSplitter::setupPlaylist(Playlist *p, bool raise, const char *icon)
{
    connect(p, SIGNAL(signalSelectionChanged(const PlaylistItemList &)), m_editor, SLOT(slotSetItems(const PlaylistItemList &)));
    connect(p, SIGNAL(signalDoubleClicked()), this, SIGNAL(signalDoubleClicked()));
    connect(p, SIGNAL(signalNumberOfItemsChanged(Playlist *)), this, SLOT(slotPlaylistCountChanged(Playlist *)));
    connect(p, SIGNAL(signalAboutToRemove(PlaylistItem *)), this, SLOT(slotPlaylistItemRemoved(PlaylistItem *)));
    connect(p, SIGNAL(signalFilesDropped(const QStringList &, Playlist *)), this, SLOT(slotAddToPlaylist(const QStringList &, Playlist *)));
    connect(p, SIGNAL(signalSetNext(PlaylistItem *)), this, SLOT(slotSetNextItem(PlaylistItem *)));

    connect(p, SIGNAL(signalToggleColumnVisible(int)), this, SLOT(slotToggleColumnVisible(int)));

    m_playlistBox->createItem(p, icon, raise);

    if(raise) {
	m_playlistStack->raiseWidget(p);
	setupColumns(p);
    }
}

Playlist *PlaylistSplitter::openPlaylist(const QString &file)
{
    QFileInfo fileInfo(file);
    if(!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable() || m_playlistFiles.insert(fileInfo.absFilePath()))
	return 0;

    Playlist *p = new Playlist(file, m_playlistStack, fileInfo.baseName(true).latin1());
    setupPlaylist(p);
    return p;
}

void PlaylistSplitter::setupColumns(Playlist *p)
{
    if(!p)
	return;
    
    for(uint i = 0; i < m_visibleColumns.size(); i++) {
	if(m_visibleColumns[i] && ! p->isColumnVisible(i))
	    p->showColumn(i);
	else if(! m_visibleColumns[i] && p->isColumnVisible(i))
	    p->hideColumn(i);
    }
}

QString PlaylistSplitter::play(PlaylistItem *item)
{
    stop();

    if(!item)
	return QString::null;

    Playlist *p = static_cast<Playlist *>(item->listView());

    p->setPlaying(item, true);

    m_playingItem = item;

    return item->absFilePath();
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::slotChangePlaylist(Playlist *p)
{
    if(!p)
	return;

    m_nextPlaylistItem = 0; 
    m_playlistStack->raiseWidget(p);
    m_editor->slotSetItems(playlistSelection());
    setupColumns(p);
    emit signalPlaylistChanged();
}

void PlaylistSplitter::slotPlaylistCountChanged(Playlist *p)
{
    if(p && p == m_playlistStack->visibleWidget())
	emit signalSelectedPlaylistCountChanged(p->childCount());
}

void PlaylistSplitter::slotPlaylistItemRemoved(PlaylistItem *item)
{
    if(item == m_playingItem)
	m_playingItem = 0;

    if(item == m_nextPlaylistItem)
	m_nextPlaylistItem = 0;
}

#include "playlistsplitter.moc"
