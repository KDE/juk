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

class QCheckBox;

class KLineEdit;
class KComboBox;

class Playlist;
class PlaylistSearch;

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    enum Mode { Default = 0, CaseSensitive = 1, Pattern = 2 };
    /**
     * Note that playlist here is just a playlist to get the columns from and
     * has nothing to do with the results of a search.
     */
    SearchWidget(QWidget *parent, const Playlist *playlist, const char *name);
    virtual ~SearchWidget();

    /**
     * Returns a list of searched columns for the given search row.
     */
    QValueList<int> searchedColumns(int searchLine = 0) { return m_searchedColumns[searchLine]; }

    QString query() const;
    bool caseSensitive() const;
    bool regExp() const;
    void setSearch(const PlaylistSearch &search);

public slots:
    void clear();
    void slotUpdateColumns();
    void slotQueryChanged(int = 0);

signals:
    void signalQueryChanged(const QString &query, bool caseSensitive, bool regExp);

private:
    void setupLayout();

private:
    const Playlist *m_playlist;
    KLineEdit *m_lineEdit;
    KComboBox *m_searchFieldsBox;
    KComboBox *m_caseSensitive;
    QStringList m_columnHeaders;
    QValueList< QValueList<int> > m_searchedColumns;
};

#endif
