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
#include <klocale.h>
#include <klistview.h>
#include <kpushbutton.h>

#include "directorylistbase.h"
#include "directorylist.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

DirectoryList::DirectoryList(const QStringList &directories, QWidget *parent,
			     const char *name) :
    KDialogBase(parent, name, true, i18n("Directory List"), Ok | Cancel, Ok, true),
    m_dirList(directories)
{
    m_base = new DirectoryListBase(this);

    setMainWidget(m_base);

    m_base->directoryListView->setFullWidth(true);

    connect(m_base->addDirectoryButton, SIGNAL(clicked()), 
	SLOT(slotAddDirectory()));
    connect(m_base->removeDirectoryButton, SIGNAL(clicked()), 
	SLOT(slotRemoveDirectory()));

    QStringList::ConstIterator it = directories.begin();
    for(; it != directories.end(); ++it)
	new KListViewItem(m_base->directoryListView, *it);

    QSize sz = sizeHint();
    setMinimumSize(QMAX(350, sz.width()), QMAX(250, sz.height()));
    resize(sizeHint());
}

DirectoryList::~DirectoryList()
{

}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void DirectoryList::slotAddDirectory()
{
    QString dir = KFileDialog::getExistingDirectory();
    if(!dir.isEmpty() && m_dirList.find(dir) == m_dirList.end()) {
	m_dirList.append(dir);
	new KListViewItem(m_base->directoryListView, dir);
	emit signalDirectoryAdded(dir);
    }
}

void DirectoryList::slotRemoveDirectory()
{
    if(!m_base->directoryListView->selectedItem())
	return;

    QString dir = m_base->directoryListView->selectedItem()->text(0); 
    m_dirList.remove(dir);
    emit signalDirectoryRemoved(dir);
    delete m_base->directoryListView->selectedItem();
}

#include "directorylist.moc"

// vim: ts=8
