/**
 * Copyright (C) 2005 Michael Pyne <mpyne@kde.org>
 * Copyright (C) 2014 Arnold Dumas <contact@arnolddumas.com>
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

#ifndef COVERDIALOG_H
#define COVERDIALOG_H

#include "ui_coverdialogbase.h"

#include <QWidget>

class QListWidgetItem;

class CoverDialog : public QWidget, public Ui::CoverDialogBase
{
    Q_OBJECT
public:
    CoverDialog(QWidget *parent);
    ~CoverDialog();

    virtual void show();

public slots:
    void slotArtistClicked(QListWidgetItem *item);
    void slotContextRequested(const QPoint &pt);
    void slotSearchPatternChanged(const QString& pattern);

private slots:
    void loadCovers();
    void removeSelectedCover();
};

#endif /* COVERDIALOG_H */

// vim: set et sw=4 tw=0 sta:
