/***************************************************************************
    begin                : Tue Aug 3 2004
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

#include "musicbrainzquery.h"

#if HAVE_MUSICBRAINZ

#include "trackpickerdialog.h"
#include "tag.h"
#include "collectionlist.h"

#include <kmainwindow.h>
#include <kapplication.h>
#include <kstatusbar.h>
#include <klocale.h>
#include <kdebug.h>

#include <qfileinfo.h>

MusicBrainzLookup::MusicBrainzLookup(const FileHandle &file) :
    KTRMLookup(file.absFilePath()),
    m_file(file) {}

void MusicBrainzLookup::recognized()
{
    KTRMLookup::recognized();
    confirmation();
}

void MusicBrainzLookup::unrecognized()
{
    KTRMLookup::unrecognized();
    message(i18n("No matches found."));
}

void MusicBrainzLookup::collision()
{
    KTRMLookup::collision();
    confirmation();
}

void MusicBrainzLookup::error()
{
    KTRMLookup::error();
    message(i18n("TRM generation failed"));
}

void MusicBrainzLookup::message(const QString &s) const
{
    KMainWindow *w = static_cast<KMainWindow *>(kapp->mainWidget());
    w->statusBar()->message(m_file.fileInfo().fileName() + " - " + s, 2000);
}

void MusicBrainzLookup::confirmation()
{
    if(results().isEmpty())
        return;

    TrackPickerDialog dialog(m_file.fileInfo().fileName(), results());

    if(dialog.exec() == QDialog::Accepted && !dialog.result().isEmpty()) {

        KTRMResult result = dialog.result();

        if(!result.title.isEmpty())
            m_file.tag()->setTitle(result.title);
        if(!result.artist.isEmpty())
            m_file.tag()->setArtist(result.artist);
        if(!result.album.isEmpty())
            m_file.tag()->setAlbum(result.album);
        if(result.track != 0)
            m_file.tag()->setTrack(result.track);
        if(result.year != 0)
            m_file.tag()->setYear(result.year);

        m_file.tag()->save();

        CollectionList::instance()->slotRefreshItem(m_file.absFilePath());
    }
}

#endif
