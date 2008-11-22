/***************************************************************************
    begin                : Tue Feb 4 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler
    email                : wheeler@kde.org
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

#include <QCheckBox>

#include <kfiledialog.h>
#include <klocale.h>
#include <k3listview.h>
#include <kpushbutton.h>

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

DirectoryList::DirectoryList(const QStringList &directories,
                            bool importPlaylists,
                             QWidget *parent) :
    KDialog(parent),
    m_dirList(directories),
    m_importPlaylists(importPlaylists)
{
    setCaption(i18n("Folder List"));
    setModal( true );
    showButtonSeparator( true );
    setButtons(KDialog::Ok | KDialog::Cancel);

    m_base = new DirectoryListBase(this);

    setMainWidget(m_base);

    m_base->directoryListView->setFullWidth(true);

    connect(m_base->addDirectoryButton, SIGNAL(clicked()),
        SLOT(slotAddDirectory()));
    connect(m_base->removeDirectoryButton, SIGNAL(clicked()),
        SLOT(slotRemoveDirectory()));

    QStringList::ConstIterator it = directories.begin();
    for(; it != directories.end(); ++it)
        new K3ListViewItem(m_base->directoryListView, *it);

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
    if(!dir.isEmpty() && !m_dirList.contains(dir)) {
        m_dirList.append(dir);
        new K3ListViewItem(m_base->directoryListView, dir);
        m_result.addedDirs.append(dir);
    }
}

void DirectoryList::slotRemoveDirectory()
{
    if(!m_base->directoryListView->selectedItem())
        return;

    QString dir = m_base->directoryListView->selectedItem()->text(0);
    m_dirList.removeAll(dir);
    m_result.removedDirs.append(dir);
    delete m_base->directoryListView->selectedItem();
}

#include "directorylist.moc"

// vim: set et sw=4 tw=0 sta:
