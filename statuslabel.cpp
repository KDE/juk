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

#include <kdebug.h>

#include <qevent.h>
#include <qstylesheet.h>

#include "statuslabel.h"
#include "playlistitem.h"
#include "playlist.h"
#include "playlistbox.h"
#include "playlistsplitter.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

StatusLabel::StatusLabel(QWidget *parent, const char *name) : QHBox(parent, name) 
{
    trackLabel = new QLabel(this, "trackLabel");
    trackLabel->installEventFilter(this);
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

	    QString label = playlist + " / " + artist + " - <i>" + track + "</i>";
	    trackLabel->setText(label);
	}
	else
	    trackLabel->clear();
    }
    else
	trackLabel->clear();    
}

void StatusLabel::clear()
{
    trackLabel->clear();
    playingItem = 0;
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

bool StatusLabel::eventFilter(QObject *o, QEvent *e)
{
    if(o && o == trackLabel &&
       e && e->type() == QEvent::MouseButtonPress) {
	PlaylistSplitter::setSelected(playingItem);
	return(true);
    }

    return(false);
}

#include "statuslabel.moc"
