/***************************************************************************
                           playlistwidget.cpp  -  description
                             -------------------
    begin                : Tue Feb 5 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <klocale.h>

#include <qnamespace.h>
#include <qvbox.h>
#include <qhbox.h>
#include <qlayout.h>
#include <qlabel.h>

#include "playlistwidget.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

PlaylistWidget::PlaylistWidget(QWidget *parent) : QWidget(parent)
{
    setupLayout();
}

PlaylistWidget::~PlaylistWidget()
{
}

void PlaylistWidget::add(const QString &item)
{
    playlistList->append(item);
}

void PlaylistWidget::add(QStringList &items)
{
    playlistList->append(items);
}

void PlaylistWidget::add(PlaylistItem *item)
{
    playlistList->append(item);
}

void PlaylistWidget::add(QPtrList<PlaylistItem> &items)
{
    playlistList->append(items);
}

void PlaylistWidget::remove(QPtrList<PlaylistItem> &items)
{
    playlistList->remove(items);
}

Playlist *PlaylistWidget::getPlaylistList()
{
    return(playlistList);
}

QPtrList<PlaylistItem> PlaylistWidget::getSelectedItems()
{
    return(playlistList->selectedItems());
}

PlaylistItem *PlaylistWidget::firstItem()
{
    return(dynamic_cast<PlaylistItem *>(playlistList->firstChild()));
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void PlaylistWidget::setupLayout()
{
    /////////////////////////////////////////////////////////////////////////
    // define a main layout box
    /////////////////////////////////////////////////////////////////////////
    QVBoxLayout *playlistMainLayout = new QVBoxLayout(this);

    playlistList = new Playlist(this, "playlistList");
    playlistMainLayout->addWidget(playlistList);
}
