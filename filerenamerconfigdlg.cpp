/***************************************************************************
    begin                : Mon Nov 01 2004
    copyright            : (C) 2004 by Michael Pyne
                         : (c) 2003 Frerich Raabe <raabe@kde.org>
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

#include <klocale.h>

#include "filerenamer.h"
#include "filerenamerconfigdlg.h"

FileRenamerConfigDlg::FileRenamerConfigDlg(QWidget *parent) :
    KDialogBase(parent, "file renamer dialog", true,
                i18n("File Renamer Options"), Ok | Cancel),
    m_renamerWidget(new FileRenamerWidget(this))
{
    m_renamerWidget->setMinimumSize(400, 300);

    setMainWidget(m_renamerWidget);
}

void FileRenamerConfigDlg::accept()
{
    // Make sure the config gets saved.

    m_renamerWidget->saveConfig();

    KDialogBase::accept();
}

#include "filerenamerconfigdlg.moc"

// vim: set et sw=4 ts=4:
