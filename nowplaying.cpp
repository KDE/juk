/***************************************************************************
    begin                : Tue Nov 9 2004
    copyright            : (C) 2004 by Scott Wheeler
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

#include <kiconloader.h>
#include <klocale.h>
#include <kdebug.h>
#include <kurldrag.h>
#include <kio/netaccess.h>

#include <qimage.h>
#include <qlayout.h>
#include <qevent.h>
#include <qdragobject.h>
#include <qimage.h>
#include <qtimer.h>
#include <qpoint.h>

#include "nowplaying.h"
#include "playlistcollection.h"
#include "playermanager.h"
#include "coverinfo.h"
#include "covermanager.h"
#include "tag.h"
#include "playlistitem.h"
#include "collectionlist.h"
#include "historyplaylist.h"

static const int imageSize = 64;

struct Line : public QFrame
{
    Line(QWidget *parent) : QFrame(parent) { setFrameShape(VLine); }
};

////////////////////////////////////////////////////////////////////////////////
// NowPlaying
////////////////////////////////////////////////////////////////////////////////

NowPlaying::NowPlaying(QWidget *parent, PlaylistCollection *collection, const char *name) :
    QHBox(parent, name),
    m_observer(this, collection),
    m_collection(collection)
{
    // m_observer is set to watch the PlaylistCollection, also watch for
    // changes that come from CollectionList.

    CollectionList::instance()->addObserver(&m_observer);

    layout()->setMargin(5);
    layout()->setSpacing(3);
    setFixedHeight(imageSize + 2 + layout()->margin() * 2);

    setStretchFactor(new CoverItem(this), 0);
    setStretchFactor(new TrackItem(this), 2);
    setStretchFactor(new Line(this), 0);
    setStretchFactor(new HistoryItem(this), 1);

    connect(PlayerManager::instance(), SIGNAL(signalPlay()), this, SLOT(slotUpdate()));
    connect(PlayerManager::instance(), SIGNAL(signalStop()), this, SLOT(slotUpdate()));

    hide();
}

void NowPlaying::addItem(NowPlayingItem *item)
{
    m_items.append(item);
}

PlaylistCollection *NowPlaying::collection() const
{
    return m_collection;
}

void NowPlaying::slotUpdate()
{
    FileHandle file = PlayerManager::instance()->playingFile();

    if(file.isNull()) {
        hide();
        return;
    }
    else
        show();

    for(QValueList<NowPlayingItem *>::Iterator it = m_items.begin();
        it != m_items.end(); ++it)
    {
        (*it)->update(file);
    }
}

////////////////////////////////////////////////////////////////////////////////
// CoverItem
////////////////////////////////////////////////////////////////////////////////

CoverItem::CoverItem(NowPlaying *parent) :
    QLabel(parent, "CoverItem"),
    NowPlayingItem(parent)
{
    setFixedHeight(parent->height() - parent->layout()->margin() * 2);
    setFrameStyle(Box | Plain);
    setLineWidth(1);
    setMargin(1);
    setAcceptDrops(true);
}

void CoverItem::update(const FileHandle &file)
{
    m_file = file;

    if(file.coverInfo()->hasCover()) {
        show();
        QImage image = file.coverInfo()->pixmap(CoverInfo::Thumbnail).convertToImage();
        setPixmap(image.smoothScale(imageSize, imageSize, QImage::ScaleMin));
    }
    else
        hide();
}

void CoverItem::mouseReleaseEvent(QMouseEvent *event)
{
    if(m_dragging) {
        m_dragging = false;
        return;
    }

    if(event->x() >= 0 && event->y() >= 0 &&
       event->x() < width() && event->y() < height() &&
       event->button() == LeftButton &&
       m_file.coverInfo()->hasCover())
    {
        m_file.coverInfo()->popup();
    }
    
    QLabel::mousePressEvent(event);
}

void CoverItem::mousePressEvent(QMouseEvent *e)
{
    m_dragging = false;
    m_dragStart = e->globalPos();
}

void CoverItem::mouseMoveEvent(QMouseEvent *e)
{
    if(m_dragging)
        return;

    QPoint diff = m_dragStart - e->globalPos();
    if(QABS(diff.x()) > 1 || QABS(diff.y()) > 1) {

        // Start a drag.

        m_dragging = true;

        CoverDrag *drag = new CoverDrag(m_file.coverInfo()->coverId(), this);
        drag->drag();
    }
}

void CoverItem::dragEnterEvent(QDragEnterEvent *e)
{
    e->accept(QImageDrag::canDecode(e) || KURLDrag::canDecode(e) || CoverDrag::canDecode(e));
}

void CoverItem::dropEvent(QDropEvent *e)
{
    QImage image;
    KURL::List urls;
    coverKey key;

    if(e->source() == this)
        return;

    if(QImageDrag::decode(e, image)) {
        m_file.coverInfo()->setCover(image);
        update(m_file);
    }
    else if(CoverDrag::decode(e, key)) {
        m_file.coverInfo()->setCoverId(key);
        update(m_file);
    }
    else if(KURLDrag::decode(e, urls)) {
        QString fileName;

        if(KIO::NetAccess::download(urls.front(), fileName, this)) {
            if(image.load(fileName)) {
                m_file.coverInfo()->setCover(image);
                update(m_file);
            }
            else
                kdError(65432) << "Unable to load image from " << urls.front() << endl;

            KIO::NetAccess::removeTempFile(fileName);
        }
        else
            kdError(65432) << "Unable to download " << urls.front() << endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
// TrackItem
////////////////////////////////////////////////////////////////////////////////

TrackItem::TrackItem(NowPlaying *parent) :
    QWidget(parent, "TrackItem"),
    NowPlayingItem(parent)
{
    setFixedHeight(parent->height() - parent->layout()->margin() * 2);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QVBoxLayout *layout = new QVBoxLayout(this);

    m_label = new LinkLabel(this);
    m_label->setLinkUnderline(false);

    layout->addStretch();
    layout->addWidget(m_label);
    layout->addStretch();

    connect(m_label, SIGNAL(linkClicked(const QString &)), this,
            SLOT(slotOpenLink(const QString &)));
}

void TrackItem::update(const FileHandle &file)
{
    m_file = file;
    QTimer::singleShot(0, this, SLOT(slotUpdate()));
}

void TrackItem::slotOpenLink(const QString &link)
{
    PlaylistCollection *collection = NowPlayingItem::parent()->collection();

    if(link == "artist")
        collection->showMore(m_file.tag()->artist());
    else if(link == "album")
        collection->showMore(m_file.tag()->artist(), m_file.tag()->album());
    else if(link == "clear")
        collection->clearShowMore();

    update(m_file);
}

void TrackItem::slotUpdate()
{
    QString title  = QStyleSheet::escape(m_file.tag()->title());
    QString artist = QStyleSheet::escape(m_file.tag()->artist());
    QString album  = QStyleSheet::escape(m_file.tag()->album());
    QString separator = (artist.isNull() || album.isNull()) ? QString::null : QString(" - ");

    // This block-o-nastiness makes the font smaller and smaller until it actually fits.

    int size = 4;
    QString format =
        "<font size=\"+%1\"><b>%2</b></font>"
        "<br />"
        "<font size=\"+%3\"><b><a href=\"artist\">%4</a>%5<a href=\"album\">%6</a></b>";

    if(NowPlayingItem::parent()->collection()->showMoreActive())
        format.append(QString(" (<a href=\"clear\">%1</a>)").arg(i18n("back to playlist")));

    format.append("</font>");

    do {
        m_label->setText(format.arg(size).arg(title).arg(size - 2)
                         .arg(artist).arg(separator).arg(album));
        --size;
    } while(m_label->heightForWidth(m_label->width()) > imageSize && size >= 0);

    m_label->setFixedHeight(QMIN(imageSize, m_label->heightForWidth(m_label->width())));
}

////////////////////////////////////////////////////////////////////////////////
// HistoryItem
////////////////////////////////////////////////////////////////////////////////

HistoryItem::HistoryItem(NowPlaying *parent) :
    LinkLabel(parent, "HistoryItem"),
    NowPlayingItem(parent)
{
    setFixedHeight(parent->height() - parent->layout()->margin() * 2);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setLinkUnderline(false);
    setText(QString("<b>%1</b>").arg(i18n("History")));

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(slotAddPlaying()));
}

void HistoryItem::update(const FileHandle &file)
{
    if(file.isNull() || (!m_history.isEmpty() && m_history.front().file == file))
        return;

    if(m_history.count() >= 10)
        m_history.remove(m_history.fromLast());

    QString format = "<br /><a href=\"%1\"><font size=\"-1\">%2</font></a>";
    QString current = QString("<b>%1</b>").arg(i18n("History"));
    QString previous;

    for(QValueList<Item>::ConstIterator it = m_history.begin();
        it != m_history.end(); ++it)
    {
        previous = current;
        current.append(format.arg((*it).anchor).arg(QStyleSheet::escape((*it).file.tag()->title())));
        setText(current);
        if(heightForWidth(width()) > imageSize) {
            setText(previous);
            break;
        }
    }

    m_file = file;
    m_timer->stop();
    m_timer->start(HistoryPlaylist::delay(), true);
}

void HistoryItem::openLink(const QString &link)
{
    for(QValueList<Item>::ConstIterator it = m_history.begin();
        it != m_history.end(); ++it)
    {
        if((*it).anchor == link) {
            if((*it).playlist) {
                CollectionListItem *collectionItem = 
                    CollectionList::instance()->lookup((*it).file.absFilePath());
                PlaylistItem *item = collectionItem->itemForPlaylist((*it).playlist);
                (*it).playlist->clearSelection();
                (*it).playlist->setSelected(item, true);
                (*it).playlist->ensureItemVisible(item);
                NowPlayingItem::parent()->collection()->raise((*it).playlist);
            }
            break;
        }
    }
}

void HistoryItem::slotAddPlaying()
{
    // More or less copied from the HistoryPlaylist

    PlayerManager *manager = PlayerManager::instance();

    if(manager->playing() && manager->playingFile() == m_file) {
        m_history.prepend(Item(KApplication::randomString(20),
                               m_file, Playlist::playingItem()->playlist()));
    }

    m_file = FileHandle::null();
}

#include "nowplaying.moc"

// vim: set et sw=4 ts=8:
