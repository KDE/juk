/***************************************************************************
    begin                : Tue Feb 4 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler
    email                : wheeler@kde.org

    copyright            : (C) 2009 by Georg Grabler
    email                : ggrabler@gmail.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "directorylist.h"

#include <QtGui/QCheckBox>
#include <QtGui/QStringListModel>
#include <QtCore/QVariant>

#include <kfiledialog.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <kdebug.h>

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

DirectoryList::Result DirectoryList::exec()
{
    m_result.status = static_cast<DialogCode>(KDialog::exec());
    m_result.addPlaylists = m_base->importPlaylistsCheckBox->isChecked();
    return m_result;
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

#include "directorylist.moc"

// vim: set et sw=4 tw=0 sta:
