/**
 * Copyright (C) 2003 Richard Lärkäng <nouseforaname@home.se>
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

#include "searchwidget.h"
#include "collectionlist.h"
#include "actioncollection.h"
#include "searchadaptor.h"
#include "juk_debug.h"

#include <klineedit.h>
#include <kiconloader.h>
#include <kcombobox.h>
#include <KLocalizedString>

#include <QAction>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QToolButton>
#include <QKeyEvent>
#include <QHBoxLayout>

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// SearchLine public methods
////////////////////////////////////////////////////////////////////////////////

SearchLine::SearchLine(QWidget *parent, bool simple)
    : QFrame(parent),
    m_simple(simple),
    m_searchFieldsBox(0)
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(5);

    if(!m_simple) {
        m_searchFieldsBox = new KComboBox(this);
        layout->addWidget(m_searchFieldsBox);
        m_searchFieldsBox->setObjectName( QLatin1String( "searchFields" ) );
        connect(m_searchFieldsBox, SIGNAL(activated(int)),
                this, SIGNAL(signalQueryChanged()));
    }

    m_lineEdit = new KLineEdit(this);
    layout->addWidget(m_lineEdit);
    m_lineEdit->setClearButtonShown(true);
    m_lineEdit->installEventFilter(this);
    connect(m_lineEdit, SIGNAL(textChanged(QString)),
            this, SIGNAL(signalQueryChanged()));
    connect(m_lineEdit, SIGNAL(returnPressed()),
            this, SLOT(slotActivate()));

    if(!m_simple) {
        m_caseSensitive = new KComboBox(this);
        layout->addWidget(m_caseSensitive);
        m_caseSensitive->addItem(i18n("Normal Matching"));
        m_caseSensitive->addItem(i18n("Case Sensitive"));
        m_caseSensitive->addItem(i18n("Pattern Matching"));
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
    bool caseSensitive = m_caseSensitive && m_caseSensitive->currentIndex() == CaseSensitive;

    Playlist *playlist = CollectionList::instance();

    QList<int> searchedColumns;

    if(!m_searchFieldsBox || m_searchFieldsBox->currentIndex() == 0) {
        foreach(int column, m_columnList) {
            if(!playlist->isColumnHidden(column))
                searchedColumns.append(column);
        }
    }
    else
        searchedColumns.append(m_columnList[m_searchFieldsBox->currentIndex() - 1]);

    if(m_caseSensitive && m_caseSensitive->currentIndex() == Pattern)
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
            m_caseSensitive->setCurrentIndex(component.isCaseSensitive() ? CaseSensitive : Default);
    }
    else {
        m_lineEdit->setText(component.pattern().pattern());
        if(m_caseSensitive)
            m_caseSensitive->setCurrentIndex(Pattern);
    }

    if(!m_simple) {
        if(component.columns().isEmpty() || component.columns().size() > 1)
            m_searchFieldsBox->setCurrentIndex(0);
        else
            m_searchFieldsBox->setCurrentIndex(component.columns().front() + 1);
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
        return QFrame::eventFilter(watched, e);

    QKeyEvent *key = static_cast<QKeyEvent *>(e);
    if(key->key() == Qt::Key_Down)
        emit signalDownPressed();

    return QFrame::eventFilter(watched, e);
}

void SearchLine::slotActivate()
{
    action("stop")->trigger();
    action("playFirst")->trigger();
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

    for(int i = 0; i < playlist->columnCount(); i++) {
        m_columnList.append(i);
        QString text = playlist->headerItem()->text(i);
        columnHeaders.append(text);
        if(currentText == text)
            selection = m_columnList.size() - 1;
    }

    if(m_searchFieldsBox) {
        m_searchFieldsBox->addItems(columnHeaders);
        m_searchFieldsBox->setCurrentIndex(selection + 1);
    }
}

////////////////////////////////////////////////////////////////////////////////
// SearchWidget public methods
////////////////////////////////////////////////////////////////////////////////

SearchWidget::SearchWidget(QWidget *parent)
    : KToolBar(parent),
    m_searchLine(this, true)
{
    new SearchAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/Search", this);

    QLabel *label = new QLabel(i18n("Search:"), this );
    label->setBuddy(&m_searchLine);
    addWidget(label);
    addWidget(&m_searchLine);

    connect(&m_searchLine, SIGNAL(signalQueryChanged()), this, SIGNAL(signalQueryChanged()));
    connect(&m_searchLine, SIGNAL(signalDownPressed()), this, SIGNAL(signalDownPressed()));
    connect(m_searchLine.m_lineEdit, SIGNAL(returnPressed()), this, SIGNAL(returnPressed()));

    updateColumns();
}

void SearchWidget::setSearch(const PlaylistSearch &search)
{
    PlaylistSearch::ComponentList components = search.components();

    if(components.isEmpty()) {
        clear();
        return;
    }

    m_searchLine.setSearchComponent(*components.begin());
}

QString SearchWidget::searchText() const
{
    return m_searchLine.searchComponent().query();
}

void SearchWidget::setSearchText(const QString &text)
{
    m_searchLine.setSearchComponent(PlaylistSearch::Component(text));
}

PlaylistSearch SearchWidget::search(const PlaylistList &playlists) const
{
    PlaylistSearch::ComponentList components;
    components.append(m_searchLine.searchComponent());
    return PlaylistSearch(playlists, components);
}



////////////////////////////////////////////////////////////////////////////////
// SearchWidget public slots
////////////////////////////////////////////////////////////////////////////////

void SearchWidget::clear()
{
    m_searchLine.clear();
}

void SearchWidget::setEnabled(bool enable)
{
    emit signalShown(enable);
    setVisible(enable);
}

void SearchWidget::setFocus()
{
    m_searchLine.setFocus();
}

////////////////////////////////////////////////////////////////////////////////
// SearchWidget private methods
////////////////////////////////////////////////////////////////////////////////

void SearchWidget::updateColumns()
{
    m_searchLine.updateColumns();
}

// vim: set et sw=4 tw=0 sta:
