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
#include "playlist.h"
#include "playlistbox.h"
#include "playlistsplitter.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

StatusLabel::StatusLabel(QWidget *parent, const char *name) : QHBox(parent, name), mode(PlaylistInfo), m_playlistCount(0), m_showTimeRemaining(false)
{
    QFrame *trackAndPlaylist = new QFrame(this);
    trackAndPlaylist->setFrameStyle(Box | Sunken);
    trackAndPlaylist->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Make sure that we have enough of a margin to suffice for the borders,
    // hence the "lineWidth() * 2"
    QHBoxLayout *trackAndPlaylistLayout = new QHBoxLayout(trackAndPlaylist, trackAndPlaylist->lineWidth() * 2, 5, "trackAndPlaylistLayout");
    trackAndPlaylistLayout->addSpacing(5);

    m_playlistLabel = new QLabel(trackAndPlaylist, "playlistLabel");
    trackAndPlaylistLayout->addWidget(m_playlistLabel);
    m_playlistLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_playlistLabel->setTextFormat(PlainText);
    m_playlistLabel->setAlignment(AlignLeft | AlignVCenter);

    m_trackLabel = new QLabel(trackAndPlaylist, "trackLabel");
    trackAndPlaylistLayout->addWidget(m_trackLabel);
    m_trackLabel->setAlignment(AlignRight | AlignVCenter);
    m_trackLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_trackLabel->setTextFormat(PlainText);

    trackAndPlaylistLayout->addSpacing(5);

    m_itemTimeLabel = new QLabel(this);
    QFontMetrics fontMetrics(font());
    m_itemTimeLabel->setAlignment(AlignCenter);
    m_itemTimeLabel->setMinimumWidth(fontMetrics.boundingRect("000:00 / 000:00").width());
    m_itemTimeLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_itemTimeLabel->setFrameStyle(Box | Sunken);
    m_itemTimeLabel->installEventFilter(this);

    setItemTotalTime(0);
    setItemCurrentTime(0);

    QHBox *jumpBox = new QHBox(this);
    jumpBox->setFrameStyle(Box | Sunken);
    jumpBox->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);

    QPushButton *jumpButton = new QPushButton(jumpBox);
    jumpButton->setPixmap(SmallIcon("up"));
    jumpButton->setFlat(true);

    QToolTip::add(jumpButton, i18n("Jump to the currently playing item"));
    connect(jumpButton, SIGNAL(clicked()), this, SIGNAL(jumpButtonClicked()));

    installEventFilter(this);
}

StatusLabel::~StatusLabel()
{

}

void StatusLabel::setPlaylistInfo(const QString &name, int count)
{
    m_playlistName = name;

    if(mode == PlaylistInfo)
	m_playlistLabel->setText(m_playlistName);

    setPlaylistCount(count);
}

void StatusLabel::setPlaylistCount(int c)
{
    m_playlistCount = c;

    if(mode == PlaylistInfo)
	m_trackLabel->setText(QString::number(c) + " " + i18n("item(s)"));
}

void StatusLabel::setPlayingItemInfo(const QString &track, const QString &playlist)
{
    mode = PlayingItemInfo;

    m_trackLabel->setText(track);
    m_playlistLabel->setText(playlist.simplifyWhiteSpace());
}

void StatusLabel::clear()
{
    m_playlistLabel->clear();
    m_trackLabel->clear();
    setItemTotalTime(0);
    setItemCurrentTime(0);

    mode = PlaylistInfo;

    setPlaylistInfo(m_playlistName, m_playlistCount);
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void StatusLabel::updateTime()
{
    int minutes;
    int seconds;

    if(m_showTimeRemaining) {
	minutes = int((m_itemTotalTime - m_itemCurrentTime) / 60);
	seconds = (m_itemTotalTime - m_itemCurrentTime) % 60;
    }
    else {
	minutes = int(m_itemCurrentTime / 60);
	seconds = m_itemCurrentTime % 60;
    }

    int totalMinutes = int(m_itemTotalTime / 60);
    int totalSeconds = m_itemTotalTime % 60;

    QString timeString = formatTime(minutes, seconds) +  " / " + formatTime(totalMinutes, totalSeconds);
    m_itemTimeLabel->setText(timeString);
}

bool StatusLabel::eventFilter(QObject *o, QEvent *e)
{
    if(!o || !e)
	return false;

    QMouseEvent *mouseEvent = dynamic_cast<QMouseEvent *>(e);
    if(mouseEvent && mouseEvent->state() == LeftButton) {

	if(o == m_itemTimeLabel) {
	    m_showTimeRemaining = !m_showTimeRemaining;
	    updateTime();
	}
	else
	    emit(jumpButtonClicked());

	return true;
    }
    return false;
}

QString StatusLabel::formatTime(int minutes, int seconds) // static
{
    QString m = QString::number(minutes);
    if(m.length() == 1)
	m = "0" + m;
    QString s = QString::number(seconds);
    if(s.length() == 1)
	s = "0" + s;
    return m + ":" + s;
}

#include "statuslabel.moc"
