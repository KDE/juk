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

AdvancedSearchDialog::AdvancedSearchDialog(const QString &defaultName,
					   const PlaylistSearch &defaultSearch,
                                           QWidget *parent,
                                           const char *name) :
    KDialogBase(parent, name, true, i18n("Create Search Playlist"), Ok|Cancel)
{
    makeVBoxMainWidget();

    QHBox *box = new QHBox(mainWidget());
    box->setSpacing(5);

    new QLabel(i18n("Playlist name:"), box);
    m_playlistNameLineEdit = new KLineEdit(defaultName, box);

    QVGroupBox *criteriaGroupBox = new QVGroupBox(i18n("Search Criteria"), mainWidget());
    static_cast<QHBox *>(mainWidget())->setStretchFactor(criteriaGroupBox, 1);

    QHButtonGroup *group = new QHButtonGroup(criteriaGroupBox);
    m_matchAnyButton = new QRadioButton(i18n("Match any of the following"), group);
    m_matchAllButton = new QRadioButton(i18n("Match all of the following"), group);

    m_criteria = new QVBox(criteriaGroupBox);

    if(defaultSearch.isNull()) {
        m_searchLines.append(new SearchLine(m_criteria));
        m_searchLines.append(new SearchLine(m_criteria));
        m_matchAnyButton->setChecked(true);
    }
    else {
        PlaylistSearch::ComponentList components = defaultSearch.components();
        for(PlaylistSearch::ComponentList::ConstIterator it = components.begin();
            it != components.end();
            ++it)
        {
            SearchLine *s = new SearchLine(m_criteria);
            s->setSearchComponent(*it);
            m_searchLines.append(s);
        }
        if(defaultSearch.searchMode() == PlaylistSearch::MatchAny)
            m_matchAnyButton->setChecked(true);
        else
            m_matchAllButton->setChecked(true);
    }

    QWidget *buttons = new QWidget(criteriaGroupBox);
    QBoxLayout *l = new QHBoxLayout(buttons, 0, 5);

    KPushButton *clearButton = new KPushButton(KStdGuiItem::clear(), buttons);
    connect(clearButton, SIGNAL(clicked()), SLOT(clear()));
    l->addWidget(clearButton);

    l->addStretch(1);

    m_moreButton = new KPushButton(i18n("More"), buttons);
    connect(m_moreButton, SIGNAL(clicked()), SLOT(more()));
    l->addWidget(m_moreButton);

    m_fewerButton = new KPushButton(i18n("Fewer"), buttons);
    connect(m_fewerButton, SIGNAL(clicked()), SLOT(fewer()));
    l->addWidget(m_fewerButton);

    m_playlistNameLineEdit->setFocus();
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
    r.playlistName = m_playlistName;
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

    m_playlistName = m_playlistNameLineEdit->text();

    KDialogBase::accept();
}

void AdvancedSearchDialog::clear()
{
    QValueListConstIterator<SearchLine *> it = m_searchLines.begin();
    for(; it != m_searchLines.end(); ++it)
        (*it)->clear();
}

void AdvancedSearchDialog::more()
{
    SearchLine *searchLine = new SearchLine(m_criteria);
    m_searchLines.append(searchLine);
    searchLine->show();
    updateButtons();
}

void AdvancedSearchDialog::fewer()
{
    SearchLine *searchLine = m_searchLines.last();
    m_searchLines.remove(searchLine);
    delete searchLine;
    updateButtons();
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void AdvancedSearchDialog::updateButtons()
{
    m_moreButton->setEnabled(m_searchLines.count() < 16);
    m_fewerButton->setEnabled(m_searchLines.count() > 1);
}

#include "advancedsearchdialog.moc"
