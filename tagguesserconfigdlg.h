/*
 * tagguesserconfigdlg.h - (c) 2003 Frerich Raabe <raabe@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef TAGGUESSERCONFIGDLG_H
#define TAGGUESSERCONFIGDLG_H

#include <kdialogbase.h>

class QListViewItem;

class TagGuesserConfigDlgWidget;
class TagGuesserConfigDlg : public KDialogBase
{
    Q_OBJECT
    public:
        TagGuesserConfigDlg(QWidget *parent, const char *name = 0);

    protected slots:
        virtual void accept();

    private slots:
        void slotCurrentChanged(QListViewItem *item);
        void slotRenameItem(QListViewItem *item, const QPoint &p, int c);
        void slotMoveUpClicked();
        void slotMoveDownClicked();
        void slotAddClicked();
        void slotModifyClicked();
        void slotRemoveClicked();

    private:
        TagGuesserConfigDlgWidget *m_child;
};

#endif // TAGGUESSERCONFIGDLG_H
