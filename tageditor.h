/***************************************************************************
                          tageditor.h  -  description
                             -------------------
    begin                : Sat Sep 7 2002
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

#ifndef TAGEDITOR_H
#define TAGEDITOR_H

#include <qmap.h>

#include "playlistitem.h"
#include "genrelist.h"

class KComboBox;
class KLineEdit;
class KIntSpinBox;
class KEdit;

class TagEditor : public QWidget
{
    Q_OBJECT

public: 
    TagEditor(QWidget *parent = 0, const char *name = 0);
    virtual ~TagEditor();
    void setGenreList(const GenreList &list);

public slots:
    void setItems(const PlaylistItemList &list);
    void refresh();
    void clear();
    void save();
    void updateCollection();
    
signals:
    void changed();

private:
    void setupLayout();
    void readConfig();
    void saveConfig();
    void save(const PlaylistItemList &list);
    void saveChangesPrompt();
    /**
     * Adds an item to JuK's tagging layout.  This handles the creation and
     * placement of the "enable" box as well.
     */
    void addItem(const QString &text, QWidget *item, QBoxLayout *layout);

    typedef QMap<QWidget *, QCheckBox *> BoxMap;
    BoxMap enableBoxes;

    GenreList genreList;

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

    PlaylistItemList items;
    
    bool dataChanged;

private slots:
    void setDataChanged(bool c = true);

};

#endif
