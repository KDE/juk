/**
 * Copyright (C) 2003-2004 Scott Wheeler <wheeler@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRACKPICKERDIALOG_H
#define TRACKPICKERDIALOG_H

#include <config-juk.h>

#include <kdialog.h>

#include "musicbrainzquery.h"
#include "ui_trackpickerdialogbase.h"

class TrackPickerDialogBase : public QWidget, public Ui::TrackPickerDialogBase
{
public:
  TrackPickerDialogBase( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};

class TrackPickerDialog : public KDialog
{
    Q_OBJECT

public:
    TrackPickerDialog(const QString &name,
                      const KTRMResultList &results,
                      QWidget *parent = 0);

    virtual ~TrackPickerDialog();

    KTRMResult result() const;

public slots:
    int exec();

private:
    TrackPickerDialogBase *m_base;
};


#endif

// vim: set et sw=4 tw=0 sta:
