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

#include "statuslabel.h"
#include "playlistitem.h"
#include "playlist.h"
#include "playlistbox.h"

StatusLabel::StatusLabel(QWidget *parent, const char *name) : QHBox(parent, name) 
{
    trackLabel = new QLabel(this, "trackLabel");
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
	    QString label = p->playlistBoxItem()->text() 
		+ " / " + item->text(PlaylistItem::ArtistColumn) 
		+ " - " + item->text(PlaylistItem::TrackColumn);
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
}

#include "statuslabel.moc"
