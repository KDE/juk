/***************************************************************************
                          statuslabel.h  -  description
                             -------------------
    begin                : Fri Oct 18 2002
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

#ifndef STATUSLABEL_H
#define STATUSLABEL_H

#include <qwidget.h>
#include <qhbox.h>
#include <qlabel.h>

class PlaylistItem;

class StatusLabel : public QHBox
{
    Q_OBJECT

public: 
    StatusLabel(QWidget *parent = 0, const char *name = 0);
    virtual ~StatusLabel();

    void setPlayingItem(PlaylistItem *item);
    void clear();

private:
    virtual bool eventFilter(QObject *o, QEvent *e);

    QLabel *trackLabel;
    PlaylistItem *playingItem;
};

#endif
