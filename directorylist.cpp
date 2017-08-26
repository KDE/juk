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
#include <QStringListModel>
#include <QtCore/QVariant>
#include <QPushButton>

#include <kfiledialog.h>
#include <klocale.h>
#include "juk_debug.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

DirectoryList::DirectoryList(QStringList directories,
                             QStringList excludedDirectories,
                             bool importPlaylists,
                             QWidget *parent) :
    KDialog(parent),
    m_dirListModel(0)
{
    if(directories.isEmpty()) {
        directories = defaultFolders();
        m_result.addedDirs = directories;
    }

    m_dirListModel = new QStringListModel(directories, this);
    m_excludedDirListModel = new QStringListModel(excludedDirectories, this);

    setCaption(i18n("Folder List"));
    setModal(true);
    showButtonSeparator(true);
    setButtons(KDialog::Ok | KDialog::Cancel);

    m_base = new DirectoryListBase(this);

    setMainWidget(m_base);

    connect(m_base->addDirectoryButton, SIGNAL(clicked()),
        SLOT(slotAddDirectory()));
    connect(m_base->removeDirectoryButton, SIGNAL(clicked()),
        SLOT(slotRemoveDirectory()));
    connect(m_base->addExcludeDirectoryButton, SIGNAL(clicked()),
        SLOT(slotAddExcludeDirectory()));
    connect(m_base->removeExcludeDirectoryButton, SIGNAL(clicked()),
        SLOT(slotRemoveExcludeDirectory()));

    m_base->directoryListView->setModel(m_dirListModel);
    m_base->excludeDirectoryListView->setModel(m_excludedDirListModel);
    m_base->importPlaylistsCheckBox->setChecked(importPlaylists);

    resize(QSize(440, 280).expandedTo(minimumSizeHint()));
}

DirectoryList::~DirectoryList()
{

}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

int DirectoryList::exec()
{
    m_result.status = static_cast<DialogCode>(KDialog::exec());
    m_result.addPlaylists = m_base->importPlaylistsCheckBox->isChecked();
    // FIXME signal
    //return m_result;
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void DirectoryList::slotAddDirectory()
{
    QString dir = KFileDialog::getExistingDirectory();

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

    // These will be used in the loop below
    QModelIndexList indexes;
    QModelIndex firstIndex;
    QString dir;

    // The multiple indexes that are possibly present cannot be deleted one
    // after the other, as changing the layout of the model can change the
    // indexes (similar to iterators and container remove methods).  So, just
    // loop deleting the first index until there is no selection.

    while(itemSelection->hasSelection()) {
        indexes = itemSelection->selectedIndexes();
        firstIndex = indexes.first();
        dir = m_dirListModel->data(firstIndex, Qt::DisplayRole).toString();

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
    QString dir = KFileDialog::getExistingDirectory();

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

    // These will be used in the loop below
    QModelIndexList indexes;
    QModelIndex firstIndex;
    QString dir;

    // The multiple indexes that are possibly present cannot be deleted one
    // after the other, as changing the layout of the model can change the
    // indexes (similar to iterators and container remove methods).  So, just
    // loop deleting the first index until there is no selection.

    while(itemSelection->hasSelection()) {
        indexes = itemSelection->selectedIndexes();
        firstIndex = indexes.first();

        m_excludedDirListModel->removeRow(firstIndex.row());
    }
    m_result.excludedDirs = m_excludedDirListModel->stringList();
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

QStringList DirectoryList::defaultFolders()
{
    QDir home = QDir::home();
    if(home.cd("Music"))
        return QStringList(home.path());
    if(home.cd("music"))
        return QStringList(home.path());
    if(home.cd(i18n("Music")))
        return QStringList(home.path());
    return QStringList();
}

// vim: set et sw=4 tw=0 sta:
