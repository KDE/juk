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


#include <qcheckbox.h>

#include "deletedialogbase.h"

class QStringList;
class KListBox;
class QLabel;
class QWidgetStack;

class DeleteWidget : public DeleteDialogBase
{
    Q_OBJECT

public:
    DeleteWidget(QWidget *parent = 0, const char *name = 0);

    void setFiles(const QStringList &files);

protected slots:
    virtual void slotShouldDelete(bool shouldDelete);
};

class DeleteDialog : public KDialogBase
{
    Q_OBJECT

public:
    DeleteDialog(QWidget *parent, const char *name = "delete_dialog");

    bool confirmDeleteList(const QStringList &condemnedFiles);
    void setFiles(const QStringList &files);
    bool shouldDelete() const { return m_widget->ddShouldDelete->isChecked(); }

protected slots:
    virtual void accept();
    void slotShouldDelete(bool shouldDelete);

private:
    DeleteWidget *m_widget;
    KGuiItem m_trashGuiItem;
};

#endif

// vim: set et ts=4 sw=4:
