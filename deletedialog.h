/**
 * Copyright (C) 2004, 2008 Michael Pyne <mpyne@kde.org>
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

#ifndef DELETEDIALOG_H
#define DELETEDIALOG_H

class QStringList;

#include <QWidget>

#include <KDialog>

namespace Ui
{
    class DeleteDialogBase;
}

class DeleteWidget : public QWidget
{
    Q_OBJECT

public:
    DeleteWidget(QWidget *parent);

    void setFiles(const QStringList &files);
    bool shouldDelete() const;

signals:
    void signalShouldDelete(bool);

protected slots:
    virtual void slotShouldDelete(bool shouldDelete);

private:
    Ui::DeleteDialogBase *m_ui;
};

class DeleteDialog : public KDialog
{
    Q_OBJECT

public:
    DeleteDialog(QWidget *parent);

    bool confirmDeleteList(const QStringList &condemnedFiles);
    void setFiles(const QStringList &files);
    bool shouldDelete() const { return m_widget->shouldDelete(); }

protected slots:
    virtual void accept();
    void slotShouldDelete(bool shouldDelete);

private:
    DeleteWidget *m_widget;
    KGuiItem m_trashGuiItem;
};

#endif

// vim: set et sw=4 tw=0 sta:
