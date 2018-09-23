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

#ifndef JUK_DELETEDIALOG_H
#define JUK_DELETEDIALOG_H

class QStringList;

#include <QWidget>
#include <QDialog>

#include <KGuiItem>

namespace Ui
{
    class DeleteDialogBase;
}

class DeleteDialog;

class DeleteWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DeleteWidget(QWidget *parent);

    void setFiles(const QStringList &files);
    bool shouldDelete() const;

signals:
    void signalShouldDelete(bool);
    void accepted();
    void rejected();

protected slots:
    virtual void slotShouldDelete(bool shouldDelete);

private:
    friend DeleteDialog; // TODO: Move KGuiItem stuff into here too
    Ui::DeleteDialogBase *m_ui;
};

class DeleteDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeleteDialog(QWidget* parent);

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
