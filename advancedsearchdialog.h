/**
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

#ifndef ADVANCEDSEARCHDIALOG_H
#define ADVANCEDSEARCHDIALOG_H

#include <QDialog>
#include <QList>

#include "playlistsearch.h"

class QLineEdit;
class QPushButton;
class QRadioButton;
class SearchLine;
class QBoxLayout;

class AdvancedSearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AdvancedSearchDialog(
            const QString& defaultName,
            PlaylistSearch& defaultSearch,
            QWidget* parent = nullptr);

    PlaylistSearch* resultSearch() const
    {
        return m_search;
    }

    QString resultPlaylistName() const
    {
        return m_playlistName;
    }

protected slots:
    void accept() Q_DECL_OVERRIDE;
    void clearSearches();
    void more();
    void fewer();

private:
    void updateButtons();

    QBoxLayout *m_criteriaLayout;
    PlaylistSearch* m_search;
    QString m_playlistName;
    QList<SearchLine *> m_searchLines;
    QLineEdit *m_playlistNameLineEdit;
    QRadioButton *m_matchAnyButton;
    QRadioButton *m_matchAllButton;
    QPushButton *m_moreButton;
    QPushButton *m_fewerButton;
};

#endif

// vim: set et sw=4 tw=0 sta:
