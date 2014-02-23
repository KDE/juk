/**
 * Copyright (C) 2005 Michael Pyne <mpyne@kde.org>
 * Copyright (C) 2014 Arnold Dumas <contact@arnolddumas.com>
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

#include "coverdialog.h"

#include <kiconloader.h>
#include <kapplication.h>
#include <kmenu.h>
#include <klocale.h>

#include <QTimer>

#include "covericonview.h"
#include "covermanager.h"
#include "collectionlist.h"

using CoverUtility::CoverIconViewItem;

class AllArtistsListViewItem : public QListWidgetItem
{
public:
    AllArtistsListViewItem(KListWidget *parent) :
        QListWidgetItem(i18n("&lt;All Artists&gt;"), parent)
    {
    }

    bool operator< (const QListWidgetItem& other) const
    {
        Q_UNUSED(other);
        return true; // Always be at the top.
    }
};

class CaseInsensitiveItem : public QListWidgetItem
{
public:
    CaseInsensitiveItem(KListWidget *parent, const QString &text) :
        QListWidgetItem(text, parent)
    {
    }

    bool operator< (const QListWidgetItem& other) const
    {
        return text().toLower().localeAwareCompare(other.text().toLower());
    }
};

CoverDialog::CoverDialog(QWidget *parent) :
    QWidget(parent, Qt::WType_Dialog)
{
    setupUi(this);

    setObjectName( QLatin1String("juk_cover_dialog" ));

    m_searchLine->setClearButtonShown(true);

    connect(m_artists, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(slotArtistClicked(QListWidgetItem*)));

    connect(m_covers, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotContextRequested(QPoint)));

    connect(m_searchLine, SIGNAL(textChanged(QString)),
            this, SLOT(slotSearchPatternChanged(QString)));
}

CoverDialog::~CoverDialog()
{
}

void CoverDialog::show()
{
    m_artists->clear();
    m_covers->clear();

    QStringList artists = CollectionList::instance()->uniqueSet(CollectionList::Artists);

    new AllArtistsListViewItem(m_artists);
    for(QStringList::ConstIterator it = artists.constBegin(); it != artists.constEnd(); ++it)
        new CaseInsensitiveItem(m_artists, *it);

    QTimer::singleShot(0, this, SLOT(loadCovers()));
    QWidget::show();
}

// Here we try to keep the GUI from freezing for too long while we load the
// covers.
void CoverDialog::loadCovers()
{
    CoverDataMapIterator it, end;
    int i = 0;

    it  = CoverManager::begin();
    end = CoverManager::end();

    for(; it != end; ++it) {
        (void) new CoverIconViewItem(it.key(), m_covers);

        // TODO: Threading!
        if(++i == 10) {
            i = 0;
            kapp->processEvents();
        }
    }
}

// TODO: Add a way to show cover art for tracks with no artist.
void CoverDialog::slotArtistClicked(QListWidgetItem *item)
{
    m_covers->clear();
    if (!item) {
        return;
    }
    if(dynamic_cast<AllArtistsListViewItem *>(item)) {
        // All artists.
        loadCovers();
    }
    else {
        QString artist = item->text().toLower();

        CoverDataMapIterator it, end;

        it  = CoverManager::begin();
        end = CoverManager::end();

        for(; it != end; ++it) {
            if(it.value()->artist == artist)
                (void) new CoverIconViewItem(it.key(), m_covers);
        }
    }
}

void CoverDialog::slotContextRequested(const QPoint &pt)
{
    static KMenu *menu = 0;

    QListWidgetItem* item = m_covers->currentItem();

    if(!item)
        return;

    if(!menu) {
        menu = new KMenu(this);
        menu->addAction(i18n("Remove Cover"), this, SLOT(removeSelectedCover()));
    }

    QPoint globalPt = m_covers->mapToGlobal(pt);
    menu->popup(globalPt);
}

void CoverDialog::slotSearchPatternChanged(const QString& pattern)
{
    m_covers->clear();

    QListWidgetItem* item = m_artists->currentItem();

    // If the expression is cleared, then use slotArtistClicked.
    if (pattern.isEmpty()) {
        slotArtistClicked(item);
    }

    else {
        QRegExp filter(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
        QString artist = item->text().toLower();

        CoverDataMapIterator it, end;

        it  = CoverManager::begin();
        end = CoverManager::end();

        // Here, only show cover that match the search pattern.
        if (dynamic_cast<AllArtistsListViewItem *>(item)) {

            for(; it != end; ++it) {
                if (filter.indexIn(it.value()->artist) != -1) {

                    (void) new CoverIconViewItem(it.key(), m_covers);
                }
            }
        }

        // Here, only show the covers that match the search pattern and
        // that have the same artist as the currently selected one.
        else {

            for(; it != end; ++it) {
                if (it.value()->artist == artist
                        && (filter.indexIn(it.value()->artist) != -1)
                        || (filter.indexIn(it.value()->album) != -1)) {

                    (void) new CoverIconViewItem(it.key(), m_covers);
                }
            }
        }
    }
}

void CoverDialog::removeSelectedCover()
{
    CoverIconViewItem *coverItem = m_covers->currentItem();

    if(!coverItem || !coverItem->isSelected()) {
        kWarning() << "No item selected for removeSelectedCover.\n";
        return;
    }

    if(!CoverManager::removeCover(coverItem->id()))
        kError() << "Unable to remove selected cover: " << coverItem->id() << endl;
    else
        delete coverItem;
}

#include "coverdialog.moc"

// vim: set et sw=4 tw=0 sta:
