/***************************************************************************
    begin                : Tue Aug 31 21:54:20 EST 2004
    copyright            : (C) 2004 by Michael Pyne
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

#ifndef _DELETEDIALOG_H
#define _DELETEDIALOG_H

#include "deletedialogbase.h"

class QStringList;

class DeleteDialog : public DeleteDialogBase
{
public:
    DeleteDialog(QWidget *parent = 0, const char *name = 0);

    void setFiles(const QStringList &files);

    static bool confirmDeleteList(QWidget *parent, const QStringList &condemnedFiles);
};

#endif

// vim: set et ts=4 sw=4:
