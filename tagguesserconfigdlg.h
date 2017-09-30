/**
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

#ifndef JUK_TAGGUESSERCONFIGDLG_H
#define JUK_TAGGUESSERCONFIGDLG_H

#include <QDialog>

#include "ui_tagguesserconfigdlgwidget.h"

class QStringListModel;

class TagGuesserConfigDlgWidget : public QWidget, public Ui::TagGuesserConfigDlgWidget
{
    Q_OBJECT

public:
    TagGuesserConfigDlgWidget(QWidget *parent)
      : QWidget(parent)
    {
        setupUi(this);
    }
};

class TagGuesserConfigDlg : public QDialog
{
    Q_OBJECT

public:
    explicit TagGuesserConfigDlg(QWidget *parent, const char *name = nullptr);

protected slots:
    virtual void accept();

private slots:
    void slotCurrentChanged(QModelIndex item);
    void slotMoveUpClicked();
    void slotMoveDownClicked();
    void slotAddClicked();
    void slotModifyClicked();
    void slotRemoveClicked();

private:
    TagGuesserConfigDlgWidget *m_child;
    QStringListModel *m_tagSchemeModel;
};

#endif // JUK_TAGGUESSERCONFIGDLG_H

// vim: set et sw=4 tw=0 sta:
