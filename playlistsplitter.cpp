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
#include <kdebug.h>

#include "playlistsplitter.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistSplitter::PlaylistSplitter(QWidget *parent, const char *name) : QSplitter(Qt::Horizontal, parent, name)
{
    setupLayout();
    readConfig();
}

PlaylistSplitter::~PlaylistSplitter()
{

}

void PlaylistSplitter::createPlaylist(const QString &name)
{
    Playlist *p = new Playlist(playlistStack, name.latin1());
    (void) new PlaylistBoxItem(playlistBox, SmallIcon("midi", 32), name, p);
    connect(p, SIGNAL(selectionChanged(const QPtrList<PlaylistItem> &)), editor, SLOT(setItems(const QPtrList<PlaylistItem> &)));
    connect(p, SIGNAL(doubleClicked(QListViewItem *)), this, SIGNAL(playlistDoubleClicked(QListViewItem *)));
}

QPtrList<PlaylistItem> PlaylistSplitter::playlistSelection() const
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

void PlaylistSplitter::open(const QStringList &files)
{
    collection->append(files);

    // If the collection is not the top widget in the widget stack, assume that 
    // the open was intended to go to the top widget and open the files there 
    // too.

    // The open methods are currently causing segfaults.  I haven't tried to debug
    // them yet.

    if(playlistStack->visibleWidget() != collection) {
	Playlist *p = static_cast<Playlist *>(playlistStack->visibleWidget());
	p->append(files);
    }
}

void PlaylistSplitter::open(const QString &file)
{
    collection->append(file);

    // If the collection is not the top widget in the widget stack, assume that 
    // the open was intended to go to the top widget and open the files there 
    // too.


    if(playlistStack->visibleWidget() != collection) {
	Playlist *p = static_cast<Playlist *>(playlistStack->visibleWidget());
	p->append(file);
    }
}

void PlaylistSplitter::remove()
{
    Playlist *p = static_cast<Playlist *>(playlistStack->visibleWidget());
    p->remove();
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
    p->clearItems(p->selectedItems()); 
}

void PlaylistSplitter::selectAll(bool select)
{
    Playlist *p = static_cast<Playlist *>(playlistStack->visibleWidget());
    p->selectAll(select);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::setupLayout()
{
    setOpaqueResize();
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

    collection = new CollectionList(playlistStack, "collectionList");
    PlaylistBoxItem *collectionBoxItem = new PlaylistBoxItem(playlistBox, SmallIcon("folder_sound", 32), i18n("Music Collection"), collection);
    connect(collection, SIGNAL(selectionChanged(const QPtrList<PlaylistItem> &)), editor, SLOT(setItems(const QPtrList<PlaylistItem> &)));
    connect(collection, SIGNAL(doubleClicked(QListViewItem *)), this, SIGNAL(playlistDoubleClicked(QListViewItem *)));

    // Show the collection on startup.
    playlistBox->setSelected(collectionBoxItem, true);

    // just for testing -- until I get the dialog/menu entry for adding playlists
    createPlaylist(i18n("Playlist 1"));
}

void PlaylistSplitter::readConfig()
{
    // This should eventually be replace by a KConfig entry.
    showEditor = true;

    setEditorVisible(showEditor);
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistSplitter::changePlaylist(PlaylistBoxItem *item)
{
    if(item && item->playlist()) {
	playlistStack->raiseWidget(item->playlist());
	editor->setItems(playlistSelection());
    }
}

#include "playlistsplitter.moc"
