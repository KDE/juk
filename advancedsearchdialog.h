/***************************************************************************
                          advancedsearchdialog.h
                             -------------------
    begin                : Thu Jul 31 00:31:51 2003
    copyright            : (C) 2003 by Scott Wheeler
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

#include <kdialogbase.h>

#include "playlistsearch.h"

class QGroupBox;
class QRadioButton;
class SearchLine;

class AdvancedSearchDialog : public KDialogBase
{
    Q_OBJECT

public:
    struct Result
    {
        DialogCode result;
        PlaylistSearch search;
    };

    AdvancedSearchDialog(QWidget *parent = 0, const char *name = 0);
    virtual ~AdvancedSearchDialog();

public slots:
    Result exec();

protected slots:
    virtual void accept();
    
private:
    QWidget *m_criteria;
    PlaylistSearch m_search;
    QValueList<SearchLine *> m_searchLines;
    QRadioButton *m_matchAnyButton;
    QRadioButton *m_matchAllButton;
};

#endif
