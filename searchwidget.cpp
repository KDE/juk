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

#include <qlayout.h>
#include <qlabel.h>
#include <qcheckbox.h>
#include <qpushbutton.h>

#include "searchwidget.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

SearchWidget::SearchWidget(QWidget *parent, const char *name) : QWidget(parent, name)
{
    QHBoxLayout *layout = new QHBoxLayout(this, 5);
    layout->setAutoAdd(true);

    new QLabel(i18n("Search:"), this);

    m_lineEdit = new KLineEdit(this, "searchLineEdit");
    m_caseSensitive = new QCheckBox(i18n("Case sensitive"), this);

    QPushButton *button = new QPushButton(i18n("Clear"), this);

    connect(button, SIGNAL(clicked()), this, SLOT(clear()));
    connect(m_lineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(slotQueryChanged()));
    
    connect(m_caseSensitive, SIGNAL(toggled(bool)), this, SLOT(slotQueryChanged()));
    setFixedHeight(minimumSizeHint().height());
}

SearchWidget::~SearchWidget()
{

}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void SearchWidget::clear()
{
    m_lineEdit->clear();
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void SearchWidget::slotQueryChanged()
{
    emit signalQueryChanged(m_lineEdit->text(), m_caseSensitive->isChecked());
}

#include "searchwidget.moc"
