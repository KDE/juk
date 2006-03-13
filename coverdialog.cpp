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

#include <k3listview.h>
#include <k3iconview.h>
#include <kiconviewsearchline.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <kmenu.h>
#include <klocale.h>

#include <qtimer.h>
#include <qtoolbutton.h>

#include <Q3ValueList>

#include "coverdialog.h"
#include "covericonview.h"
#include "covermanager.h"
#include "collectionlist.h"

using CoverUtility::CoverIconViewItem;

class AllArtistsListViewItem : public K3ListViewItem
{
public:
    AllArtistsListViewItem(Q3ListView *parent) :
        K3ListViewItem(parent, i18n("<All Artists>"))
    {
    }

    int compare(Q3ListViewItem *, int, bool) const
    {
        return -1; // Always be at the top.
    }
};

class CaseInsensitiveItem : public K3ListViewItem
{
public:
    CaseInsensitiveItem(Q3ListView *parent, const QString &text) :
        K3ListViewItem(parent, text)
    {
    }

    int compare(Q3ListViewItem *item, int column, bool ascending) const
    {
        Q_UNUSED(ascending);
        return text(column).lower().localeAwareCompare(item->text(column).lower());
    }
};

CoverDialog::CoverDialog(QWidget *parent) :
    CoverDialogBase(parent, "juk_cover_dialog", Qt::WType_Dialog)
{
    m_covers->setResizeMode(Q3IconView::Adjust);
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
    Q3ValueList<coverKey> keys = CoverManager::keys();
    Q3ValueList<coverKey>::ConstIterator it;
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
void CoverDialog::slotArtistClicked(Q3ListViewItem *item)
{
    m_covers->clear();

    if(dynamic_cast<AllArtistsListViewItem *>(item)) {
        // All artists.
        loadCovers();
    }
    else {
        QString artist = item->text(0).lower();
        Q3ValueList<coverKey> keys = CoverManager::keys();
        Q3ValueList<coverKey>::ConstIterator it;

        for(it = keys.begin(); it != keys.end(); ++it) {
            CoverDataPtr data = CoverManager::coverInfo(*it);
            if(data->artist == artist)
                new CoverIconViewItem(*it, m_covers);
        }
    }
}

void CoverDialog::slotContextRequested(Q3IconViewItem *item, const QPoint &pt)
{
    static KMenu *menu = 0;

    if(!item)
        return;

    if(!menu) {
        menu = new KMenu(this);
        menu->insertItem(i18n("Remove Cover"), this, SLOT(removeSelectedCover()));
    }

    menu->popup(pt);
}

void CoverDialog::removeSelectedCover()
{
    CoverIconViewItem *coverItem = m_covers->currentItem();

    if(!coverItem || !coverItem->isSelected()) {
        kWarning(65432) << "No item selected for removeSelectedCover.\n";
        return;
    }

    if(!CoverManager::removeCover(coverItem->id()))
        kError(65432) << "Unable to remove selected cover: " << coverItem->id() << endl;
    else
        delete coverItem;
}

#include "coverdialog.moc"

// vim: set et sw=4 tw=0 sta:
