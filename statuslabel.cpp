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
#include <klocale.h>
#include <kdebug.h>

#include <qevent.h>
#include <qstylesheet.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qfontmetrics.h>
#include <qlayout.h>

#include "statuslabel.h"
#include "playlistitem.h"
#include "playlist.h"
#include "playlistbox.h"
#include "playlistsplitter.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

StatusLabel::StatusLabel(QWidget *parent, const char *name) : QHBox(parent, name), mode(PlaylistInfo), playlistCount(0), showTimeRemaining(false)
{
    QFrame *trackAndPlaylist = new QFrame(this);
    trackAndPlaylist->setFrameStyle(Box | Sunken);
    trackAndPlaylist->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // Make sure that we have enough of a margin to suffice for the borders, 
    // hence the "lineWidth() * 2"
    QHBoxLayout *trackAndPlaylistLayout = new QHBoxLayout(trackAndPlaylist, trackAndPlaylist->lineWidth() * 2, 5, "trackAndPlaylistLayout");
    trackAndPlaylistLayout->addSpacing(5);

    playlistLabel = new QLabel(trackAndPlaylist, "playlistLabel");
    trackAndPlaylistLayout->addWidget(playlistLabel);
    playlistLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    playlistLabel->setTextFormat(PlainText);
    playlistLabel->setAlignment(AlignLeft | AlignVCenter);

    trackLabel = new QLabel(trackAndPlaylist, "trackLabel");
    trackAndPlaylistLayout->addWidget(trackLabel);
    trackLabel->setAlignment(AlignRight | AlignVCenter);
    trackLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    trackLabel->setTextFormat(PlainText);

    trackAndPlaylistLayout->addSpacing(5);
    
    itemTimeLabel = new QLabel(this);
    QFontMetrics fontMetrics(font());
    itemTimeLabel->setAlignment(AlignCenter);
    itemTimeLabel->setMinimumWidth(fontMetrics.boundingRect("000:00 / 000:00").width());
    itemTimeLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    itemTimeLabel->setFrameStyle(Box | Sunken);
    itemTimeLabel->installEventFilter(this);

    setItemTotalTime(0);
    setItemCurrentTime(0);

    QHBox *jumpBox = new QHBox(this);
    jumpBox->setFrameStyle(Box | Sunken);
    jumpBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

    QPushButton *jumpButton = new QPushButton(jumpBox);
    jumpButton->setPixmap(SmallIcon("up"));
    jumpButton->setFlat(true);

    QToolTip::add(jumpButton, i18n("Jump to the currently playing item"));
    connect(jumpButton, SIGNAL(clicked()), this, SLOT(jumpToPlayingItem()));

    installEventFilter(this);
}

StatusLabel::~StatusLabel()
{

}

void StatusLabel::setPlaylistInfo(const QString &name, int count)
{
    playlistName = name;

    if(mode == PlaylistInfo)
	playlistLabel->setText(playlistName);

    setPlaylistCount(count);
}

void StatusLabel::setPlaylistCount(int c)
{
    playlistCount = c;

    if(mode == PlaylistInfo)
	trackLabel->setText(QString::number(c) + " " + i18n("Item(s)"));
}

void StatusLabel::setPlayingItemInfo(const QString &name, const QString &artist, const QString &playlist)
{
    mode = PlayingItemInfo;

    trackLabel->setText(artist.simplifyWhiteSpace() + " - " + name.simplifyWhiteSpace());
    playlistLabel->setText(playlist.simplifyWhiteSpace());
}

void StatusLabel::clear()
{
    playlistLabel->clear();
    trackLabel->clear();
    setItemTotalTime(0);
    setItemCurrentTime(0);
    
    mode = PlaylistInfo;

    setPlaylistInfo(playlistName, playlistCount);
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void StatusLabel::updateTime()
{
    int minutes;
    int seconds;

    if(showTimeRemaining) {
	minutes = int((itemTotalTime - itemCurrentTime) / 60);
	seconds = (itemTotalTime - itemCurrentTime) % 60;
    }
    else {
	minutes = int(itemCurrentTime / 60);
	seconds = itemCurrentTime % 60;
    }

    int totalMinutes = int(itemTotalTime / 60);
    int totalSeconds = itemTotalTime % 60;

    QString timeString = formatTime(minutes, seconds) +  " / " + formatTime(totalMinutes, totalSeconds);
    itemTimeLabel->setText(timeString);    
}

bool StatusLabel::eventFilter(QObject *o, QEvent *e)
{
    if(!o || !e)
	return(false);
    
    QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent *>(e);
    if(mouseEvent && mouseEvent->state() == LeftButton) {

	if(o == itemTimeLabel) {
	    showTimeRemaining = !showTimeRemaining;
	    updateTime();
	}
	else
	    jumpToPlayingItem();

	return(true);
    }
    return(false);
}

QString StatusLabel::formatTime(int minutes, int seconds) // static
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
//    PlaylistSplitter::setSelected(playingItem);
}

#include "statuslabel.moc"
