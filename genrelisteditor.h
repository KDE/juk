/***************************************************************************
                          genrelisteditor.h  -  description
                             -------------------
    begin                : Sun Dec 8 2002
    copyright            : (C) 2002 by Scott Wheeler
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

#ifndef GENRELISTEDITOR_H
#define GENRELISTEDITOR_H

#include <qdict.h>

#include "genrelisteditorbase.h"

class GenreList;

class GenreListEditor : public GenreListEditorBase 
{
    Q_OBJECT

public: 
    GenreListEditor(QWidget *parent = 0, const char *name = 0);
    ~GenreListEditor();

private:
    void loadID3v1Genres();
    void loadLists();
    void updateGenreList();

private slots:
    virtual void slotUpdateGenreBoxes(QListViewItem *item);
    virtual void slotUpdateGenreName(const QString &name);

private:
    QDict<GenreList> m_listDict;
};

#endif
