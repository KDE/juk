/**
 * Copyright (C) 2003-2004 Scott Wheeler <wheeler@kde.org>
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

#ifndef JUK_DIRECTORYLIST_H
#define JUK_DIRECTORYLIST_H

#include <QDialog>

#include "ui_directorylistbase.h"

class QStringListModel;

class DirectoryListBase : public QWidget, public Ui::DirectoryListBase
{
    Q_OBJECT

public:
    DirectoryListBase(QWidget *parent) : QWidget(parent)
    {
        setupUi(this);
    }
};

class DirectoryList : public QDialog
{
    Q_OBJECT

public:
    struct Result
    {
        QStringList addedDirs;
        QStringList removedDirs;
        QStringList excludedDirs;
        DialogCode status;
        bool addPlaylists;
    };

    DirectoryList(
        const QStringList &directories,
        const QStringList &excludeDirectories,
        bool importPlaylists,
        QWidget *parent = nullptr);

    Result dialogResult() const { return m_result; }

public slots:
    int exec();

signals:
    void signalDirectoryAdded(const QString &directory);
    void signalDirectoryRemoved(const QString &directory);
    void signalExcludeDirectoryAdded(const QString &directory);
    void signalExcludeDirectoryRemoved(const QString &directory);

private slots:
    void slotAddDirectory();
    void slotRemoveDirectory();
    void slotAddExcludeDirectory();
    void slotRemoveExcludeDirectory();

private:
    QStringListModel *m_dirListModel;
    QStringListModel *m_excludedDirListModel;
    DirectoryListBase *m_base;
    Result m_result;
};

#endif

// vim: set et sw=4 tw=0 sta:
