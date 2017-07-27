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

#include "advancedsearchdialog.h"

#include <kcombobox.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <klocale.h>
#include <kvbox.h>

#include <QRadioButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>

#include "collectionlist.h"
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
    setObjectName( QLatin1String( name ) );
    setModal(true);

    KVBox *mw = new KVBox(this);
    setMainWidget(mw);

    KHBox *box = new KHBox(mw);
    box->setSpacing(5);

    new QLabel(i18n("Playlist name:"), box);
    m_playlistNameLineEdit = new KLineEdit(defaultName, box);

    QGroupBox *criteriaGroupBox = new QGroupBox(i18n("Search Criteria"), mw);
    mw->setStretchFactor(criteriaGroupBox, 1);

    m_criteriaLayout = new QVBoxLayout;

    QGroupBox *group = new QGroupBox();

    m_matchAnyButton = new QRadioButton(i18n("Match any of the following"));
    m_matchAllButton = new QRadioButton(i18n("Match all of the following"));

    QHBoxLayout *hgroupbox = new QHBoxLayout;
    hgroupbox->addWidget(m_matchAnyButton);
    hgroupbox->addWidget(m_matchAllButton);

    group->setLayout(hgroupbox);

    m_criteriaLayout->addWidget(group);

    if(defaultSearch.isNull()) {
        SearchLine *newSearchLine = new SearchLine(this);
        m_searchLines.append(newSearchLine);
        m_criteriaLayout->addWidget(newSearchLine);
        newSearchLine = new SearchLine(this);
        m_searchLines.append(newSearchLine);
        m_criteriaLayout->addWidget(newSearchLine);
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
            m_criteriaLayout->addWidget(s);
        }
        if(defaultSearch.searchMode() == PlaylistSearch::MatchAny)
            m_matchAnyButton->setChecked(true);
        else
            m_matchAllButton->setChecked(true);
    }

    QWidget *buttons = new QWidget(mw);
    QHBoxLayout *l = new QHBoxLayout(buttons);
    l->setSpacing(5);
    l->setMargin(0);

    KPushButton *clearButton = new KPushButton(KStandardGuiItem::clear(), buttons);
    connect(clearButton, SIGNAL(clicked()), SLOT(clear()));
    l->addWidget(clearButton);

    l->addStretch(1);

    m_moreButton = new KPushButton(i18nc("additional search options", "More"), buttons);
    connect(m_moreButton, SIGNAL(clicked()), SLOT(more()));
    l->addWidget(m_moreButton);

    m_fewerButton = new KPushButton(i18n("Fewer"), buttons);
    connect(m_fewerButton, SIGNAL(clicked()), SLOT(fewer()));
    l->addWidget(m_fewerButton);

    m_criteriaLayout->addStretch(1);

    criteriaGroupBox->setLayout(m_criteriaLayout);

    m_playlistNameLineEdit->setFocus();
}

AdvancedSearchDialog::~AdvancedSearchDialog()
{

}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

//AdvancedSearchDialog::Result AdvancedSearchDialog::exec()
int AdvancedSearchDialog::exec()
{
    return 0; // FIXME signal
    Result r;
    r.result = DialogCode(KDialog::exec());
    r.search = m_search;
    r.playlistName = m_playlistName;
    //return r;
}

////////////////////////////////////////////////////////////////////////////////
// protected slots
////////////////////////////////////////////////////////////////////////////////

void AdvancedSearchDialog::accept()
{
    m_search.clearPlaylists();
    m_search.clearComponents();

    m_search.addPlaylist(CollectionList::instance());

    QList<SearchLine *>::const_iterator it = m_searchLines.constBegin();
    for(; it != m_searchLines.constEnd(); ++it)
        m_search.addComponent((*it)->searchComponent());

    PlaylistSearch::SearchMode m = PlaylistSearch::SearchMode(!m_matchAnyButton->isChecked());
    m_search.setSearchMode(m);

    m_playlistName = m_playlistNameLineEdit->text();

    KDialog::accept();
}

void AdvancedSearchDialog::clear()
{
    QList<SearchLine *>::const_iterator it = m_searchLines.constBegin();
    for(; it != m_searchLines.constEnd(); ++it)
        (*it)->clear();
}

void AdvancedSearchDialog::more()
{
    SearchLine *searchLine = new SearchLine(this);
    m_criteriaLayout->addWidget(searchLine);
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
