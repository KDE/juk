/***************************************************************************
                          searchwidget.cpp
                             -------------------
    begin                : Sun Mar 6 2003
    copyright            : (C) 2003 by Scott Wheeler <wheeler@kde.org>
                           (C) 2003 by Richard Lärkäng <nouseforaname@home.se>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <klocale.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <kdebug.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qpushbutton.h>

#include "searchwidget.h"
#include "collectionlist.h"

////////////////////////////////////////////////////////////////////////////////
// SearchLine public methods
////////////////////////////////////////////////////////////////////////////////

SearchLine::SearchLine(QWidget *parent, const char *name) : QHBox(parent, name)
{
    setSpacing(5);

    m_searchFieldsBox = new KComboBox(this, "searchFields");
    connect(m_searchFieldsBox, SIGNAL(activated(int)),
            this, SIGNAL(signalQueryChanged()));

    m_lineEdit = new KLineEdit(this, "searchLineEdit");
    connect(m_lineEdit, SIGNAL(textChanged(const QString &)),
            this, SIGNAL(signalQueryChanged()));

    m_caseSensitive = new KComboBox(this);
    m_caseSensitive->insertItem(i18n("Normal Matching"), 0);
    m_caseSensitive->insertItem(i18n("Case Sensitive"), 1);
    m_caseSensitive->insertItem(i18n("Pattern Matching"), 2);
    connect(m_caseSensitive, SIGNAL(activated(int)),
            this, SIGNAL(signalQueryChanged()));

    updateColumns();
}

PlaylistSearch::Component SearchLine::searchComponent() const
{
    QString query = m_lineEdit->text();
    bool caseSensitive = m_caseSensitive->currentItem() == CaseSensitive;

    Playlist *playlist = CollectionList::instance();

    QValueList<int> searchedColumns;

    if(m_searchFieldsBox->currentItem() == 0) {
	QValueListConstIterator<int> it = m_columnList.begin();
	for(; it != m_columnList.end(); ++it) {
	    if(playlist->isColumnVisible(*it))
		searchedColumns.append(*it);
	}
    }
    else
	searchedColumns.append(m_columnList[m_searchFieldsBox->currentItem() - 1]);

    return PlaylistSearch::Component(query, caseSensitive, searchedColumns);
}

void SearchLine::setSearchComponent(const PlaylistSearch::Component &component)
{
    if(component == searchComponent())
	return;

    if(!component.isPatternSearch()) {
	m_lineEdit->setText(component.query());
	m_caseSensitive->setCurrentItem(component.isCaseSensitive() ? CaseSensitive : Default);
    }
    else {
	m_lineEdit->setText(component.pattern().pattern());
	m_caseSensitive->setCurrentItem(Pattern);
    }
}

void SearchLine::clear()
{
    // We don't want to emit the signal if it's already empty.
    if(!m_lineEdit->text().isEmpty())
	m_lineEdit->clear();
}

void SearchLine::updateColumns()
{
    QString currentText = m_searchFieldsBox->currentText();
    m_searchFieldsBox->clear();

    QStringList columnHeaders;

    columnHeaders.append(QString("<%1>").arg(i18n("All Visible")));

    Playlist *playlist = CollectionList::instance();

    int selection = -1;
    m_columnList.clear();

    for(int i = 0; i < playlist->columns(); i++) {
	if(playlist->isColumnVisible(i)) {
	    m_columnList.append(i);
	    QString text = playlist->columnText(i);
	    columnHeaders.append(text);
	    if(currentText == text)
		selection = m_columnList.size() - 1;
	}
    }

    m_searchFieldsBox->insertStringList(columnHeaders);
    m_searchFieldsBox->setCurrentItem(selection + 1);
}

////////////////////////////////////////////////////////////////////////////////
// SearchWidget public methods
////////////////////////////////////////////////////////////////////////////////

SearchWidget::SearchWidget(QWidget *parent, const char *name) : QWidget(parent, name)
{
    setupLayout();
    updateColumns();
}

SearchWidget::~SearchWidget()
{

}

void SearchWidget::setSearch(const PlaylistSearch &search)
{
    PlaylistSearch::ComponentList components = search.components();

    if(components.isEmpty()) {
	clear();
	return;
    }

    m_searchLine->setSearchComponent(*components.begin());
}

PlaylistSearch SearchWidget::search(const PlaylistList &playlists) const
{
    PlaylistSearch::ComponentList components;
    components.append(m_searchLine->searchComponent());
    return PlaylistSearch(playlists, components);
}

void SearchWidget::updateColumns()
{
    m_searchLine->updateColumns();
}

////////////////////////////////////////////////////////////////////////////////
// SearchWidget public slots
////////////////////////////////////////////////////////////////////////////////

void SearchWidget::clear()
{
    m_searchLine->clear();
}

////////////////////////////////////////////////////////////////////////////////
// SearchWidget private methods
////////////////////////////////////////////////////////////////////////////////

void SearchWidget::setupLayout()
{
    QHBoxLayout *layout = new QHBoxLayout(this, 5);
    layout->setAutoAdd(true);

    new QLabel(i18n("Search:"), this);

    m_searchLine = new SearchLine(this);
    connect(m_searchLine, SIGNAL(signalQueryChanged()), this, SIGNAL(signalQueryChanged()));

    QPushButton *button = new QPushButton(i18n("Clear"), this);
    connect(button, SIGNAL(clicked()), this, SLOT(clear()));

    setFixedHeight(minimumSizeHint().height());
}

#include "searchwidget.moc"
