/***************************************************************************
                          playlistsplitter.cpp  -  description
                             -------------------
    begin                : Fri Sep 13 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
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
#include <kapplication.h>
#include <klocale.h>
#include <kdebug.h>

#include <qinputdialog.h>

#include "playlistsplitter.h"
#include "playlist.h"
#include "collectionlist.h"
#include "tageditor.h"

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

void processEvents()
{
    static int processed = 0;
    if(processed == 0)
        kapp->processEvents();
    processed = ( processed + 1 ) % 10;
}

////////////////////////////////////////////////////////////////////////////////
// static public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistSplitter *PlaylistSplitter::splitter = 0;

PlaylistSplitter *PlaylistSplitter::instance()
{
    return(splitter);
}

void PlaylistSplitter::initialize(QWidget *parent)
{
    splitter = new PlaylistSplitter(parent);
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

QString PlaylistSplitter::uniquePlaylistName()
{
    return(uniquePlaylistName(i18n("Playlist")));
}

QString PlaylistSplitter::uniquePlaylistName(const QString &startingWith, bool useParenthesis)
{
    if(!playlistBox)
	return(QString::null);

    QStringList names = playlistBox->names();

    int playlistNumber = 1;

    // while the list contains more than zero instances of the generated 
    // string...

    if(useParenthesis) {
	while(names.contains(startingWith + " (" + QString::number(playlistNumber) + ")") != 0)
	    playlistNumber++;
	
	return(startingWith + " (" + QString::number(playlistNumber) + ")");	
    }
    else
    {
	while(names.contains(startingWith + ' ' + QString::number(playlistNumber)) != 0)
	    playlistNumber++;
	
	return(startingWith + " " + QString::number(playlistNumber));
    }
}

PlaylistItemList PlaylistSplitter::playlistSelection() const
{
    Playlist *p = static_cast<Playlist *>(playlistStack->visibleWidget());
    return(p->selectedItems());
}

PlaylistItem *PlaylistSplitter::playlistFirstItem() const
{
    Playlist *p = static_cast<Playlist *>(playlistStack->visibleWidget());
    PlaylistItem *i = static_cast<PlaylistItem *>(p->firstChild());
    return(i);
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::open()
{
    QStringList files = KFileDialog::getOpenFileNames(QString::null, "*.mp3 *.m3u|Media Files");
    open(files);
}

void PlaylistSplitter::openDirectory()
{
    open(KFileDialog::getExistingDirectory());
}

void PlaylistSplitter::open(const QStringList &files)
{
    Playlist *p = static_cast<Playlist *>(playlistStack->visibleWidget());
    if(p)
	add(files, p);
}

void PlaylistSplitter::open(const QString &file)
{
    Playlist *p = static_cast<Playlist *>(playlistStack->visibleWidget());
    if(p)
	add(file, p);
}

void PlaylistSplitter::save()
{
    if(editor)
	editor->save();
}

void PlaylistSplitter::remove()
{
    Playlist *p = static_cast<Playlist *>(playlistStack->visibleWidget());
    if(p)
	p->remove();
}

void PlaylistSplitter::refresh()
{
    Playlist *p = static_cast<Playlist *>(playlistStack->visibleWidget());
    if(p)
	p->refresh();
}

void PlaylistSplitter::setEditorVisible(bool visible)
{
    if(visible)
	editor->show();
    else
	editor->hide();
}

void PlaylistSplitter::clearSelectedItems()
{
    Playlist *p = static_cast<Playlist *>(playlistStack->visibleWidget());
    if(p)
	p->clearItems(p->selectedItems()); 
}

void PlaylistSplitter::selectAll(bool select)
{
    Playlist *p = static_cast<Playlist *>(playlistStack->visibleWidget());
    if(p)
	p->selectAll(select);
}

Playlist *PlaylistSplitter::createPlaylist()
{
    bool ok;

    // If this text is changed, please also change it in PlaylistBox::duplicate().

    QString name = QInputDialog::getText(i18n("New Playlist..."), i18n("Please enter a name for the new playlist:"),
					 QLineEdit::Normal, uniquePlaylistName(), &ok);
    if(ok)
	return(createPlaylist(name));
    else
	return(0);
}

Playlist *PlaylistSplitter::createPlaylist(const QString &name)
{
    Playlist *p = new Playlist(playlistStack, name.latin1());
    new PlaylistBoxItem(playlistBox, SmallIcon("midi", 32), name, p);
    connect(p, SIGNAL(selectionChanged(const PlaylistItemList &)), editor, SLOT(setItems(const PlaylistItemList &)));
    connect(p, SIGNAL(doubleClicked(QListViewItem *)), this, SIGNAL(playlistDoubleClicked(QListViewItem *)));
    connect(p, SIGNAL(collectionChanged()), editor, SLOT(updateCollection()));
    return(p);
}

void PlaylistSplitter::openPlaylist()
{
    QStringList files = KFileDialog::getOpenFileNames(QString::null, "*.m3u");

    for(QStringList::ConstIterator it = files.begin(); it != files.end(); ++it)
	Playlist *p = openPlaylist(*it);
}

Playlist *PlaylistSplitter::openPlaylist(const QString &playlistFile)
{
    QFileInfo file(playlistFile);
    if(!file.exists() || !file.isFile() || !file.isReadable())
	return(0);
	
    QString name = file.baseName(true);
    Playlist *p = new Playlist(playlistFile, playlistStack, name.latin1());
    connect(p, SIGNAL(selectionChanged(const PlaylistItemList &)), editor, SLOT(setItems(const PlaylistItemList &)));
    connect(p, SIGNAL(doubleClicked(QListViewItem *)), this, SIGNAL(playlistDoubleClicked(QListViewItem *)));
    connect(p, SIGNAL(collectionChanged()), editor, SLOT(updateCollection()));
    new PlaylistBoxItem(playlistBox, SmallIcon("midi", 32), name, p);
    return(p);
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

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

PlaylistSplitter::PlaylistSplitter(QWidget *parent) : QSplitter(Qt::Horizontal, parent, "playlistSplitter")
{
    setupLayout();
    readConfig();
    mediaExtensions.append("mp3");
    listExtensions.append("m3u");
}

PlaylistSplitter::~PlaylistSplitter()
{

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

    connect(playlistBox, SIGNAL(currentChanged(PlaylistBoxItem *)), this, SLOT(changePlaylist(PlaylistBoxItem *)));

    // Create the collection list; this should always exist.  This has a 
    // slightly different creation process than normal playlists (since it in
    // fact is a subclass) so it is created here rather than by using 
    // createPlaylist().

    CollectionList::initialize(playlistStack);
    collection = CollectionList::instance();

    PlaylistBoxItem *collectionBoxItem = new PlaylistBoxItem(playlistBox, SmallIcon("folder_sound", 32), 
							     i18n("Music Collection"), collection);

    connect(collection, SIGNAL(selectionChanged(const PlaylistItemList &)), 
	    editor, SLOT(setItems(const PlaylistItemList &)));
    connect(collection, SIGNAL(doubleClicked(QListViewItem *)), this, SIGNAL(playlistDoubleClicked(QListViewItem *)));
    connect(collection, SIGNAL(collectionChanged()), editor, SLOT(updateCollection()));

    // Show the collection on startup.
    playlistBox->setSelected(collectionBoxItem, true);
}

void PlaylistSplitter::readConfig()
{

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

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::changePlaylist(PlaylistBoxItem *item)
{
    if(item && item->playlist()) {
	playlistStack->raiseWidget(item->playlist());
	editor->setItems(playlistSelection());
	emit(playlistChanged(item->playlist()));
    }
}

#include "playlistsplitter.moc"
