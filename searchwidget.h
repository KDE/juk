/**
 * Copyright (C) 2003 Richard Lärkäng <nouseforaname@home.se>
 * Copyright (C) 2003-2004 Scott Wheeler <wheeler@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef JUK_SEARCHWIDGET_H
#define JUK_SEARCHWIDGET_H

#include <QWidget>
#include <QList>
#include <QLineEdit>

#include "playlistsearch.h"

class QEvent;
class QComboBox;

class SearchWidget;

class SearchLine : public QWidget
{
    Q_OBJECT

    friend class SearchWidget;

public:
    enum Mode { Default = 0, CaseSensitive = 1, Pattern = 2 };

    explicit SearchLine(QWidget *parent, bool simple = false);

    PlaylistSearch::Component searchComponent() const;
    void setSearchComponent(const PlaylistSearch::Component &component);

    void updateColumns();

public slots:
    void clear();

protected:
    virtual bool eventFilter(QObject *watched, QEvent *e) override;

signals:
    void signalQueryChanged();
    void signalDownPressed();
    void returnPressed();

private slots:
    void slotActivate();

private:
    bool m_simple;
    QLineEdit *m_lineEdit;
    QComboBox *m_searchFieldsBox;
    QComboBox *m_caseSensitive;
    QList<int> m_columnList;
};

class SearchWidget : public SearchLine
{
    Q_OBJECT

public:
    explicit SearchWidget(QWidget *parent);

    PlaylistSearch* search(const PlaylistList& playlists) const;
    PlaylistSearch* search(Playlist *playlist) const;
    void setSearch(const PlaylistSearch* search);

    virtual QString searchText() const;
    virtual void setSearchText(const QString &text);

public slots:
    void setEnabled(bool enable);

signals:
    // This signal is only emitted when the Show/Hide action is triggered.
    // Minimizing/closing the JuK window will not trigger this signal.

    void signalShown(bool shown);
};

#endif

// vim: set et sw=4 tw=0 sta:
