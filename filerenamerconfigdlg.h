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

#ifndef JUK_FILERENAMERCONFIGDLG_H
#define JUK_FILERENAMERCONFIGDLG_H


class FileRenamerWidget;

class FileRenamerConfigDlg : public KDialogBase
{
    Q_OBJECT
    public:
    FileRenamerConfigDlg(QWidget *parent);

    protected slots:
    virtual void accept();

    private:
    FileRenamerWidget *m_renamerWidget;
};

#endif // FILERENAMERCONFIGDLG_H

// vim: set et ts=4 sw=4:
