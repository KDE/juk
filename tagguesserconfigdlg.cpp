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

#include <qtoolbutton.h>

TagGuesserConfigDlg::TagGuesserConfigDlg(QWidget *parent, const char *name)
    : KDialogBase(parent, name, true, i18n("Tag guesser configuration"),
                  Ok | Cancel, Ok, true)
{
    m_child = new TagGuesserConfigDlgWidget(this, "child");
    setMainWidget(m_child);

    m_child->lvSchemes->setItemsRenameable(true);
    m_child->bMoveUp->setIconSet(QIconSet(DesktopIcon("1uparrow")));
    m_child->bMoveDown->setIconSet(QIconSet(DesktopIcon("1downarrow")));

    const QStringList schemes = TagGuesser::schemeStrings();
    QStringList::ConstIterator it = schemes.begin();
    QStringList::ConstIterator end = schemes.end();
    for (; it != end; ++it)
        new KListViewItem(m_child->lvSchemes, *it);

    connect(m_child->lvSchemes, SIGNAL(currentChanged(QListViewItem *)),
            this, SLOT(slotCurrentChanged(QListViewItem *)));
    connect(m_child->lvSchemes, SIGNAL(doubleClicked(QListViewItem *, const QPoint &, int)),
            this, SLOT(slotRenameItem(QListViewItem *, const QPoint &, int)));
    connect(m_child->bAdd, SIGNAL(clicked()), this, SLOT(slotAddClicked()));
    connect(m_child->bModify, SIGNAL(clicked()), this, SLOT(slotModifyClicked()));
    connect(m_child->bRemove, SIGNAL(clicked()), this, SLOT(slotRemoveClicked()));
}

void TagGuesserConfigDlg::accept()
{
    QStringList schemes;
    for (QListViewItem *it = m_child->lvSchemes->firstChild(); it; it = it->nextSibling())
        schemes += it->text(0);
    TagGuesser::setSchemeStrings(schemes);
    accept();
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
