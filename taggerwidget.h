/***************************************************************************
                     taggerwidget.h  -  description
                             -------------------
    begin                : Tue Feb 5 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef TAGGERWIDGET_H
#define TAGGERWIDGET_H

#include <qptrlist.h>

#include "playlist.h"
#include "playlistitem.h"
#include "genrelist.h"
#include "tageditor.h"

class TaggerWidget : public QWidget
{
    Q_OBJECT

public:
    TaggerWidget(QWidget *parent);
    virtual ~TaggerWidget();

    void add(const QString &item);
    void add(const QStringList &items);

    Playlist *getTaggerList();

    QPtrList<PlaylistItem> getSelectedItems();

public slots:
    void save();
    void remove();
    void remove(const QPtrList<PlaylistItem> &items);

private:
    void setupLayout();

    // main visual objects
    Playlist *taggerList;
    TagEditor *editor;
};

#endif
