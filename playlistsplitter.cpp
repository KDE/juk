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
#include <kdebug.h>

#include <qinputdialog.h>

#include "playlistitem.h"
#include "playlistsplitter.h"
#include "collectionlist.h"
#include "directorylist.h"
#include "playlist.h"


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

PlaylistSplitter::PlaylistSplitter(QWidget *parent, bool restoreOnLoad, const char *name) : QSplitter(Qt::Horizontal, parent, name), 
											    playingItem(0), restore(restoreOnLoad)
{
    mediaExtensions.append("mp3");
    mediaExtensions.append("ogg");
    listExtensions.append("m3u");

    setupLayout();
    readConfig();
}

PlaylistSplitter::~PlaylistSplitter()
{
    saveConfig();
}

QString PlaylistSplitter::uniquePlaylistName(const QString &startingWith, bool useParenthesis)
{
    if(!playlistBox)
	return QString::null;

    QStringList names = playlistBox->names();

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
    Playlist *p;
    PlaylistItem *i;

    if(playingItem) {
	playingItem->setPixmap(0, 0);

	p = static_cast<Playlist *>(playingItem->listView());
	i = p->nextItem(playingItem, random);
    }
    else {
	PlaylistItemList items = playlistSelection();
	if(!items.isEmpty())
	    i = items.first();
	else {
	    p = visiblePlaylist();
	    i = static_cast<PlaylistItem *>(p->firstChild());
	}
    }

    if(i) {
	i->setPixmap(0, QPixmap(UserIcon("playing")));
	playingItem = i;
	return i->absFilePath();
    }
    else
	return QString::null;
}

QString PlaylistSplitter::playPreviousFile(bool random)
{
    if(playingItem) {
	Playlist *p = static_cast<Playlist *>(playingItem->listView());
	PlaylistItem *i = p->previousItem(playingItem, random);

	playingItem->setPixmap(0, 0);
	i->setPixmap(0, QPixmap(UserIcon("playing")));
	
	playingItem = i;
	return i->absFilePath();
    }
    else
	return QString::null;
}

QString PlaylistSplitter::playSelectedFile()
{
    stop();

    PlaylistItemList items = playlistSelection();

    if(!items.isEmpty()) {
	PlaylistItem *i = items.first();
	i->setPixmap(0, QPixmap(UserIcon("playing")));
	
	playingItem = i;
	return i->absFilePath();
    }
    else
	return QString::null;
}

QString PlaylistSplitter::playFirstFile()
{
    stop();

    Playlist *p = visiblePlaylist();
    PlaylistItem *i = static_cast<PlaylistItem *>(p->firstChild());

    if(i) {
	i->setPixmap(0, QPixmap(UserIcon("playing")));
	i->setPixmap(0, QPixmap(UserIcon("playing")));
	playingItem = i;

	return i->absFilePath();
    }
    else
	return QString::null;
}

void PlaylistSplitter::stop()
{
    if(playingItem) {
	playingItem->setPixmap(0, 0);
	playingItem = 0;
    }
}

QString PlaylistSplitter::playingArtist() const
{
    if(playingItem)
	return playingItem->text(PlaylistItem::ArtistColumn);
    else
	return QString::null;
}

QString PlaylistSplitter::playingTrack() const
{
    if(playingItem)
	return playingItem->text(PlaylistItem::TrackColumn);
    else
	return QString::null;
}

QString PlaylistSplitter::playingList() const
{
    if(playingItem)
	return static_cast<Playlist *>(playingItem->listView())->name();
    else
	return QString::null;
}

void PlaylistSplitter::add(const QString &file, Playlist *list)
{
    KApplication::setOverrideCursor(Qt::waitCursor);
    addImpl(file, list);
    KApplication::restoreOverrideCursor();
    
    if(editor)
	editor->updateCollection();
}

void PlaylistSplitter::add(const QStringList &files, Playlist *list)
{
    KApplication::setOverrideCursor(Qt::waitCursor);
    for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it)
        addImpl(*it, list);
    KApplication::restoreOverrideCursor();

    if(editor)
	editor->updateCollection();
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

    if(visiblePlaylist() == collection || 
       KMessageBox::questionYesNo(this, i18n("Do you want to add this item to the current list or to the collection list?"), 
				  QString::null, KGuiItem(i18n("Current")), KGuiItem(i18n("Collection"))) == KMessageBox::No)
	add(file, collection);
    else
	add(file, visiblePlaylist());
}

void PlaylistSplitter::open(const QStringList &files) 
{
    if(files.isEmpty())
	return;
    
    if(visiblePlaylist() == collection || 
       KMessageBox::questionYesNo(this, i18n("Do you want to add these items to the current list or to the collection list?"), 
				  QString::null, KGuiItem(i18n("Current")), KGuiItem(i18n("Collection"))) == KMessageBox::No)
	add(files, collection);
    else
	add(files, visiblePlaylist());
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::open()
{
    QStringList files = KFileDialog::getOpenFileNames(QString::null, 
						      extensionsString((mediaExtensions + listExtensions), i18n("Media Files")));
    open(files);
}

void PlaylistSplitter::openDirectory()
{ 
    DirectoryList *l = new DirectoryList(directoryList, this, "directoryList");

    directoryQueue.clear();

    connect(l, SIGNAL(directoryAdded(const QString &)), this, SLOT(queueDirectory(const QString &)));
    connect(l, SIGNAL(directoryRemoved(const QString &)), this, SLOT(queueDirectoryRemove(const QString &)));

    if(l->exec() == QDialog::Accepted) {
	open(directoryQueue);
	directoryList += directoryQueue;
	for(QStringList::Iterator it = directoryQueueRemove.begin(); it !=  directoryQueueRemove.end(); it++)
	    directoryList.remove(*it);
    }
}
void PlaylistSplitter::setEditorVisible(bool visible)
{
    if(visible)
	editor->show();
    else
	editor->hide();
}

Playlist *PlaylistSplitter::createPlaylist()
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

Playlist *PlaylistSplitter::createPlaylist(const QString &name)
{
    Playlist *p = new Playlist(this, playlistStack, name.latin1());
    setupPlaylist(p, true);
    return p;
}


void PlaylistSplitter::selectPlaying()
{
    if(!playingItem)
	return;

    Playlist *l = static_cast<Playlist *>(playingItem->listView());
	
    if(!l)
	return;

    l->clearSelection();
    l->setSelected(playingItem, true);
    l->ensureItemVisible(playingItem);
    
    playlistBox->raise(l);
}

void PlaylistSplitter::removeSelectedItems()
{
    PlaylistItemList items = playlistSelection();

    Playlist *p = visiblePlaylist();
    if(p)
	p->remove(items);
}

void PlaylistSplitter::clearSelectedItems()
{
    PlaylistItemList items = playlistSelection();

    Playlist *p = visiblePlaylist();
    if(p)
	p->clearItems(items); 
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::setupLayout()
{
    playlistBox = new PlaylistBox(this, "playlistBox");

    // Create a splitter to go between the playlists and the editor.

    QSplitter *editorSplitter = new QSplitter(Qt::Vertical, this, "editorSplitter");

    // Create the playlist and the editor.

    playlistStack = new QWidgetStack(editorSplitter, "playlistStack");
    editor = new TagEditor(editorSplitter, "tagEditor");

    // Make the editor as small as possible (or at least as small as recommended)

    editorSplitter->setResizeMode(editor, QSplitter::FollowSizeHint);

    // Make the connection that will update the selected playlist when a 
    // selection is made in the playlist box.

    connect(playlistBox, SIGNAL(signalCurrentChanged(Playlist *)), 
	    this, SLOT(changePlaylist(Playlist *)));

    connect(playlistBox, SIGNAL(signalDoubleClicked()), this, SIGNAL(listBoxDoubleClicked()));

    // Create the collection list; this should always exist.  This has a 
    // slightly different creation process than normal playlists (since it in
    // fact is a subclass) so it is created here rather than by using 
    // createPlaylist().

    CollectionList::initialize(this, playlistStack, restore);
    collection = CollectionList::instance();
    setupPlaylist(collection, true, "folder_sound");

    // Show the collection on startup.
    playlistBox->setSelected(0, true);
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
	
	if(restore) {

	    QString playlistsFile = KGlobal::dirs()->saveLocation("appdata") + "playlists";

	    QFile f(playlistsFile);
	    
	    if(f.open(IO_ReadOnly)) {
		QDataStream s(&f);
		while(!s.atEnd()) {
		    Playlist *p = new Playlist(this, playlistStack);
		    s >> *p;

		    // check to see if we've alredy loaded this item before continuing

		    if(p->fileName().isEmpty() || !playlistFiles.insert(p->fileName()))
			setupPlaylist(p);
		    else
			delete p;
		}
	    }

	    directoryList = config->readListEntry("DirectoryList");
	    open(directoryList);
	}

	// restore the list of hidden and shown columns

	if(collection) {
	    // the last column is just a filler
	    m_visibleColumns.resize(collection->columns() - 1, true);
	    QValueList<int> l = config->readIntListEntry("VisibleColumns");

	    uint i = 0;
	    for(QValueList<int>::Iterator it = l.begin(); it != l.end(); ++it) {
		if(! bool(*it)) {
		    m_visibleColumns[i] = bool(*it);
		    collection->hideColumn(i);
		}

		// while we're looping go ahead and populate m_columnNames
		
		m_columnNames.append(collection->columnText(i));

		i++;
	    }
	    setupColumns(collection);
	}
    }
}	


void PlaylistSplitter::saveConfig()
{
    KConfig *config = KGlobal::config();

    // Save the list of open playlists.
    
    if(restore && playlistBox) {

	// Start at item 1.  We want to skip the collection list.

	QString playlistsFile = KGlobal::dirs()->saveLocation("appdata") + "playlists";
	QFile f(playlistsFile);
	
	if(f.open(IO_WriteOnly)) {

	    QDataStream s(&f);

	    QPtrList<Playlist> l = playlistBox->playlists();

	    for(Playlist *p = l.first(); p; p = l.next())
		s << *p;

	    f.close();
	}
	{ // block for Playlists group
	    KConfigGroupSaver saver(config, "Playlists");
	    config->writeEntry("DirectoryList", directoryList);

	    QValueList<int> l;
	    for(uint i = 0; i < m_visibleColumns.size(); i++)
		l.append(int(m_visibleColumns[i]));
	    
	    config->writeEntry("VisibleColumns", l);

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
            if(mediaExtensions.contains(extension) > 0)
		list->createItem(fileInfo);
	    else if(listExtensions.contains(extension) > 0)
		openPlaylist(fileInfo.absFilePath());
        }
    }    
}

void PlaylistSplitter::setupPlaylist(Playlist *p, bool raise, const char *icon)
{
    connect(p, SIGNAL(selectionChanged(const PlaylistItemList &)), editor, SLOT(setItems(const PlaylistItemList &)));
    connect(p, SIGNAL(doubleClicked()), this, SIGNAL(doubleClicked()));
    connect(p, SIGNAL(collectionChanged()), editor, SLOT(updateCollection()));
    connect(p, SIGNAL(numberOfItemsChanged(Playlist *)), this, SLOT(playlistCountChanged(Playlist *)));
    connect(p, SIGNAL(aboutToRemove(PlaylistItem *)), this, SLOT(playlistItemRemoved(PlaylistItem *)));

    connect(p, SIGNAL(signalToggleColumnVisible(int)), this, SLOT(slotToggleColumnVisible(int)));

    playlistBox->createItem(p, icon, raise);

    if(raise) {
	playlistStack->raiseWidget(p);
	setupColumns(p);
    }
}

Playlist *PlaylistSplitter::openPlaylist(const QString &file)
{
    QFileInfo fileInfo(file);
    if(!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable() || playlistFiles.insert(fileInfo.absFilePath()))
	return 0;

    Playlist *p = new Playlist(this, file, playlistStack, fileInfo.baseName(true).latin1());
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

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::changePlaylist(Playlist *p)
{
    if(!p)
	return;
    
    playlistStack->raiseWidget(p);
    editor->setItems(playlistSelection());
    setupColumns(p);
    emit(playlistChanged());
}

void PlaylistSplitter::playlistCountChanged(Playlist *p)
{
    if(p && p == playlistStack->visibleWidget())
	emit(selectedPlaylistCountChanged(p->childCount()));
}

void PlaylistSplitter::playlistItemRemoved(PlaylistItem *item)
{
    if(item == playingItem)
	playingItem = 0;
}

void PlaylistSplitter::slotToggleColumnVisible(int column)
{
    m_visibleColumns[column] = ! m_visibleColumns[column];
    if(visiblePlaylist())
	setupColumns(visiblePlaylist());
}

#include "playlistsplitter.moc"
