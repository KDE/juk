/***************************************************************************
                          musicbrainzitem.cpp
                             -------------------
    begin                : Thur Sep 04 2003
    copyright            : (C) 2003 by Adam Treat
    email                : manyoso@yahoo.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <config.h>

#if HAVE_MUSICBRAINZ

#include "musicbrainzitem.h"

////////////////////////////////////////////////////////////////////////////////
// MusicBrainzItem public methods
////////////////////////////////////////////////////////////////////////////////

MusicBrainzItem::MusicBrainzItem(KListView *parent, const MusicBrainzQuery::Track &track,
                                 const QString &name, const QString &artist,
                                 const QString &album) :
    KListViewItem(parent, name, artist, album),
    m_track(track)
{

}

MusicBrainzItem::~MusicBrainzItem()
{

}

#include "musicbrainzitem.moc"

#endif // HAVE_MUSICBRAINZ
