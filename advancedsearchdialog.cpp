/***************************************************************************
                          advancedsearchdialog.cpp
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

#include <kcombobox.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <klocale.h>

#include <qradiobutton.h>
#include <qvgroupbox.h>
#include <qlabel.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qlayout.h>

#include "advancedsearchdialog.h"

AdvancedSearchDialog::AdvancedSearchDialog(QWidget *parent, const char *name) :
    KDialogBase(parent, name, true, i18n("Create Search Playlist"), Ok|Cancel)
{
    makeVBoxMainWidget();

    QHBox *box = new QHBox(mainWidget());
    box->setSpacing(5);

    new QLabel(i18n("Playlist name"), box);
    new KLineEdit(box);

    QVGroupBox *criteriaGroupBox = new QVGroupBox(i18n("Search Criteria"), mainWidget());

    box = new QHBox(criteriaGroupBox);
    new QRadioButton(i18n("Match any of the following"), box);
    new QRadioButton(i18n("Match all of the following"), box);

    m_criteria = new QVBox(criteriaGroupBox);

    searchLine();
    searchLine();

    QWidget *buttons = new QWidget(criteriaGroupBox);
    QBoxLayout *l = new QHBoxLayout(buttons, 0, 5);

    l->addWidget(new KPushButton(i18n("Clear"), buttons));

    l->addStretch(1);

    l->addWidget(new KPushButton(i18n("More"), buttons));
    l->addWidget(new KPushButton(i18n("Fewer"), buttons));
}

AdvancedSearchDialog::~AdvancedSearchDialog()
{

}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

QWidget *AdvancedSearchDialog::searchLine()
{
    QHBox *box = new QHBox(m_criteria);
    box->setSpacing(5);

    new KComboBox(box);

    new KLineEdit(box);

    KComboBox *caseSensitive = new KComboBox(box);
    caseSensitive->insertItem(i18n("Normal Matching"), 0);
    caseSensitive->insertItem(i18n("Case Sensitive"), 1);
    caseSensitive->insertItem(i18n("Pattern Matching"), 2);

    return box;
}
