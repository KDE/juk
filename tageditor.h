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

#include <qwidget.h>
#include <qmap.h>

#include "playlistitem.h"
#include "genrelist.h"

class KComboBox;
class KLineEdit;
class KIntSpinBox;
class KEdit;
class KPushButton;

class QCheckBox;
class QBoxLayout;

class TagEditor : public QWidget
{
    Q_OBJECT

public: 
    TagEditor(QWidget *parent = 0, const char *name = 0);
    virtual ~TagEditor();
    void setGenreList(const GenreList &list);
    void save() { save(m_items); }

public slots:
    void slotSetItems(const PlaylistItemList &list);
    void slotRefresh();
    void slotClear();
    void slotUpdateCollection();
   
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

    virtual void showEvent(QShowEvent *e);

private slots:
    void slotDataChanged(bool c = true);

private:
    typedef QMap<QWidget *, QCheckBox *> BoxMap;
    BoxMap m_enableBoxes;

    GenreList m_genreList;

    KComboBox *m_artistNameBox;
    KLineEdit *m_trackNameBox;
    KComboBox *m_albumNameBox;
    KComboBox *m_genreBox;
    KLineEdit *m_fileNameBox;
    KIntSpinBox *m_trackSpin;
    KIntSpinBox *m_yearSpin;
    KLineEdit *m_lengthBox;
    KLineEdit *m_bitrateBox;
    KEdit *m_commentBox;

    PlaylistItemList m_items;
    
    bool m_dataChanged; 
};

#endif
