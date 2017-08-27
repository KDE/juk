/**
 * Copyright (C) 2003-2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2017 Michael Pyne <mpyne@kde.org>
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

#include "advancedsearchdialog.h"

#include <kcombobox.h>
#include <klocale.h>
#include <kvbox.h>
#include <KStandardGuiItem>

#include <QDialogButtonBox>
#include <QRadioButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QLineEdit>

#include "collectionlist.h"
#include "searchwidget.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

AdvancedSearchDialog::AdvancedSearchDialog(const QString &defaultName,
                                           const PlaylistSearch &defaultSearch,
                                           QWidget *parent) :
    QDialog(parent)
{
    setWindowTitle(i18n("Create Search Playlist"));
    setObjectName(QStringLiteral("juk_advSrchDlg"));

    auto mw = new QVBoxLayout(this);
    setLayout(mw);

    auto box = new QHBoxLayout;
    mw->addLayout(box);

    box->addWidget(new QLabel(i18n("Playlist name:")));
    m_playlistNameLineEdit = new QLineEdit(defaultName);
    box->addWidget(m_playlistNameLineEdit);

    auto criteriaGroupBox = new QGroupBox(i18n("Search Criteria"));
    mw->addWidget(criteriaGroupBox, 1);
    m_criteriaLayout = new QVBoxLayout(criteriaGroupBox);

    auto group = new QGroupBox;
    m_matchAnyButton = new QRadioButton(i18n("Match any of the following"));
    m_matchAllButton = new QRadioButton(i18n("Match all of the following"));

    QHBoxLayout *hgroupbox = new QHBoxLayout(group);
    hgroupbox->addWidget(m_matchAnyButton);
    hgroupbox->addWidget(m_matchAllButton);

    m_criteriaLayout->addWidget(group);
    m_criteriaLayout->addStretch(1); // more()/fewer() assume this is here

    QWidget *buttons = new QWidget;
    mw->addWidget(buttons);

    QHBoxLayout *l = new QHBoxLayout(buttons);
    l->setSpacing(5);
    l->setMargin(0);

    const auto &clearGuiItem = KStandardGuiItem::clear();
    QPushButton *clearButton = new QPushButton(clearGuiItem.icon(), clearGuiItem.text());
    connect(clearButton, &QPushButton::clicked,
            this, &AdvancedSearchDialog::clearSearches);
    l->addWidget(clearButton);

    l->addStretch(1);

    m_moreButton = new QPushButton(i18nc("additional search options", "More"));
    connect(m_moreButton, &QPushButton::clicked,
            this, &AdvancedSearchDialog::more);
    l->addWidget(m_moreButton);

    m_fewerButton = new QPushButton(i18n("Fewer"));
    connect(m_fewerButton, &QPushButton::clicked,
            this, &AdvancedSearchDialog::fewer);
    l->addWidget(m_fewerButton);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mw->addWidget(buttonBox);

    if(defaultSearch.isNull()) {
        this->more();
        this->more(); // Create first 2 searches
        m_matchAnyButton->setChecked(true);
    }
    else {
        PlaylistSearch::ComponentList components = defaultSearch.components();

        for(PlaylistSearch::ComponentList::ConstIterator it = components.constBegin();
            it != components.constEnd();
            ++it)
        {
            SearchLine *s = new SearchLine(this);
            s->setSearchComponent(*it);
            m_searchLines.append(s);
            m_criteriaLayout->insertWidget(m_criteriaLayout->count() - 1, s);
        }

        if(defaultSearch.searchMode() == PlaylistSearch::MatchAny)
            m_matchAnyButton->setChecked(true);
        else
            m_matchAllButton->setChecked(true);
    }

    m_playlistNameLineEdit->setFocus();
}

////////////////////////////////////////////////////////////////////////////////
// protected slots
////////////////////////////////////////////////////////////////////////////////

void AdvancedSearchDialog::accept()
{
    m_search.clearPlaylists();
    m_search.clearComponents();

    m_search.addPlaylist(CollectionList::instance());

    for(const auto &searchLine : m_searchLines)
        m_search.addComponent(searchLine->searchComponent());

    PlaylistSearch::SearchMode m = PlaylistSearch::SearchMode(!m_matchAnyButton->isChecked());
    m_search.setSearchMode(m);

    m_playlistName = m_playlistNameLineEdit->text();

    QDialog::accept();
}

void AdvancedSearchDialog::clearSearches()
{
    for(auto &searchLine : m_searchLines)
        searchLine->clear();
}

void AdvancedSearchDialog::more()
{
    SearchLine *searchLine = new SearchLine(this);
    // inserting it to keep the trailing stretch item at end
    m_criteriaLayout->insertWidget(m_criteriaLayout->count() - 1, searchLine);
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

// vim: set et sw=4 tw=0 sta:
