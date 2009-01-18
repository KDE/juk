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

#include "nowplaying.h"

#include <kiconloader.h>
#include <klocale.h>
#include <kdebug.h>
#include <krandom.h>
#include <kglobalsettings.h>
#include <kio/netaccess.h>

#include <QImage>
#include <QLayout>
#include <QEvent>
#include <QDrag>
#include <QTimer>
#include <QPoint>
#include <QFrame>
#include <QDropEvent>
#include <QLabel>
#include <QVBoxLayout>
#include <QDragEnterEvent>
#include <QMouseEvent>
#include <QUrl>
#include <QList>
#include <QTextDocument>

#include "playlistcollection.h"
#include "playlistitem.h"
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

NowPlaying::NowPlaying(QWidget *parent, PlaylistCollection *collection) :
    QWidget(parent),
    m_observer(this, collection),
    m_collection(collection)
{
    setObjectName("NowPlaying");

    // m_observer is set to watch the PlaylistCollection, also watch for
    // changes that come from CollectionList.

    CollectionList::instance()->addObserver(&m_observer);

    QHBoxLayout *layout = new QHBoxLayout(this);
    setLayout(layout);

    layout->setMargin(0);
    layout->setSpacing(3);
    setFixedHeight(imageSize + 2);

    layout->addWidget(new CoverItem(this), 0);
    layout->addWidget(new TrackItem(this), 2);
    layout->addWidget(new Line(this), 0);
    layout->addWidget(new HistoryItem(this), 1);

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

    foreach(NowPlayingItem *item, m_items)
        item->update(file);
}

////////////////////////////////////////////////////////////////////////////////
// CoverItem
////////////////////////////////////////////////////////////////////////////////

CoverItem::CoverItem(NowPlaying *parent) :
    QLabel(parent),
    NowPlayingItem(parent)
{
    setObjectName("CoverItem");
    setFixedHeight(parent->height() - parent->layout()->margin() * 2);
    setMargin(1);
    setAcceptDrops(true);
}

void CoverItem::update(const FileHandle &file)
{
    m_file = file;

    if(file.coverInfo()->hasCover()) {
        show();
        setPixmap(
	    file.coverInfo()->pixmap(CoverInfo::Thumbnail)
	    .scaled(imageSize, imageSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
       event->button() == Qt::LeftButton &&
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
    if(diff.manhattanLength() > KGlobalSettings::dndEventDelay()) {

        // Start a drag.

        m_dragging = true;

        QDrag *drag = new QDrag(this);
        CoverDrag *data = new CoverDrag(m_file.coverInfo()->coverId());

        drag->setMimeData(data);
        drag->exec(Qt::CopyAction);
    }
}

void CoverItem::dragEnterEvent(QDragEnterEvent *e)
{
    e->setAccepted(CoverDrag::isCover(e->mimeData()) || e->mimeData()->hasUrls());
}

void CoverItem::dropEvent(QDropEvent *e)
{
    QImage image;
    QList<QUrl> urls;
    coverKey key;

    if(e->source() == this)
        return;

    key = CoverDrag::idFromData(e->mimeData());
    if(key != CoverManager::NoMatch) {
        m_file.coverInfo()->setCoverId(key);
        update(m_file);
    }
    else if(e->mimeData()->hasImage()) {
        m_file.coverInfo()->setCover(qvariant_cast<QImage>(e->mimeData()->imageData()));
        update(m_file);
    }
    else {
        urls = e->mimeData()->urls();
        if(urls.isEmpty())
            return;

        QString fileName;

        if(KIO::NetAccess::download(urls.front(), fileName, this)) {
            if(image.load(fileName)) {
                m_file.coverInfo()->setCover(image);
                update(m_file);
            }
            else
                kError(65432) << "Unable to load image from " << urls.front() << endl;

            KIO::NetAccess::removeTempFile(fileName);
        }
        else
            kError(65432) << "Unable to download " << urls.front() << endl;
    }
}

////////////////////////////////////////////////////////////////////////////////
// TrackItem
////////////////////////////////////////////////////////////////////////////////

TrackItem::TrackItem(NowPlaying *parent) :
    QWidget(parent),
    NowPlayingItem(parent)
{
    setObjectName("TrackItem");
    setFixedHeight(parent->height() - parent->layout()->margin() * 2);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    QVBoxLayout *layout = new QVBoxLayout(this);

    m_label = new QLabel(this);
    m_label->setWordWrap(true);

    m_label->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::LinksAccessibleByKeyboard);

#ifdef __GNUC__
    #warning We should potentially check on URL underlining.
#endif
    /* m_label->setLinkUnderline(false); */

    layout->addStretch();
    layout->addWidget(m_label);
    layout->addStretch();

    connect(m_label, SIGNAL(linkActivated(const QString &)), this,
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
    QString title  = Qt::escape(m_file.tag()->title());
    QString artist = Qt::escape(m_file.tag()->artist());
    QString album  = Qt::escape(m_file.tag()->album());
    QString separator = (artist.isNull() || album.isNull()) ? QString::null : QString(" - ");	//krazy:exclude=nullstrassign for old broken gcc

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

    m_label->setFixedHeight(qMin(imageSize, m_label->heightForWidth(m_label->width())));
}

////////////////////////////////////////////////////////////////////////////////
// HistoryItem
////////////////////////////////////////////////////////////////////////////////

HistoryItem::HistoryItem(NowPlaying *parent) :
    QLabel(parent),
    NowPlayingItem(parent)
{
    setFixedHeight(parent->height() - parent->layout()->margin() * 2);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    /* setLinkUnderline(false); */
    setText(QString("<b>%1</b>").arg(i18n("History")));

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(slotAddPlaying()));
}

void HistoryItem::update(const FileHandle &file)
{
    if(file.isNull() || (!m_history.isEmpty() && m_history.front().file == file))
        return;

    if(m_history.count() >= 10)
        m_history.removeLast();

    QString format = "<br /><a href=\"%1\"><font size=\"-1\">%2</font></a>";
    QString current = QString("<b>%1</b>").arg(i18n("History"));
    QString previous;

    foreach(const Item &historyItem, m_history) {
        previous = current;
        current.append(format.arg(historyItem.anchor).arg(Qt::escape(historyItem.file.tag()->title())));
        setText(current);
        if(heightForWidth(width()) > imageSize) {
            setText(previous);
            break;
        }
    }

    m_file = file;
    m_timer->stop();
    m_timer->setSingleShot(true);
    m_timer->start(HistoryPlaylist::delay());
}

void HistoryItem::openLink(const QString &link)
{
    foreach(const Item &historyItem, m_history) {
        if(historyItem.anchor == link) {
            if(historyItem.playlist) {
                CollectionListItem *collectionItem =
                    CollectionList::instance()->lookup(historyItem.file.absFilePath());
                PlaylistItem *item = collectionItem->itemForPlaylist(historyItem.playlist);
                historyItem.playlist->clearSelection();
                historyItem.playlist->setSelected(item, true);
                historyItem.playlist->ensureItemVisible(item);
                NowPlayingItem::parent()->collection()->raise(historyItem.playlist);
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
        m_history.prepend(Item(KRandom::randomString(20),
                               m_file, Playlist::playingItem()->playlist()));
    }

    m_file = FileHandle::null();
}

#include "nowplaying.moc"

// vim: set et sw=4 tw=0 sta:
