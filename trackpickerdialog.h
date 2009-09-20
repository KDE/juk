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
