/***************************************************************************
                          searchwidget.cpp
                             -------------------
    begin                : Sun Mar 6 2003
    copyright            : (C) 2003 by Scott Wheeler
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

#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qpushbutton.h>

#include "searchwidget.h"
#include "playlist.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

SearchWidget::SearchWidget(QWidget *parent, const Playlist *playlist, const char *name) 
    : QWidget(parent, name),
      m_playlist(playlist)

{
    setupLayout();
    slotUpdateColumns();
}

SearchWidget::~SearchWidget()
{

}

QString SearchWidget::query() const
{
    return m_lineEdit->text();
}

bool SearchWidget::caseSensitive() const
{
    return m_caseSensitive->isChecked();
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void SearchWidget::clear()
{
    m_lineEdit->clear();
}

void SearchWidget::slotQueryChanged(int)
{
    m_searchedColumns[0].clear();

    if(m_searchFieldsBox->currentItem() == 0) {
	for(int i = 0; i < m_playlist->columns(); i++) {
	    if(m_playlist->isColumnVisible(i) && !m_playlist->columnText(i).isEmpty())
		m_searchedColumns[0].append(i);
	}
    }
    else
	m_searchedColumns[0].append(m_searchFieldsBox->currentItem() - 1);
    
    emit signalQueryChanged(m_lineEdit->text(), m_caseSensitive->isChecked());
}

void SearchWidget::slotUpdateColumns()
{
    QString currentText = m_searchFieldsBox->currentText();
    m_searchFieldsBox->clear();
    m_columnHeaders.clear();
    m_columnHeaders.append(QString("<%1>").arg(i18n("All Visible")));

    int selection = -1;

    for(int i = 0; i < m_playlist->columns(); i++) {
	if(m_playlist->isColumnVisible(i) && !m_playlist->columnText(i).isEmpty()) {
	    QString text = m_playlist->columnText(i);
	    m_columnHeaders.append(text);
	    if(currentText == text)
		selection = i;
	}
    }

    m_searchFieldsBox->insertStringList(m_columnHeaders);
    m_searchFieldsBox->setCurrentItem(selection + 1);
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void SearchWidget::setupLayout()
{
    QHBoxLayout *layout = new QHBoxLayout(this, 5);
    layout->setAutoAdd(true);

    new QLabel(i18n("Search:"), this);

    m_searchFieldsBox = new KComboBox(this, "searchFields");
    connect(m_searchFieldsBox, SIGNAL(activated(int)), this, SLOT(slotQueryChanged()));

    m_lineEdit = new KLineEdit(this, "searchLineEdit");
    m_caseSensitive = new QCheckBox(i18n("Case sensitive"), this);

    QPushButton *button = new QPushButton(i18n("Clear"), this);

    connect(button, SIGNAL(clicked()), this, SLOT(clear()));
    connect(m_lineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotQueryChanged()));
    
    connect(m_caseSensitive, SIGNAL(toggled(bool)), this, SLOT(slotQueryChanged()));
    setFixedHeight(minimumSizeHint().height());
}

#include "searchwidget.moc"
