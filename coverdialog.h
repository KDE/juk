/***************************************************************************
    begin                : Sun May 15 2005 
    copyright            : (C) 2005 by Michael Pyne
    email                : michael.pyne@kdemail.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef JUK_COVERDIALOG_H
#define JUK_COVERDIALOG_H

#include "coverdialogbase.h"

class CoverDialog : public CoverDialogBase
{
    Q_OBJECT
public:
    CoverDialog(QWidget *parent);
    ~CoverDialog();

    virtual void show();

public slots:
    void slotArtistClicked(QListViewItem *item);
    void slotContextRequested(QIconViewItem *item, const QPoint &pt);

private slots:
    void loadCovers();
    void removeSelectedCover();
};

#endif /* JUK_COVERDIALOG_H */

// vim: set et ts=4 sw=4:
