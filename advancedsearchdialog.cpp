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
#include <qhbuttongroup.h>

#include "collectionlist.h"
#include "advancedsearchdialog.h"
#include "searchwidget.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

AdvancedSearchDialog::AdvancedSearchDialog(QWidget *parent, const char *name) :
    KDialogBase(parent, name, true, i18n("Create Search Playlist"), Ok|Cancel)
{
    makeVBoxMainWidget();

    QHBox *box = new QHBox(mainWidget());
    box->setSpacing(5);

    new QLabel(i18n("Playlist name:"), box);
    new KLineEdit(box);

    QVGroupBox *criteriaGroupBox = new QVGroupBox(i18n("Search Criteria"), mainWidget());

    QHButtonGroup *group = new QHButtonGroup(criteriaGroupBox);
    m_matchAnyButton = new QRadioButton(i18n("Match any of the following"), group);
    m_matchAllButton = new QRadioButton(i18n("Match all of the following"), group);
    m_matchAnyButton->setChecked(true);

    m_criteria = new QVBox(criteriaGroupBox);

    m_searchLines.append(new SearchLine(m_criteria));
    m_searchLines.append(new SearchLine(m_criteria));

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
// public slots
////////////////////////////////////////////////////////////////////////////////

AdvancedSearchDialog::Result AdvancedSearchDialog::exec()
{
    Result r;
    r.result = DialogCode(KDialogBase::exec());
    r.search = m_search;
    return r;
}

////////////////////////////////////////////////////////////////////////////////
// protected slots
////////////////////////////////////////////////////////////////////////////////

void AdvancedSearchDialog::accept()
{
    m_search.clearPlaylists();
    m_search.clearComponents();

    m_search.addPlaylist(CollectionList::instance());

    QValueListConstIterator<SearchLine *> it = m_searchLines.begin();
    for(; it != m_searchLines.end(); ++it)
        m_search.addComponent((*it)->searchComponent());

    PlaylistSearch::SearchMode m = PlaylistSearch::SearchMode(!m_matchAnyButton->isChecked());
    m_search.setSearchMode(m);

    KDialogBase::accept();
}

#include "advancedsearchdialog.moc"
