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

#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <ktoolbar.h>

#include <QFrame>
#include <QList>

#include "playlistsearch.h"

class QEvent;

class KComboBox;
class KLineEdit;
class SearchWidget;

class SearchLine : public QFrame
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
    virtual void setFocus();

protected:
    virtual bool eventFilter(QObject *watched, QEvent *e);

signals:
    void signalQueryChanged();
    void signalDownPressed();

private slots:
    void slotActivate();

private:
    bool m_simple;
    KLineEdit *m_lineEdit;
    KComboBox *m_searchFieldsBox;
    KComboBox *m_caseSensitive;
    QList<int> m_columnList;
};

class SearchWidget : public KToolBar
{
    Q_OBJECT

public:
    explicit SearchWidget(QWidget *parent);

    PlaylistSearch search(const PlaylistList &playlists) const;
    void setSearch(const PlaylistSearch &search);

    virtual QString searchText() const;
    virtual void setSearchText(const QString &text);

public slots:
    void clear();
    void setEnabled(bool enable);
    virtual void setFocus();

signals:
    void signalQueryChanged();
    void returnPressed();

    // This signal is only emitted when the Show/Hide action is triggered.
    // Minimizing/closing the JuK window will not trigger this signal.

    void signalShown(bool shown);

    void signalDownPressed();

private:
    void updateColumns();

private:
    SearchLine m_searchLine;
    QStringList m_columnHeaders;
};

#endif

// vim: set et sw=4 tw=0 sta:
