/***************************************************************************
                          playlistitem.cpp  -  description
                             -------------------
    begin                : Sun Feb 17 2002
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

#include <kstatusbar.h>
#include <kmainwindow.h>
#include <klocale.h>
#include <kdebug.h>
#include <kapplication.h>

#include "playlistitem.h"
#include "filerenamer.h"
#include "collectionlist.h"

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem::~PlaylistItem()
{
    m_data->deleteUser();
}

void PlaylistItem::setFile(const QString &file)
{
    m_data->setFile(file);
    slotRefresh();
}

Tag *PlaylistItem::tag()
{
    return m_data->tag();
}

const Tag *PlaylistItem::tag() const
{
    return m_data->tag();
}

// Some forwarding methods - these can't be inlined because the Data class
// isn't defined yet.

QString PlaylistItem::fileName() const
{
    return m_data->fileInfo()->fileName();
}

QString PlaylistItem::filePath() const
{
    return m_data->fileInfo()->filePath();
}

QString PlaylistItem::absFilePath() const
{
    return m_data->absFilePath();
}

QString PlaylistItem::dirPath(bool absPath) const
{
    return m_data->fileInfo()->dirPath(absPath);
}

bool PlaylistItem::isWritable() const
{
    return m_data->fileInfo()->isWritable();
}

void PlaylistItem::setSelected(bool selected)
{
    static_cast<Playlist *>(listView())->markItemSelected(this, selected);
    KListViewItem::setSelected(selected);
}

void PlaylistItem::guessTagInfo(TagGuesser::Type type)
{
    switch(type) {
    case TagGuesser::FileName:
    {
	TagGuesser guesser(tag()->absFilePath());

	if(!guesser.title().isNull())
	    tag()->setTrack(guesser.title());
	if(!guesser.artist().isNull())
	    tag()->setArtist(guesser.artist());
	if(!guesser.album().isNull())
	    tag()->setAlbum(guesser.album());
	if(!guesser.track().isNull())
	    tag()->setTrackNumber(guesser.track().toInt());
	if(!guesser.comment().isNull())
	    tag()->setComment(guesser.comment());

	tag()->save();
	slotRefresh();
	break;
    }
    case TagGuesser::MusicBrainz:
    {
#if HAVE_MUSICBRAINZ
	MusicBrainzQuery *query = new MusicBrainzQuery(MusicBrainzQuery::File,
						       tag()->absFilePath());
	connect(query, SIGNAL(signalDone(const MusicBrainzQuery::TrackList &)),
		SLOT(slotTagGuessResults(const MusicBrainzQuery::TrackList &)));
	KMainWindow *win = static_cast<KMainWindow *>(kapp->mainWidget());
	connect(query, SIGNAL(signalStatusMsg(const QString &, int)),
		win->statusBar(), SLOT(message(const QString &, int)));
	query->start();
#endif //add message box telling users musicbrainz is not installed or keep it quiet?
	break;
    }
    }
}

void PlaylistItem::renameFile()
{
    FileRenamer renamer;
    renamer.rename(this);
    slotRefresh();
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistItem::slotRefresh()
{
    // This signal will be received by the "parent" CollectionListItem which will
    // in turn call slotRefreshImpl() for all of its children, including this item.
    emit(signalRefreshed());
}

void PlaylistItem::slotRefreshFromDisk()
{
    m_data->refresh();
    slotRefresh();
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem protected methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem::PlaylistItem(CollectionListItem *item, Playlist *parent) :
    QObject(parent), KListViewItem(parent),
    m_playing(false)
{
    setup(item, parent);
}

PlaylistItem::PlaylistItem(CollectionListItem *item, Playlist *parent, QListViewItem *after) :
    QObject(parent), KListViewItem(parent, after),
    m_playing(false)
{
    setup(item, parent);
}


// This constructor should only be used by the CollectionList subclass.
PlaylistItem::PlaylistItem(CollectionList *parent) :
    QObject(parent), KListViewItem(parent),
    m_collectionItem(static_cast<CollectionListItem *>(this)), m_data(0), m_playing(false)
{
    setDragEnabled(true);
}

void PlaylistItem::paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int align)
{
    if(!m_playing)
	return KListViewItem::paintCell(p, cg, column, width, align);

    QColorGroup colorGroup = cg;

    QColor base = colorGroup.base();
    QColor selection = colorGroup.highlight();

    int r = (base.red() + selection.red()) / 2;
    int b = (base.blue() + selection.blue()) / 2;
    int g = (base.green() + selection.green()) / 2;

    QColor c(r, g, b);

    colorGroup.setColor(QColorGroup::Base, c);
    QListViewItem::paintCell(p, colorGroup, column, width, align);
}

int PlaylistItem::compare(QListViewItem *item, int column, bool ascending) const
{
    // reimplemented from QListViewItem

    if(!item)
	return 0;

    PlaylistItem *playlistItem = static_cast<PlaylistItem *>(item);

    // The following statments first check to see if you can sort based on the
    // specified column.  If the values for the two PlaylistItems are the same
    // in that column it then trys to sort based on columns 1, 2, 3 and 0,
    // (artist, album, track number, track name) in that order.

    int c = compare(this, playlistItem, column, ascending);

    if(c != 0)
	return c;
    else {

	// Loop through the columns doing comparisons until something is differnt.
	// If all else is the same, compare the track name.

	Playlist *p = static_cast<Playlist *>(listView());
	int last = p->isColumnVisible(AlbumColumn) ? TrackNumberColumn : ArtistColumn;

	for(int i = ArtistColumn; i <= last; i++) {
	    if(p->isColumnVisible(i)) {
		c = compare(this, playlistItem, i, ascending);
		if(c != 0)
		    return c;
	    }
	}
	return compare(this, playlistItem, TrackColumn, ascending);
    }
}

int PlaylistItem::compare(const PlaylistItem *firstItem, const PlaylistItem *secondItem, int column, bool) const
{
    if(column == TrackNumberColumn) {
        if(firstItem->tag()->trackNumber() > secondItem->tag()->trackNumber())
            return 1;
        else if(firstItem->tag()->trackNumber() < secondItem->tag()->trackNumber())
            return -1;
        else
            return 0;
    }
    else if(column == LengthColumn) {
        if(firstItem->tag()->seconds() > secondItem->tag()->seconds())
            return 1;
        else if(firstItem->tag()->seconds() < secondItem->tag()->seconds())
            return -1;
        else
            return 0;
    }
    else
	return strcoll(firstItem->data()->local8BitLower(column),
		       secondItem->data()->local8BitLower(column));
}

bool PlaylistItem::isValid() const
{
    return m_data && m_data->tag();
}


////////////////////////////////////////////////////////////////////////////////
// PlaylistItem protected slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistItem::slotRefreshImpl()
{
    // This should be the only function that needs to be rewritten if the structure of
    // PlaylistItemData changes.

    int offset = static_cast<Playlist *>(listView())->columnOffset();
    

    setText(TrackColumn + offset,       tag()->track());
    setText(ArtistColumn + offset,      tag()->artist());
    setText(AlbumColumn + offset,       tag()->album());
    setText(TrackNumberColumn + offset, tag()->trackNumberString());
    setText(GenreColumn + offset,       tag()->genre().name());
    setText(YearColumn + offset,        tag()->yearString());
    setText(LengthColumn + offset,      tag()->lengthString());
    setText(FileNameColumn + offset,    filePath());

    QString shortComment = tag()->comment().simplifyWhiteSpace();
    if(shortComment.length() > 50)
	shortComment = shortComment.left(47) + "...";

    setText(CommentColumn + offset,     shortComment);

    int columns = listView()->columns();
    m_data->setColumns(columns);
    for(int i = 0; i < columns; i++)
	m_data->setLocal8BitLower(i, text(i).lower().local8Bit());
}

void PlaylistItem::slotTagGuessResults(const MusicBrainzQuery::TrackList &res)
{
#if HAVE_MUSICBRAINZ
    //FIXME:GUI to pick one of the results

    KMainWindow *win = static_cast<KMainWindow *>(kapp->mainWidget());

    if(res.count() == 0)
    {
        win->statusBar()->message(i18n("No matches found."), 2000);
        return;
    }
    MusicBrainzQuery::Track track = res.first();

    if(!track.name.isEmpty())
        tag()->setTrack(track.name);
    if(!track.artist.isEmpty())
        tag()->setArtist(track.artist);
    if(!track.album.isEmpty())
        tag()->setAlbum(track.album);
    if(!track.number)
        tag()->setTrackNumber(track.number);

    tag()->save();
    slotRefresh();

    win->statusBar()->message(i18n("Done."), 2000);
#else
    Q_UNUSED(res)
#endif
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem private methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistItem::setup(CollectionListItem *item, Playlist *parent)
{
    m_collectionItem = item;

    if(item) {
	m_data = item->data()->newUser();
	item->addChildItem(this);
	slotRefreshImpl();
	connect(this, SIGNAL(signalRefreshed()), parent, SIGNAL(signalDataChanged()));
    }

    setDragEnabled(true);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem::Data public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem::Data *PlaylistItem::Data::newUser(const QFileInfo &file, const QString &path)
{
    return new Data(file, path);
}

PlaylistItem::Data *PlaylistItem::Data::newUser()
{
    m_referenceCount++;
    return this;
}

void PlaylistItem::Data::refresh()
{
    m_fileInfo.refresh();
    delete m_dataTag;
    m_dataTag = Tag::createTag(m_fileInfo.filePath());
    Q_ASSERT(m_dataTag);
    m_absFileName = m_fileInfo.absFilePath();
}

void PlaylistItem::Data::deleteUser()
{
    // The delete this is safe because we control object creation through a
    // protected constructor and the newUser() methods.

    if(--m_referenceCount == 0)
        delete this;
}

Tag *PlaylistItem::Data::tag()
{
    return m_dataTag;
}

const Tag *PlaylistItem::Data::tag() const
{
    return m_dataTag;
}

void PlaylistItem::Data::setFile(const QString &file)
{
    m_fileInfo.setFile(file);
    refresh();
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem::Data protected methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem::Data::Data(const QFileInfo &file, const QString &path) :
    m_fileInfo(file),
    m_referenceCount(1),
    m_absFileName(path)
{
    m_dataTag = Tag::createTag(path);
}

PlaylistItem::Data::~Data()
{
    delete m_dataTag;
}

#include "playlistitem.moc"
