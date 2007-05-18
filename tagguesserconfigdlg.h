/*
 * tagguesserconfigdlg.h - Copyright (c) 2003 Frerich Raabe <raabe@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef TAGGUESSERCONFIGDLG_H
#define TAGGUESSERCONFIGDLG_H

#include <kdialog.h>
#include "ui_tagguesserconfigdlgwidget.h"

class Q3ListViewItem;


class TagGuesserConfigDlgWidget : public QWidget, public Ui::TagGuesserConfigDlgWidget
{
public:
  TagGuesserConfigDlgWidget( QWidget *parent ) : QWidget( parent ) {
    setupUi( this );
  }
};

class TagGuesserConfigDlg : public KDialog
{
    Q_OBJECT
    public:
        explicit TagGuesserConfigDlg(QWidget *parent, const char *name = 0);

    protected slots:
        virtual void accept();

    private slots:
        void slotCurrentChanged(Q3ListViewItem *item);
        void slotRenameItem(Q3ListViewItem *item, const QPoint &p, int c);
        void slotMoveUpClicked();
        void slotMoveDownClicked();
        void slotAddClicked();
        void slotModifyClicked();
        void slotRemoveClicked();

    private:
        TagGuesserConfigDlgWidget *m_child;
};

#endif // TAGGUESSERCONFIGDLG_H

// vim: set et sw=4 tw=0 sta:
