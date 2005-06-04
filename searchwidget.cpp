/***************************************************************************
    begin                : Sun Mar 6 2003
    copyright            : (C) 2003 by Richard Lärkäng
    email                : nouseforaname@home.se

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

#include <klocale.h>
#include <klineedit.h>
#include <kiconloader.h>
#include <kcombobox.h>
#include <kdebug.h>
#include <kaction.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>

#include "searchwidget.h"
#include "collectionlist.h"
#include "actioncollection.h"

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// SearchLine public methods
////////////////////////////////////////////////////////////////////////////////

SearchLine::SearchLine(QWidget *parent, bool simple, const char *name) :
    QHBox(parent, name),
    m_simple(simple),
    m_searchFieldsBox(0)
{
    setSpacing(5);

    if(!m_simple) {
	m_searchFieldsBox = new KComboBox(this, "searchFields");
	connect(m_searchFieldsBox, SIGNAL(activated(int)),
		this, SIGNAL(signalQueryChanged()));
    }

    m_lineEdit = new KLineEdit(this, "searchLineEdit");
    m_lineEdit->installEventFilter(this);
    connect(m_lineEdit, SIGNAL(textChanged(const QString &)),
            this, SIGNAL(signalQueryChanged()));
    connect(m_lineEdit, SIGNAL(returnPressed()),
            this, SLOT(slotActivate()));

    if(!m_simple) {
	m_caseSensitive = new KComboBox(this);
	m_caseSensitive->insertItem(i18n("Normal Matching"), 0);
	m_caseSensitive->insertItem(i18n("Case Sensitive"), 1);
	m_caseSensitive->insertItem(i18n("Pattern Matching"), 2);
	connect(m_caseSensitive, SIGNAL(activated(int)),
		this, SIGNAL(signalQueryChanged()));
    }
    else
	m_caseSensitive = 0;

    updateColumns();
}

PlaylistSearch::Component SearchLine::searchComponent() const
{
    QString query = m_lineEdit->text();
    bool caseSensitive = m_caseSensitive && m_caseSensitive->currentItem() == CaseSensitive;

    Playlist *playlist = CollectionList::instance();

    QValueList<int> searchedColumns;

    if(!m_searchFieldsBox || m_searchFieldsBox->currentItem() == 0) {
	QValueListConstIterator<int> it = m_columnList.begin();
	for(; it != m_columnList.end(); ++it) {
	    if(playlist->isColumnVisible(*it))
		searchedColumns.append(*it);
	}
    }
    else
	searchedColumns.append(m_columnList[m_searchFieldsBox->currentItem() - 1]);

    if(m_caseSensitive && m_caseSensitive->currentItem() == Pattern)
	return PlaylistSearch::Component(QRegExp(query), searchedColumns);
    else
	return PlaylistSearch::Component(query, caseSensitive, searchedColumns);
}

void SearchLine::setSearchComponent(const PlaylistSearch::Component &component)
{
    if(component == searchComponent())
	return;

    if(m_simple || !component.isPatternSearch()) {
	m_lineEdit->setText(component.query());
	if(m_caseSensitive)
	    m_caseSensitive->setCurrentItem(component.isCaseSensitive() ? CaseSensitive : Default);
    }
    else {
	m_lineEdit->setText(component.pattern().pattern());
	if(m_caseSensitive)
	    m_caseSensitive->setCurrentItem(Pattern);
    }

    if(!m_simple) {
	if(component.columns().isEmpty() || component.columns().size() > 1)
	    m_searchFieldsBox->setCurrentItem(0);
	else
	    m_searchFieldsBox->setCurrentItem(component.columns().front() + 1);
    }
}

void SearchLine::clear()
{
    // We don't want to emit the signal if it's already empty.
    if(!m_lineEdit->text().isEmpty())
	m_lineEdit->clear();
}

void SearchLine::setFocus()
{
    m_lineEdit->setFocus();
}

bool SearchLine::eventFilter(QObject *watched, QEvent *e)
{
    if(watched != m_lineEdit || e->type() != QEvent::KeyPress)
	return QHBox::eventFilter(watched, e);

    QKeyEvent *key = static_cast<QKeyEvent *>(e);
    if(key->key() == Qt::Key_Down)
	emit signalDownPressed();

    return QHBox::eventFilter(watched, e);
}

void SearchLine::slotActivate()
{
    action("stop")->activate();
    action("playFirst")->activate();
}

void SearchLine::updateColumns()
{
    QString currentText;

    if(m_searchFieldsBox) {
	currentText = m_searchFieldsBox->currentText();
	m_searchFieldsBox->clear();
    }

    QStringList columnHeaders;

    columnHeaders.append(QString("<%1>").arg(i18n("All Visible")));

    Playlist *playlist = CollectionList::instance();

    int selection = -1;
    m_columnList.clear();

    for(int i = 0; i < playlist->columns(); i++) {
	m_columnList.append(i);
	QString text = playlist->columnText(i);
	columnHeaders.append(text);
	if(currentText == text)
	    selection = m_columnList.size() - 1;
    }

    if(m_searchFieldsBox) {
	m_searchFieldsBox->insertStringList(columnHeaders);
	m_searchFieldsBox->setCurrentItem(selection + 1);
    }
}

////////////////////////////////////////////////////////////////////////////////
// SearchWidget public methods
////////////////////////////////////////////////////////////////////////////////

SearchWidget::SearchWidget(QWidget *parent, const char *name) : KToolBar(parent, name)
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

QString SearchWidget::searchText() const
{
    return m_searchLine->searchComponent().query();
}

void SearchWidget::setSearchText(const QString &text)
{
    m_searchLine->setSearchComponent(PlaylistSearch::Component(text));
}

PlaylistSearch SearchWidget::search(const PlaylistList &playlists) const
{
    PlaylistSearch::ComponentList components;
    components.append(m_searchLine->searchComponent());
    return PlaylistSearch(playlists, components);
}



////////////////////////////////////////////////////////////////////////////////
// SearchWidget public slots
////////////////////////////////////////////////////////////////////////////////

void SearchWidget::clear()
{
    m_searchLine->clear();
}

void SearchWidget::setEnabled(bool enable)
{
    emit signalShown(enable);
    setShown(enable);
}

void SearchWidget::setFocus()
{
    m_searchLine->setFocus();
}

////////////////////////////////////////////////////////////////////////////////
// SearchWidget private methods
////////////////////////////////////////////////////////////////////////////////

void SearchWidget::updateColumns()
{
    m_searchLine->updateColumns();
}

void SearchWidget::setupLayout()
{
    boxLayout()->setSpacing(5);

    QToolButton *clearSearchButton = new QToolButton(this);
    clearSearchButton->setTextLabel(i18n("Clear Search"), true);
    clearSearchButton->setIconSet(SmallIconSet("locationbar_erase"));

    QLabel *label = new QLabel(i18n("Search:"), this, "kde toolbar widget");

    m_searchLine = new SearchLine(this, true, "kde toolbar widget");

    label->setBuddy(m_searchLine);

    connect(m_searchLine, SIGNAL(signalQueryChanged()), this, SIGNAL(signalQueryChanged()));
    connect(m_searchLine, SIGNAL(signalDownPressed()), this, SIGNAL(signalDownPressed()));
    connect(clearSearchButton, SIGNAL(pressed()), m_searchLine, SLOT(clear()));
    setStretchableWidget(m_searchLine);

    // I've decided that I think this is ugly, for now.

    /*
      QToolButton *b = new QToolButton(this);
      b->setTextLabel(i18n("Advanced Search"), true);
      b->setIconSet(SmallIconSet("wizard"));

      connect(b, SIGNAL(clicked()), this, SIGNAL(signalAdvancedSearchClicked()));
    */
}

#include "searchwidget.moc"
