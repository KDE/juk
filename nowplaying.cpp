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

#include <qimage.h>
#include <qlayout.h>

#include "nowplaying.h"
#include "playermanager.h"
#include "coverinfo.h"
#include "tag.h"

static const int imageSize = 64;

struct Line : public QFrame
{
    Line(QWidget *parent) : QFrame(parent) { setFrameShape(VLine); }
};

struct CoverPopup : public QWidget
{
    CoverPopup(const QPixmap &image, int x, int y) : QWidget(0, 0, WX11BypassWM)
    {
        QHBoxLayout *layout = new QHBoxLayout(this);
        QLabel *label = new QLabel(this);
        layout->addWidget(label);

        label->setFrameStyle(QFrame::Box | QFrame::Raised);
        label->setLineWidth(1);

        label->setPixmap(image);
        setGeometry(x - 10, y - 10, label->width(), label->height());
        show();
    }
};

////////////////////////////////////////////////////////////////////////////////
// NowPlaying
////////////////////////////////////////////////////////////////////////////////

NowPlaying::NowPlaying(QWidget *parent, const char *name) :
    QHBox(parent, name)
{
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
    NowPlayingItem(parent),
    m_popup(0)
{
    setFixedHeight(parent->height() - parent->layout()->margin() * 2);
    setFrameStyle(Box | Plain);
    setLineWidth(1);
    setMargin(1);
}

void CoverItem::update(const FileHandle &file)
{
    m_file = file;

    if(file.coverInfo()->hasCover()) {
        show();
        QImage image = file.coverInfo()->pixmap(CoverInfo::FullSize).convertToImage();
        setPixmap(image.smoothScale(imageSize, imageSize, QImage::ScaleMax));
    }
    else
        hide();
}

void CoverItem::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == LeftButton &&
       m_file.coverInfo()->hasCover())
    {
        if(m_popup)
            delete m_popup;

        m_popup = new CoverPopup(m_file.coverInfo()->pixmap(CoverInfo::FullSize),
                                 event->globalX(), event->globalY());
        m_popup->installEventFilter(this);
    }
    
    QLabel::mousePressEvent(event);
}

bool CoverItem::eventFilter(QObject *object, QEvent *event)
{
    if(object == m_popup && (event->type() == QEvent::MouseButtonPress ||
                             event->type() == QEvent::Leave))
    {
        delete m_popup;
        m_popup = 0;
        return true;
    }
        
    return QLabel::eventFilter(object, event);
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

    m_label = new KActiveLabel(this);
    m_label->setLinkUnderline(false);

    layout->addStretch();
    layout->addWidget(m_label);
    layout->addStretch();
}

void TrackItem::update(const FileHandle &file)
{
    QString title  = QStyleSheet::escape(file.tag()->title());
    QString artist = QStyleSheet::escape(file.tag()->artist());
    QString album  = QStyleSheet::escape(file.tag()->album());
    QString separator = (artist.isNull() || album.isNull()) ? QString::null : QString(" - ");

    // This block-o-nastiness makes the font smaller and smaller until it actually fits.

    int size = 4;
    QString format =
        "<font size=\"+%1\"><b>%2</b></font>"
        "<br />"
        "<font size=\"+%3\"><b><a href=\"#\">%4</a>%5<a href=\"#\">%6</a></b></font>";

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
    KActiveLabel(parent, "HistoryItem"),
    NowPlayingItem(parent)
{
    setFixedHeight(parent->height() - parent->layout()->margin() * 2);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    setLinkUnderline(false);
    setText(QString("<b>%1</b><br>").arg(i18n("History")));
}

void HistoryItem::update(const FileHandle &file)
{
    if(file.isNull() || (!m_history.isEmpty() && m_history.front() == file))
        return;

    if(m_history.count() >= 10)
        m_history.remove(m_history.fromLast());

    m_history.prepend(file);
    QString format = "<a href=\"#\"><font size=\"-2\">%1</font></a>";
    QString current = QString("<b>%1</b><br>").arg(i18n("History"));
    QString previous;


    for(FileHandleList::ConstIterator it = m_history.begin();
        it != m_history.end(); ++it)
    {
        previous = current;
        current.append(format.arg(QStyleSheet::escape((*it).tag()->title())));
        setText(current);
        if(heightForWidth(width()) < imageSize)
            current.append("<br>");
        else {
            setText(previous);
            return;
        }
    }
}

#include "nowplaying.moc"
