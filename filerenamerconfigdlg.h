/*
 * filerenamerconfigdlg.h - (c) 2003 Frerich Raabe <raabe@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef FILERENAMERCONFIGDLG_H
#define FILERENAMERCONFIGDLG_H

#include <kdialogbase.h>

class FileRenamerConfigDlgWidget;
class FileRenamerConfigDlg : public KDialogBase
{
    Q_OBJECT
    public:
        FileRenamerConfigDlg(QWidget *parent, const char *name = 0);

    protected slots:
        virtual void accept();

    private slots:
        void currentFilenameChanged(const QString &curFilename);
        void stateChanged(const QString & = QString::null);

    private:
        void loadSettings();
        void saveSettings();

        FileRenamerConfigDlgWidget *m_child;
};

#endif // FILERENAMERCONFIGDLG_H
// vim:ts=4:sw=4:et
