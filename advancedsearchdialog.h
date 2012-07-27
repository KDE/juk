/***************************************************************************
    begin                : Thu Jul 31 00:31:51 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler
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

#ifndef ADVANCEDSEARCHDIALOG_H
#define ADVANCEDSEARCHDIALOG_H

#include <kdialog.h>
#include <QList>

#include "playlist/playlistsearch.h"

class KLineEdit;
class KPushButton;
class QRadioButton;
class SearchLine;
class QBoxLayout;

class AdvancedSearchDialog : public KDialog
{
    Q_OBJECT

public:
    struct Result
    {
        DialogCode result;
        PlaylistSearch search;
        QString playlistName;
    };

    explicit AdvancedSearchDialog(const QString &defaultName,
                         const PlaylistSearch &defaultSearch = PlaylistSearch(),
                         QWidget *parent = 0,
                         const char *name = 0);

    virtual ~AdvancedSearchDialog();

public slots:
    Result exec();

protected slots:
    virtual void accept();
    virtual void clear();
    virtual void more();
    virtual void fewer();

private:
    void updateButtons();

    QBoxLayout *m_criteriaLayout;
    PlaylistSearch m_search;
    QString m_playlistName;
    QList<SearchLine *> m_searchLines;
    KLineEdit *m_playlistNameLineEdit;
    QRadioButton *m_matchAnyButton;
    QRadioButton *m_matchAllButton;
    KPushButton *m_moreButton;
    KPushButton *m_fewerButton;
};

#endif

// vim: set et sw=4 tw=0 sta:
