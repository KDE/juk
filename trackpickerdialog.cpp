/***************************************************************************
    begin                : Sat Sep 6 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler
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

#include <config.h>

#if HAVE_MUSICBRAINZ

#include <qlabel.h>

#include <klistview.h>
#include <klocale.h>

#include "trackpickerdialog.h"
#include "trackpickerdialogbase.h"

#define NUMBER(x) (x == 0 ? QString::null : QString::number(x))

class TrackPickerItem : public KListViewItem
{
public:
    TrackPickerItem(KListView *parent, const KTRMResult &result) :
        KListViewItem(parent, parent->lastChild(),
                      result.title(), result.artist(), result.album(),
                      NUMBER(result.track()), NUMBER(result.year())),
        m_result(result) {}
    KTRMResult result() const { return m_result; }

private:
    KTRMResult m_result;
};

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

TrackPickerDialog::TrackPickerDialog(const QString &name,
                                     const KTRMResultList &results,
                                     QWidget *parent) :
    KDialogBase(parent, name.latin1(), true, i18n("Internet Tag Guesser"), Ok | Cancel, Ok, true)
{
    m_base = new TrackPickerDialogBase(this);
    setMainWidget(m_base);

    m_base->fileLabel->setText(name);
    m_base->trackList->setSorting(-1);

    for(KTRMResultList::ConstIterator it = results.begin(); it != results.end(); ++it)
        new TrackPickerItem(m_base->trackList, *it);

    m_base->trackList->setSelected(m_base->trackList->firstChild(), true);

    connect(m_base->trackList, SIGNAL(doubleClicked(QListViewItem *, const QPoint &, int)),
            this, SLOT(accept()));

    setMinimumWidth(kMax(400, width()));
}

TrackPickerDialog::~TrackPickerDialog()
{

}

KTRMResult TrackPickerDialog::result() const
{
    if(m_base->trackList->selectedItem())
        return static_cast<TrackPickerItem *>(m_base->trackList->selectedItem())->result();
    else
        return KTRMResult();
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

#endif // HAVE_MUSICBRAINZ
