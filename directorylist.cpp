/**
 * Copyright (C) 2003-2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2009 Georg Grabler <ggrabler@gmail.com>
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

#include "directorylist.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QPushButton>
#include <QStandardPaths>
#include <QStringListModel>

#include "juk_debug.h"

////////////////////////////////////////////////////////////////////////////////
// static helpers
////////////////////////////////////////////////////////////////////////////////

static QStringList defaultFolders()
{
    return QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

DirectoryList::DirectoryList(const QStringList &directories,
                             const QStringList &excludedDirectories,
                             bool importPlaylists,
                             QWidget *parent)
  : QDialog(parent)
  , m_dirListModel(new QStringListModel(directories, this))
  , m_excludedDirListModel(new QStringListModel(excludedDirectories, this))
{
    if(directories.isEmpty()) {
        const auto defaultDirs = defaultFolders();
        m_dirListModel->setStringList(defaultDirs);
        m_result.addedDirs = defaultDirs;
    }

    setWindowTitle(i18nc("@title:window", "Folder List"));
    setModal(true);

    m_base = new DirectoryListBase(this);
    auto layout = new QVBoxLayout(this);
    layout->addWidget(m_base);

    connect(m_base->addDirectoryButton, SIGNAL(clicked()),
        SLOT(slotAddDirectory()));
    connect(m_base->removeDirectoryButton, SIGNAL(clicked()),
        SLOT(slotRemoveDirectory()));
    connect(m_base->addExcludeDirectoryButton, SIGNAL(clicked()),
        SLOT(slotAddExcludeDirectory()));
    connect(m_base->removeExcludeDirectoryButton, SIGNAL(clicked()),
        SLOT(slotRemoveExcludeDirectory()));
    connect(m_base->dlgButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_base->dlgButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_base->directoryListView->setModel(m_dirListModel);
    m_base->excludeDirectoryListView->setModel(m_excludedDirListModel);
    m_base->importPlaylistsCheckBox->setChecked(importPlaylists);

    resize(QSize(440, 280).expandedTo(minimumSizeHint()));
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

int DirectoryList::exec()
{
    m_result.status = static_cast<QDialog::DialogCode>(QDialog::exec());
    m_result.addPlaylists = m_base->importPlaylistsCheckBox->isChecked();
    return m_result.status;
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void DirectoryList::slotAddDirectory()
{
    QString dir = QFileDialog::getExistingDirectory();
    if(dir.isEmpty())
        return;

    QStringList dirs = m_dirListModel->stringList();
    if(!dirs.contains(dir)) {
        dirs.append(dir);
        m_dirListModel->setStringList(dirs);

        m_result.addedDirs.append(dir);
        m_result.removedDirs.removeAll(dir);
    }
}

void DirectoryList::slotRemoveDirectory()
{
    QItemSelectionModel *itemSelection = m_base->directoryListView->selectionModel();

    // The multiple indexes that are possibly present cannot be deleted one
    // after the other, as changing the layout of the model can change the
    // indexes (similar to iterators and container remove methods). So, just
    // loop deleting the first index until there is no selection.

    while(itemSelection->hasSelection()) {
        QModelIndexList indexes = itemSelection->selectedIndexes();
        QModelIndex firstIndex = indexes.first();
        QString dir = m_dirListModel->data(firstIndex, Qt::DisplayRole).toString();

        m_dirListModel->removeRow(firstIndex.row());

        // Don't mess up if user removes directory they've just added before
        // closing out of the dialog.
        if(m_result.addedDirs.contains(dir))
            m_result.addedDirs.removeAll(dir);
        else
            m_result.removedDirs.append(dir);
    }
}

void DirectoryList::slotAddExcludeDirectory()
{
    QString dir = QFileDialog::getExistingDirectory();
    if(dir.isEmpty())
        return;

    QStringList dirs = m_excludedDirListModel->stringList();
    if(!dirs.contains(dir)) {
        dirs.append(dir);
        m_excludedDirListModel->setStringList(dirs);
    }
    m_result.excludedDirs = m_excludedDirListModel->stringList();
}

void DirectoryList::slotRemoveExcludeDirectory()
{
    QItemSelectionModel *itemSelection = m_base->excludeDirectoryListView->selectionModel();

    // The multiple indexes that are possibly present cannot be deleted one
    // after the other, as changing the layout of the model can change the
    // indexes (similar to iterators and container remove methods).  So, just
    // loop deleting the first index until there is no selection.

    while(itemSelection->hasSelection()) {
        QModelIndexList indexes = itemSelection->selectedIndexes();
        QModelIndex firstIndex = indexes.first();

        m_excludedDirListModel->removeRow(firstIndex.row());
    }
    m_result.excludedDirs = m_excludedDirListModel->stringList();
}

// vim: set et sw=4 tw=0 sta:
