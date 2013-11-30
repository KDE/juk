/**
 * Copyright (C) 2004 Michael Pyne <mpyne@kde.org>
 * Copyright (C) 2003 Frerich Raabe <raabe@kde.org>
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

#include "filerenamerconfigdlg.h"
#include "filerenamer.h"

#include <klocale.h>

FileRenamerConfigDlg::FileRenamerConfigDlg(QWidget *parent) :
    KDialog(parent),
    m_renamerWidget(new FileRenamerWidget(this))
{
    setObjectName( QLatin1String("file renamer dialog" ));
    setModal(true);
    setCaption(i18n("File Renamer Options"));
    setButtons(Ok | Cancel);

    m_renamerWidget->setMinimumSize(400, 300);

    setMainWidget(m_renamerWidget);
}

void FileRenamerConfigDlg::accept()
{
    // Make sure the config gets saved.

    m_renamerWidget->saveConfig();

    KDialog::accept();
}

#include "filerenamerconfigdlg.moc"

// vim: set et sw=4 tw=0 sta:
