/***************************************************************************
    begin                : Sun May 15 2005 
    copyright            : (C) 2005 by Michael Pyne
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

#include <klistview.h>
#include <kiconview.h>
#include <kiconviewsearchline.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <kpopupmenu.h>
#include <klocale.h>

#include <qtimer.h>
#include <qtoolbutton.h>

#include "coverdialog.h"
#include "covericonview.h"
#include "covermanager.h"
#include "collectionlist.h"

using CoverUtility::CoverIconViewItem;

class AllArtistsListViewItem : public KListViewItem
{
public:
    AllArtistsListViewItem(QListView *parent) :
        KListViewItem(parent, i18n("<All Artists>"))
    {
    }

    int compare(QListViewItem *, int, bool) const
    {
        return -1; // Always be at the top.
    }
};

class CaseInsensitiveItem : public KListViewItem
{
public:
    CaseInsensitiveItem(QListView *parent, const QString &text) :
        KListViewItem(parent, text)
    {
    }

    int compare(QListViewItem *item, int column, bool ascending) const
    {
        Q_UNUSED(ascending);
        return text(column).lower().localeAwareCompare(item->text(column).lower());
    }
};

CoverDialog::CoverDialog(QWidget *parent) :
    CoverDialogBase(parent, "juk_cover_dialog", WType_Dialog)
{
    m_covers->setResizeMode(QIconView::Adjust);
    m_covers->setGridX(140);
    m_covers->setGridY(150);

    m_searchLine->setIconView(m_covers);
    m_clearSearch->setIconSet(SmallIconSet("locationbar_erase"));
}

CoverDialog::~CoverDialog()
{
}

void CoverDialog::show()
{
    m_artists->clear();
    m_covers->clear();

    QStringList artists = CollectionList::instance()->uniqueSet(CollectionList::Artists);

    m_artists->setSorting(-1);
    new AllArtistsListViewItem(m_artists);
    for(QStringList::ConstIterator it = artists.begin(); it != artists.end(); ++it)
        new CaseInsensitiveItem(m_artists, *it);

    m_artists->setSorting(0);

    QTimer::singleShot(0, this, SLOT(loadCovers()));
    CoverDialogBase::show();
}

// Here we try to keep the GUI from freezing for too long while we load the
// covers.
void CoverDialog::loadCovers()
{
    QValueList<coverKey> keys = CoverManager::keys();
    QValueList<coverKey>::ConstIterator it;
    int i = 0;

    for(it = keys.begin(); it != keys.end(); ++it) {
        new CoverIconViewItem(*it, m_covers);

        if(++i == 10) {
            i = 0;
            kapp->processEvents();
        }
    }
}

// TODO: Add a way to show cover art for tracks with no artist.
void CoverDialog::slotArtistClicked(QListViewItem *item)
{
    m_covers->clear();

    if(dynamic_cast<AllArtistsListViewItem *>(item)) {
        // All artists.
        loadCovers();
    }
    else {
        QString artist = item->text(0).lower();
        QValueList<coverKey> keys = CoverManager::keys();
        QValueList<coverKey>::ConstIterator it;

        for(it = keys.begin(); it != keys.end(); ++it) {
            CoverDataPtr data = CoverManager::coverInfo(*it);
            if(data->artist == artist)
                new CoverIconViewItem(*it, m_covers);
        }
    }
}

void CoverDialog::slotContextRequested(QIconViewItem *item, const QPoint &pt)
{
    static KPopupMenu *menu = 0;

    if(!item)
        return;

    if(!menu) {
        menu = new KPopupMenu(this);
        menu->insertItem(i18n("Remove Cover"), this, SLOT(removeSelectedCover()));
    }

    menu->popup(pt);
}

void CoverDialog::removeSelectedCover()
{
    CoverIconViewItem *coverItem = m_covers->currentItem();

    if(!coverItem || !coverItem->isSelected()) {
        kdWarning(65432) << "No item selected for removeSelectedCover.\n";
        return;
    }

    if(!CoverManager::removeCover(coverItem->id()))
        kdError(65432) << "Unable to remove selected cover: " << coverItem->id() << endl;
    else
        delete coverItem;
}

#include "coverdialog.moc"

// vim: set et ts=4 sw=4:
