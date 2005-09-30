/***************************************************************************
    begin                : Thu Oct 28 2004
    copyright            : (C) 2004 by Michael Pyne
    email                : michael.pyne@kdemail.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kurlrequester.h>
#include <klocale.h>

#include <qradiobutton.h>
#include <qlayout.h>

#include "exampleoptions.h"

ExampleOptions::ExampleOptions(QWidget *parent) :
    ExampleOptionsBase(parent, "example options widget")
{
}

void ExampleOptions::exampleSelectionChanged()
{
    if(m_fileTagsButton->isChecked())
        emit fileChanged();
    else
        emit dataChanged();
}

void ExampleOptions::exampleDataChanged()
{
    emit dataChanged();
}

void ExampleOptions::exampleFileChanged()
{
    emit fileChanged();
}

ExampleOptionsDialog::ExampleOptionsDialog(QWidget *parent) :
    QDialog(parent, "example options dialog")
{
    setCaption(i18n("JuK"));
    QVBoxLayout *l = new QVBoxLayout(this);

    m_options = new ExampleOptions(this);
    l->addWidget(m_options);

    // Forward signals

    connect(m_options, SIGNAL(fileChanged()), SLOT(fileModeSelected()));
    connect(m_options, SIGNAL(dataChanged()), SIGNAL(dataChanged()));
    connect(m_options->m_exampleFile, SIGNAL(urlSelected(const QString &)),
            this,                     SIGNAL(fileChanged(const QString &)));
    connect(m_options->m_exampleFile, SIGNAL(returnPressed(const QString &)),
            this,                     SIGNAL(fileChanged(const QString &)));
}

void ExampleOptionsDialog::hideEvent(QHideEvent *)
{
    emit signalHidden();
}

void ExampleOptionsDialog::showEvent(QShowEvent *)
{
    emit signalShown();
}

void ExampleOptionsDialog::fileModeSelected()
{
    emit fileChanged(m_options->m_exampleFile->url());
}

#include "exampleoptions.moc"

// vim: set et sw=4 ts=4:
