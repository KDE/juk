/*
 * tagguesserconfigdlg.cpp - (c) 2003 Frerich Raabe <raabe@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "tagguesser.h"
#include "tagguesserconfigdlg.h"
#include "tagguesserconfigdlgwidget.h"

#include <kiconloader.h>
#include <klistview.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <klineedit.h>
#include <kapplication.h>

#include <qtoolbutton.h>
#include <qevent.h>

TagGuesserConfigDlg::TagGuesserConfigDlg(QWidget *parent, const char *name)
    : KDialogBase(parent, name, true, i18n("Tag Guesser Configuration"),
                  Ok | Cancel, Ok, true)
{
    m_child = new TagGuesserConfigDlgWidget(this, "child");
    setMainWidget(m_child);

    m_child->lvSchemes->setItemsRenameable(true);
    m_child->lvSchemes->setSorting(-1);
    m_child->lvSchemes->setDefaultRenameAction(QListView::Accept);
    m_child->bMoveUp->setIconSet(BarIconSet("1uparrow"));
    m_child->bMoveDown->setIconSet(BarIconSet("1downarrow"));

    const QStringList schemes = TagGuesser::schemeStrings();
    QStringList::ConstIterator it = schemes.begin();
    QStringList::ConstIterator end = schemes.end();
    for (; it != end; ++it) {
        KListViewItem *item = new KListViewItem(m_child->lvSchemes, *it);
        item->moveItem(m_child->lvSchemes->lastItem());
    }

    connect(m_child->lvSchemes, SIGNAL(currentChanged(QListViewItem *)),
            this, SLOT(slotCurrentChanged(QListViewItem *)));
    connect(m_child->lvSchemes, SIGNAL(doubleClicked(QListViewItem *, const QPoint &, int)),
            this, SLOT(slotRenameItem(QListViewItem *, const QPoint &, int)));
    connect(m_child->bMoveUp, SIGNAL(clicked()), this, SLOT(slotMoveUpClicked()));
    connect(m_child->bMoveDown, SIGNAL(clicked()), this, SLOT(slotMoveDownClicked()));
    connect(m_child->bAdd, SIGNAL(clicked()), this, SLOT(slotAddClicked()));
    connect(m_child->bModify, SIGNAL(clicked()), this, SLOT(slotModifyClicked()));
    connect(m_child->bRemove, SIGNAL(clicked()), this, SLOT(slotRemoveClicked()));

    m_child->lvSchemes->setSelected(m_child->lvSchemes->firstChild(), true);
    slotCurrentChanged(m_child->lvSchemes->currentItem());

    resize( 400, 300 );
}

void TagGuesserConfigDlg::accept()
{
    if(m_child->lvSchemes->renameLineEdit()) {
        QKeyEvent returnKeyPress(QEvent::KeyPress, Key_Return, 0, 0);
        KApplication::sendEvent(m_child->lvSchemes->renameLineEdit(), &returnKeyPress);
    }

    QStringList schemes;
    for (QListViewItem *it = m_child->lvSchemes->firstChild(); it; it = it->nextSibling())
        schemes += it->text(0);
    TagGuesser::setSchemeStrings(schemes);
    KDialogBase::accept();
}

void TagGuesserConfigDlg::slotCurrentChanged(QListViewItem *item)
{
    m_child->bMoveUp->setEnabled(item != 0 && item->itemAbove() != 0);
    m_child->bMoveDown->setEnabled(item != 0 && item->itemBelow() != 0);
    m_child->bModify->setEnabled(item != 0);
    m_child->bRemove->setEnabled(item != 0);
}

void TagGuesserConfigDlg::slotRenameItem(QListViewItem *item, const QPoint &, int c)
{
    m_child->lvSchemes->rename(item, c);
}

void TagGuesserConfigDlg::slotMoveUpClicked()
{
    QListViewItem *item = m_child->lvSchemes->currentItem();
    if(item->itemAbove() == m_child->lvSchemes->firstChild())
        item->itemAbove()->moveItem(item);
    else
      item->moveItem(item->itemAbove()->itemAbove());
    m_child->lvSchemes->ensureItemVisible(item);
    slotCurrentChanged(item);
}

void TagGuesserConfigDlg::slotMoveDownClicked()
{
    QListViewItem *item = m_child->lvSchemes->currentItem();
    item->moveItem(item->itemBelow());
    m_child->lvSchemes->ensureItemVisible(item);
    slotCurrentChanged(item);
}

void TagGuesserConfigDlg::slotAddClicked()
{
    KListViewItem *item = new KListViewItem(m_child->lvSchemes);
    m_child->lvSchemes->rename(item, 0);
}

void TagGuesserConfigDlg::slotModifyClicked()
{
    m_child->lvSchemes->rename(m_child->lvSchemes->currentItem(), 0);
}

void TagGuesserConfigDlg::slotRemoveClicked()
{
    delete m_child->lvSchemes->currentItem();
}

#include "tagguesserconfigdlg.moc"
