/***************************************************************************
                          statuslabel.cpp  -  description
                             -------------------
    begin                : Fri Oct 18 2002
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

#include <kpushbutton.h>
#include <kiconloader.h>
#include <kdebug.h>

#include <qevent.h>
#include <qstylesheet.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qfontmetrics.h>


#include "statuslabel.h"
#include "playlistitem.h"
#include "playlist.h"
#include "playlistbox.h"
#include "playlistsplitter.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

StatusLabel::StatusLabel(QWidget *parent, const char *name) : QHBox(parent, name), playingItem(0)
{
    setSpacing(0);

    QHBox *trackAndPlaylist = new QHBox(this);
    trackAndPlaylist->setFrameStyle(Box | Sunken);

    playlistLabel = new QLabel(trackAndPlaylist, "playlistLabel");
    playlistLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    playlistLabel->setTextFormat(RichText);
    playlistLabel->setAlignment(AlignLeft);

    trackLabel = new QLabel(trackAndPlaylist, "trackLabel");
    trackLabel->setAlignment(AlignRight);
    trackLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    trackLabel->setTextFormat(RichText);

    itemTimeLabel = new QLabel(this);
    QFontMetrics fontMetrics(font());
    itemTimeLabel->setAlignment(AlignCenter);
    itemTimeLabel->setMinimumWidth(fontMetrics.boundingRect("000:00 / 000:00").width());
    itemTimeLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    itemTimeLabel->setFrameStyle(Box | Sunken);

    setItemTotalTime(0);
    setItemCurrentTime(0);
  
    QLabel *iconLabel = new QLabel(this);
    iconLabel->setPixmap(SmallIcon("up"));
    iconLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
    iconLabel->setFrameStyle(Box | Sunken);
    QToolTip::add(iconLabel, i18n("Jump to the currently playing item"));
}

StatusLabel::~StatusLabel()
{

}

void StatusLabel::setPlayingItem(PlaylistItem *item)
{
    playingItem = item;

    if(item) {
	Playlist *p = static_cast<Playlist *>(item->listView());
	if(p && p->playlistBoxItem()) {

	    QString playlist = QStyleSheet::escape(p->playlistBoxItem()->text());
	    QString artist = QStyleSheet::escape(item->text(PlaylistItem::ArtistColumn));
    	    QString track = QStyleSheet::escape(item->text(PlaylistItem::TrackColumn));

	    playlistLabel->setText(playlist);
	    QString label = artist + " - " + track;
	    trackLabel->setText(label);
	}
	else
	    clear();
    }
    else
	clear();
}

void StatusLabel::clear()
{
    playlistLabel->clear();
    trackLabel->clear();
    playingItem = 0;
    setItemTotalTime(0);
    setItemCurrentTime(0);
}

void StatusLabel::setItemTotalTime(long time)
{
    itemTotalMinutes = int(time / 60);
    itemTotalSeconds = time % 60;
}

void StatusLabel::setItemCurrentTime(long time)
{
    int minutes = int(time / 60);
    int seconds = time % 60;
    QString timeString = formatTime(minutes, seconds) +  " / " + formatTime(itemTotalMinutes, itemTotalSeconds);
    itemTimeLabel->setText(timeString);
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void StatusLabel::mousePressEvent(QMouseEvent *)
{
    jumpToPlayingItem();
}

QString StatusLabel::formatTime(int minutes, int seconds)
{
    QString m = QString::number(minutes);
    if(m.length() == 1)
	m = "0" + m;
    QString s = QString::number(seconds);
    if(s.length() == 1)
	s = "0" + s;
    return(m + ":" + s);
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void StatusLabel::jumpToPlayingItem() const
{
    if(playingItem)
	PlaylistSplitter::setSelected(playingItem);
}

#include "statuslabel.moc"
