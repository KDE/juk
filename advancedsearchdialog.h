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

class QGroupBox;

class AdvancedSearchDialog : public KDialogBase
{
public:
    AdvancedSearchDialog(QWidget *parent = 0, const char *name = 0);
    virtual ~AdvancedSearchDialog();

private:
    QWidget *searchLine();

    QWidget *m_criteria;
};

#endif
