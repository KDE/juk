/***************************************************************************
                          searchwidget.h
                             -------------------
    begin                : Sun Mar 6 2003
    copyright            : (C) 2003 by Scott Wheeler <wheeler@kde.org>
                           (C) 2003 by Richard Lärkäng <nouseforaname@home.se>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <qwidget.h>
#include <qhbox.h>

#include "playlistsearch.h"

class QCheckBox;

class KComboBox;

class Playlist;

class SearchLine : public QHBox
{
    Q_OBJECT

public:
    enum Mode { Default = 0, CaseSensitive = 1, Pattern = 2 };

    SearchLine(QWidget *parent, const char *name = 0);
    virtual ~SearchLine() {}

    PlaylistSearch::Component searchComponent() const;
    void setSearchComponent(const PlaylistSearch::Component &component);

    void updateColumns();
    void clear();

signals:
    void signalQueryChanged();

private:
    KLineEdit *m_lineEdit;
    KComboBox *m_searchFieldsBox;
    KComboBox *m_caseSensitive;
    QValueList<int> m_columnList;
};

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    SearchWidget(QWidget *parent, const char *name = 0);
    virtual ~SearchWidget();

    /**
     * Returns a list of searched columns for the given search row.
     */
    
    PlaylistSearch search(const PlaylistList &playlists) const;
    void setSearch(const PlaylistSearch &search);

public slots:
    void clear();

signals:
    void signalQueryChanged();

private:
    void updateColumns();
    void setupLayout();

private:
    SearchLine *m_searchLine;
    QStringList m_columnHeaders;
};

#endif
