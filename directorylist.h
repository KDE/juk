/***************************************************************************
                          directorylist.h  -  description
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

#ifndef DIRECTORYLIST_H
#define DIRECTORYLIST_H

#include <qstringlist.h>

#include <kdialogbase.h>

class DirectoryListBase;

class DirectoryList : public KDialogBase
{
    Q_OBJECT

public:
    DirectoryList(QStringList &directories, QWidget* parent = 0, const char* name = 0);
    virtual ~DirectoryList();

signals:
    void signalDirectoryAdded(const QString &directory);
    void signalDirectoryRemoved(const QString &directory);

private slots:
    void slotAddDirectory();
    void slotRemoveDirectory();

private:
    QStringList m_dirList;
    DirectoryListBase *m_base;
};

#endif

// vim:ts=8
