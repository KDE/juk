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

#include "directorylistbase.h"

class DirectoryList : public DirectoryListBase
{
    Q_OBJECT

public:
    DirectoryList(QStringList &directories, QWidget* parent = 0, const char* name = 0);
    virtual ~DirectoryList();

signals:
    void directoryAdded(const QString &directory);
    void directoryRemoved(const QString &directory);

private slots:
    void addDirectory();
    void removeDirectory();

private:
    QStringList dirList;
};

#endif
