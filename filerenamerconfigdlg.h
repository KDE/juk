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

#ifndef JUK_FILERENAMERCONFIGDLG_H
#define JUK_FILERENAMERCONFIGDLG_H

#include <QDialog>

class FileRenamerWidget;

class FileRenamerConfigDlg : public QDialog
{
    Q_OBJECT
public:
    explicit FileRenamerConfigDlg(QWidget *parent);

protected slots:
    virtual void accept();

private:
    FileRenamerWidget *m_renamerWidget;
};

#endif // JUK_FILERENAMERCONFIGDLG_H

// vim: set et sw=4 tw=0 sta:
