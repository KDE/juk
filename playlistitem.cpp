/***************************************************************************
                          playlistitem.cpp  -  description
                             -------------------
    begin                : Sun Feb 17 2002
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

#include <kstatusbar.h>
#include <kmainwindow.h>
#include <klocale.h>
#include <kdebug.h>

#include "playlistitem.h"
#include "collectionlist.h"
#include "trackpickerdialog.h"
#include "stringshare.h"

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistItem::~PlaylistItem()
{
    emit signalAboutToDelete();
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

QString PlaylistItem::text(int column) const
{
    if(!m_data->tag())
	return QString::null;

    int offset = static_cast<Playlist *>(listView())->columnOffset();

    switch(column - offset) {
    case TrackColumn:
	return m_data->tag()->title();
    case ArtistColumn:
	return m_data->tag()->artist();
    case AlbumColumn:
	return m_data->tag()->album();
    case TrackNumberColumn:
	return m_data->tag()->track() > 0
	    ? QString::number(m_data->tag()->track())
	    : QString::null;
    case GenreColumn:
	return m_data->tag()->genre();
    case YearColumn:
	return m_data->tag()->year() > 0 
	    ? QString::number(m_data->tag()->year())
	    : QString::null;
    case LengthColumn:
	return m_data->tag()->lengthString();
    case CommentColumn:
	return m_data->tag()->comment();
    case FileNameColumn:
	return m_data->tag()->fileName();
    default:
	return KListViewItem::text(column);
    }
}

void PlaylistItem::setText(int column, const QString &text)
{
    int offset = static_cast<Playlist *>(listView())->columnOffset();
    if(column - offset >= 0 && column + offset <= lastColumn()) {
	KListViewItem::setText(column, QString::null);
	return;
    }

    KListViewItem::setText(column, text);
    emit signalColumnWidthChanged(column);
}

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
	TagGuesser guesser(tag()->fileName());

	if(!guesser.title().isNull())
	    tag()->setTitle(guesser.title());
	if(!guesser.artist().isNull())
	    tag()->setArtist(guesser.artist());
	if(!guesser.album().isNull())
	    tag()->setAlbum(guesser.album());
	if(!guesser.track().isNull())
	    tag()->setTrack(guesser.track().toInt());
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
						       tag()->fileName());
	connect(query, SIGNAL(signalDone(const MusicBrainzQuery::TrackList &)),
		SLOT(slotTagGuessResults(const MusicBrainzQuery::TrackList &)));
	KMainWindow *win = dynamic_cast<KMainWindow *>(kapp->mainWidget());
	if(win)
	    connect(query, SIGNAL(signalStatusMsg(const QString &, int)),
		    win->statusBar(), SLOT(message(const QString &, int)));
	else
	    kdWarning(65432) << "Could not find the main window." << endl;

	query->start();
#endif //add message box telling users musicbrainz is not installed or keep it quiet?
	break;
    }
    }
}

Playlist *PlaylistItem::playlist() const
{
    return static_cast<Playlist *>(listView());
}

QValueVector<int> PlaylistItem::cachedWidths() const
{
    return m_data->cachedWidths();
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistItem public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistItem::slotRefresh()
{
    // This signal will be received by the "parent" CollectionListItem which will
    // in turn call slotRefreshImpl() for all of its children, including this item.

    emit signalRefreshed();
}

void PlaylistItem::slotRefreshFromDisk()
{
    m_data->refresh();
    slotRefresh();
}

void PlaylistItem::slotClear()
{
    // deleteLater();
    static_cast<Playlist *>(listView())->clearItem(this);
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

    int offset = static_cast<Playlist *>(listView())->columnOffset();

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
	int last = p->isColumnVisible(AlbumColumn + offset) ? TrackNumberColumn : ArtistColumn;

	for(int i = ArtistColumn; i <= last; i++) {
	    if(p->isColumnVisible(i + offset)) {
		c = compare(this, playlistItem, i, ascending);
		if(c != 0)
		    return c;
	    }
	}
	return compare(this, playlistItem, TrackColumn + offset, ascending);
    }
}

int PlaylistItem::compare(const PlaylistItem *firstItem, const PlaylistItem *secondItem, int column, bool) const
{
    int offset = static_cast<Playlist *>(listView())->columnOffset();

    if(column < offset) {
	QString first = firstItem->text(column).lower();
	QString second = secondItem->text(column).lower();
	return first.localeAwareCompare(second);
    }

    if(column == TrackNumberColumn + offset) {
        if(firstItem->tag()->track() > secondItem->tag()->track())
            return 1;
        else if(firstItem->tag()->track() < secondItem->tag()->track())
            return -1;
        else
            return 0;
    }
    else if(column == LengthColumn + offset) {
        if(firstItem->tag()->seconds() > secondItem->tag()->seconds())
            return 1;
        else if(firstItem->tag()->seconds() < secondItem->tag()->seconds())
            return -1;
        else
            return 0;
    }
    else
	return strcoll(firstItem->data()->local8BitLower(column - offset),
		       secondItem->data()->local8BitLower(column - offset));
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
    int offset = static_cast<Playlist *>(listView())->columnOffset();
    int columns = lastColumn() + offset + 1;
    m_data->setColumns(columns);

    for(int i = offset; i < columns; i++) {
	int id = i - offset;
	if(id != TrackNumberColumn && id != LengthColumn)
	{        
	    // All columns other than track num and length need local-encoded data for sorting        

	    QCString lower = text(i).lower().local8Bit();

	    // For some columns, we may be able to share some strings

	    if((id == ArtistColumn) || (id == AlbumColumn) ||
	       (id == GenreColumn)  || (id == YearColumn)  ||
	       (id == CommentColumn))
	    {
		lower = StringShare::tryShare(lower);
	    }
	    m_data->setLocal8BitLower(id, lower);
	}

	int newWidth = width(listView()->fontMetrics(), listView(), i);
	m_data->setCachedWidth(i, newWidth);

	if(newWidth != m_data->cachedWidth(i))
	    emit signalColumnWidthChanged(i);
    }

    repaint();
}

void PlaylistItem::slotTagGuessResults(const MusicBrainzQuery::TrackList &res)
{
#if HAVE_MUSICBRAINZ

    KMainWindow *win = dynamic_cast<KMainWindow *>(kapp->mainWidget());

    if(win && res.isEmpty()) {
        win->statusBar()->message(i18n("No matches found."), 2000);
        return;
    }

    TrackPickerDialog *trackPicker = new TrackPickerDialog(fileName(), res, win);

    if(win && trackPicker->exec() != QDialog::Accepted) {
	win->statusBar()->message(i18n("Canceled."), 2000);
	return;
    }

    MusicBrainzQuery::Track track = trackPicker->selectedTrack();

    if(!track.name.isEmpty())
        tag()->setTitle(track.name);
    if(!track.artist.isEmpty())
        tag()->setArtist(track.artist);
    if(!track.album.isEmpty())
        tag()->setAlbum(track.album);
    if(track.number)
        tag()->setTrack(track.number);

    tag()->save();
    slotRefresh();

    if(win)
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

    // We only want this connection to take effect for changes after item
    // creation -- i.e. editing the item.  We'll handle item creation separately
    // as that avoids this signal firing a few thousand times.

    connect(this, SIGNAL(signalColumnWidthChanged(int)), parent, SLOT(slotWeightDirty(int)));
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

void PlaylistItem::Data::setColumns(int columns)
{
    m_local8Bit.resize(columns);
    m_cachedWidths.resize(columns, -1);
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
