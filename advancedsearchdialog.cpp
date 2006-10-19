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
#include <kvbox.h>

#include <QRadioButton>
#include <QLabel>
#include <QLayout>

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QBoxLayout>
#include <QGroupBox>

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
    KDialog(parent)
{
    setCaption( i18n("Create Search Playlist") );
    setButtons( Ok|Cancel );
    setDefaultButton( Ok );
    setObjectName(name);
    setModal(true);

    KVBox *mw = new KVBox(this);
    setMainWidget(mw);

    KHBox *box = new KHBox(mw);
    box->setSpacing(5);

    new QLabel(i18n("Playlist name:"), box);
    m_playlistNameLineEdit = new KLineEdit(defaultName, box);

    QGroupBox *criteriaGroupBox = new QGroupBox(i18n("Search Criteria"), mw);
    mw->setStretchFactor(criteriaGroupBox, 1);

    QVBoxLayout *criteriaLayout = new QVBoxLayout;

    QGroupBox *group = new QGroupBox();

    
    m_matchAnyButton = new QRadioButton(i18n("Match any of the following"));
    m_matchAllButton = new QRadioButton(i18n("Match all of the following"));

    QHBoxLayout *hgroupbox = new QHBoxLayout;
    hgroupbox->addWidget(m_matchAnyButton);
    hgroupbox->addWidget(m_matchAllButton);

    group->setLayout(hgroupbox);

    criteriaLayout->addWidget(group);

    if(defaultSearch.isNull()) {
        SearchLine *newSearchLine = new SearchLine(0);
        m_searchLines.append(newSearchLine);
        criteriaLayout->addWidget(newSearchLine);
        newSearchLine = new SearchLine(0);
        m_searchLines.append(newSearchLine);
        criteriaLayout->addWidget(newSearchLine);
        m_matchAnyButton->setChecked(true);
    }
    else {
        PlaylistSearch::ComponentList components = defaultSearch.components();
        for(PlaylistSearch::ComponentList::ConstIterator it = components.begin();
            it != components.end();
            ++it)
        {
            SearchLine *s = new SearchLine(0);
            s->setSearchComponent(*it);
            m_searchLines.append(s);
            criteriaLayout->addWidget(s);
        }
        if(defaultSearch.searchMode() == PlaylistSearch::MatchAny)
            m_matchAnyButton->setChecked(true);
        else
            m_matchAllButton->setChecked(true);
    }

    QWidget *buttons = new QWidget();
    QHBoxLayout *l = new QHBoxLayout(buttons);
    l->setSpacing(5);
    l->setMargin(0);

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

    criteriaLayout->addWidget(buttons);

    criteriaLayout->addStretch(1);

    criteriaGroupBox->setLayout(criteriaLayout);

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
    r.result = DialogCode(KDialog::exec());
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

    QList<SearchLine *>::const_iterator it = m_searchLines.begin();
    for(; it != m_searchLines.end(); ++it)
        m_search.addComponent((*it)->searchComponent());

    PlaylistSearch::SearchMode m = PlaylistSearch::SearchMode(!m_matchAnyButton->isChecked());
    m_search.setSearchMode(m);

    m_playlistName = m_playlistNameLineEdit->text();

    KDialog::accept();
}

void AdvancedSearchDialog::clear()
{
    QList<SearchLine *>::const_iterator it = m_searchLines.begin();
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
    m_searchLines.removeAll(searchLine);
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

// vim: set et sw=4 tw=0 sta:
