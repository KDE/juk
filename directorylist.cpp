/***************************************************************************
                          directorylist.cpp  -  description
                             -------------------
    begin                : Tue Feb 4 2003
    copyright            : (C) 2003 by Scott Wheeler
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

#include <kfiledialog.h>
#include <klistview.h>
#include <kpushbutton.h>

#include "directorylist.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

DirectoryList::DirectoryList(QStringList &directories, QWidget* parent, const char* name) : DirectoryListBase(parent, name, true, 0),
											    dirList(directories)
{
    directoryListView->setFullWidth(true);
    connect(addDirectoryButton, SIGNAL(clicked()), this, SLOT(addDirectory()));
    connect(removeDirectoryButton, SIGNAL(clicked()), this, SLOT(removeDirectory()));

    for(QStringList::ConstIterator it = directories.begin(); it != directories.end(); ++it)
	new KListViewItem(directoryListView, *it);
}

DirectoryList::~DirectoryList()
{
    
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void DirectoryList::addDirectory()
{
    QString dir = KFileDialog::getExistingDirectory();
    if(!dir.isEmpty() && dirList.find(dir) == dirList.end()) {
	dirList.append(dir);
	new KListViewItem(directoryListView, dir);
	emit directoryAdded(dir);
    }
}

void DirectoryList::removeDirectory()
{
    QString dir = directoryListView->selectedItem()->text(0); 
    dirList.remove(dir);
    emit directoryRemoved(dir);
    delete(directoryListView->selectedItem());
}

#include "directorylist.moc"
