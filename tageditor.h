/***************************************************************************
                          tageditor.h  -  description
                             -------------------
    begin                : Sat Sep 7 2002
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

#ifndef TAGEDITOR_H
#define TAGEDITOR_H

#include <kcombobox.h>
#include <klineedit.h>
#include <knuminput.h>
#include <keditcl.h>

#include "tag.h"
#include "playlist.h"
#include "playlistitem.h"
#include "genrelist.h"

class TagEditor : public QWidget
{
    Q_OBJECT

public: 
    TagEditor(QWidget *parent = 0, const char *name = 0);
    virtual ~TagEditor();

public slots:
    void setItems(const QPtrList<PlaylistItem> &list);
    void refresh();
    void clear();
    void save();
    void updateCollection(Playlist *list);
    
signals:
    void changed();

private:
    void setupLayout();
    void readConfig();
    void saveConfig();
    void save(const QPtrList<PlaylistItem> &list);
    void saveChangesPrompt();

    GenreList *genreList;

    KComboBox *artistNameBox;
    KLineEdit *trackNameBox;
    KComboBox *albumNameBox;
    KComboBox *genreBox;
    KLineEdit *fileNameBox;
    KIntSpinBox *trackSpin;
    KIntSpinBox *yearSpin;
    KLineEdit *lengthBox;
    KLineEdit *bitrateBox;
    KEdit *commentBox;

    QPtrList<PlaylistItem> items;
    
    bool dataChanged;

private slots:
    void setDataChanged(bool c = true);

};

#endif
