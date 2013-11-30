/**
 * Copyright (C) 2003 Frerich Raabe <raabe@kde.org>
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

#include "tagguesserconfigdlg.h"
#include "tagguesser.h"

#include <kicon.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <klineedit.h>
#include <kapplication.h>

#include <QKeyEvent>
#include <QStringListModel>

TagGuesserConfigDlg::TagGuesserConfigDlg(QWidget *parent, const char *name)
    : KDialog(parent)
{
    setObjectName( QLatin1String( name ) );
    setModal(true);
    setCaption(i18n("Tag Guesser Configuration"));
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);
    showButtonSeparator(true);

    m_child = new TagGuesserConfigDlgWidget(this);
    setMainWidget(m_child);

    m_child->bMoveUp->setIcon(KIcon( QLatin1String( "arrow-up" )));
    m_child->bMoveDown->setIcon(KIcon( QLatin1String( "arrow-down" )));

    m_tagSchemeModel = new QStringListModel(m_child->lvSchemes);
    m_child->lvSchemes->setModel(m_tagSchemeModel);
    m_child->lvSchemes->setHeaderHidden(true);
    m_tagSchemeModel->setStringList(TagGuesser::schemeStrings());

    connect(m_child->lvSchemes, SIGNAL(clicked(QModelIndex)), this, SLOT(slotCurrentChanged(QModelIndex)));
    connect(m_child->bMoveUp, SIGNAL(clicked()), this, SLOT(slotMoveUpClicked()));
    connect(m_child->bMoveDown, SIGNAL(clicked()), this, SLOT(slotMoveDownClicked()));
    connect(m_child->bAdd, SIGNAL(clicked()), this, SLOT(slotAddClicked()));
    connect(m_child->bModify, SIGNAL(clicked()), this, SLOT(slotModifyClicked()));
    connect(m_child->bRemove, SIGNAL(clicked()), this, SLOT(slotRemoveClicked()));

    resize( 400, 300 );
}

void TagGuesserConfigDlg::slotCurrentChanged(QModelIndex item)
{
    m_child->bRemove->setEnabled(m_tagSchemeModel->rowCount() != 0);

    // Ensure up/down buttons are appropriately enabled.

    if (!m_tagSchemeModel->rowCount() || item == m_tagSchemeModel->index(0, 0, QModelIndex()))
        m_child->bMoveUp->setEnabled(false);
    else
        m_child->bMoveUp->setEnabled(true);

    if (!m_tagSchemeModel->rowCount() || item == m_tagSchemeModel->index(m_tagSchemeModel->rowCount(QModelIndex())-1, 0, QModelIndex()))
        m_child->bMoveDown->setEnabled(false);
    else
        m_child->bMoveDown->setEnabled(true);
}

void TagGuesserConfigDlg::slotMoveUpClicked()
{
    QModelIndex currentItem = m_child->lvSchemes->currentIndex();
    int row = currentItem.row();

    m_tagSchemeModel->insertRow(row - 1); // Insert in front of item above
    row++; // Now we're one row down

    QModelIndex newItem = m_tagSchemeModel->index(row - 2, 0);

    // Copy over, then delete old item
    currentItem = m_tagSchemeModel->index(row, 0);
    m_tagSchemeModel->setData(newItem, m_tagSchemeModel->data(currentItem, Qt::DisplayRole), Qt::DisplayRole);
    m_tagSchemeModel->removeRow(row);

    m_child->lvSchemes->setCurrentIndex(newItem);
    slotCurrentChanged(newItem);
}

void TagGuesserConfigDlg::slotMoveDownClicked()
{
    QModelIndex currentItem = m_child->lvSchemes->currentIndex();
    int row = currentItem.row();

    m_tagSchemeModel->insertRow(row + 2); // Insert in front of 2 items below

    QModelIndex newItem = m_tagSchemeModel->index(row + 2, 0);

    // Copy over, then delete old item
    currentItem = m_tagSchemeModel->index(row, 0);
    m_tagSchemeModel->setData(newItem, m_tagSchemeModel->data(currentItem, Qt::DisplayRole), Qt::DisplayRole);
    m_tagSchemeModel->removeRow(row);

    newItem = m_tagSchemeModel->index(row + 1, 0);
    m_child->lvSchemes->setCurrentIndex(newItem);
    slotCurrentChanged(newItem);
}

void TagGuesserConfigDlg::slotAddClicked()
{
    m_tagSchemeModel->insertRow(0, QModelIndex());
    m_child->lvSchemes->setCurrentIndex(m_tagSchemeModel->index(0, 0, QModelIndex()));
    m_child->lvSchemes->edit(m_child->lvSchemes->currentIndex());
    slotCurrentChanged(m_child->lvSchemes->currentIndex());
}

void TagGuesserConfigDlg::slotModifyClicked()
{
    m_child->lvSchemes->edit(m_child->lvSchemes->currentIndex());
}

void TagGuesserConfigDlg::slotRemoveClicked()
{
    m_tagSchemeModel->removeRow(m_child->lvSchemes->currentIndex().row(), QModelIndex());
    slotCurrentChanged(m_child->lvSchemes->currentIndex());
}

void TagGuesserConfigDlg::accept()
{
    TagGuesser::setSchemeStrings(m_tagSchemeModel->stringList());
    KDialog::accept();
}

// vim: set et sw=4 tw=0 sta:
