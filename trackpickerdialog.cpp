/***************************************************************************
                          trackpickerdialog.h
                             -------------------
    begin                : Sat Sep 6 2003
    copyright            : (C) 2003 by Scott Wheeler
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

#include <qlabel.h>

#include <klocale.h>

#include "trackpickerdialog.h"
#include "trackpickerdialogbase.h"
#include "musicbrainzitem.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

TrackPickerDialog::TrackPickerDialog(const QString &fileName,
                                     const MusicBrainzQuery::TrackList &tracks,
				     QWidget *parent,
                                     const char *name) :
    KDialogBase(parent, name, true, i18n("Directory List"), Ok | Cancel, Ok, true)
{
    m_base = new TrackPickerDialogBase(this);
    setMainWidget(m_base);

    m_base->fileLabel->setText(fileName);

    MusicBrainzQuery::TrackList::ConstIterator it = tracks.begin();
    for(; it != tracks.end(); ++it)
        new MusicBrainzItem(m_base->trackList, *it, (*it).name, (*it).artist, (*it).album);


    m_base->trackList->setSelected(m_base->trackList->firstChild(), true);
    setMinimumWidth(QMAX(400, width()));

}

TrackPickerDialog::~TrackPickerDialog()
{

}

MusicBrainzQuery::Track TrackPickerDialog::selectedTrack() const
{
    if(m_base->trackList->selectedItem())
        return static_cast<MusicBrainzItem *>(m_base->trackList->selectedItem())->track();
    else
        return MusicBrainzQuery::Track();
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

int TrackPickerDialog::exec()
{
    int dialogCode = KDialogBase::exec();

    // Only return true if an item was selected.

    if(m_base->trackList->selectedItem())
        return dialogCode;
    else
        return Rejected;
}

#include "trackpickerdialog.moc"
