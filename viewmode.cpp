/**
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

#include "viewmode.h"

#include <kiconloader.h>

#include <QPainter>
#include <QResizeEvent>

#include "playlistbox.h"
#include "searchplaylist.h"
#include "treeviewitemplaylist.h"
#include "collectionlist.h"
#include "juk_debug.h"

////////////////////////////////////////////////////////////////////////////////
// ViewMode
////////////////////////////////////////////////////////////////////////////////

ViewMode::ViewMode(PlaylistBox *b) :
    QObject(b),
    m_playlistBox(b),
    m_visible(false),
    m_needsRefresh(false)
{
    m_playlistBox->viewport()->installEventFilter(this);
}

ViewMode::~ViewMode()
{
    removeEventFilter(m_playlistBox->viewport());
}

bool ViewMode::eventFilter(QObject *watched, QEvent *e)
{
    if(m_visible && watched == m_playlistBox->viewport() && e->type() == QEvent::Resize) {
        QResizeEvent *re = static_cast<QResizeEvent *>(e);
        if(re->size().width() != re->oldSize().width())
            m_needsRefresh = true;
    }

    if(e->type() == QEvent::Hide)
        m_needsRefresh = true;

    return QObject::eventFilter(watched, e);
}

QString ViewMode::name() const
{
    return i18nc("the normal viewing mode", "Default");
}

void ViewMode::setShown(bool shown)
{
    m_visible = shown;
    if(shown) {
        updateIcons();
        m_needsRefresh = true;
    }
}

void ViewMode::updateIcons()
{
    for(QTreeWidgetItemIterator it(m_playlistBox); *it; ++it) {
        PlaylistBox::Item *i = static_cast<PlaylistBox::Item *>(*it);
        i->setIcon(0, QIcon::fromTheme(i->iconName()));
    }
}

void ViewMode::setupItem(PlaylistBox::Item *item) const
{
    Q_UNUSED(item);
}

void ViewMode::paintDropIndicator(QPainter *painter, int width, int height) // static
{
    static const int border = 1;
    static const int lineWidth = 2;

    QPen oldPen = painter->pen();
    QPen newPen = oldPen;

    newPen.setWidth(lineWidth);
    newPen.setStyle(Qt::DotLine);

    painter->setPen(newPen);
    painter->drawRect(border, border, width - border * 2, height - border * 2);
    painter->setPen(oldPen);
}

///////////////////////////////////////////////////////////////////////////////
// CompactViewMode
////////////////////////////////////////////////////////////////////////////////

CompactViewMode::CompactViewMode(PlaylistBox *b) : ViewMode(b)
{

}

CompactViewMode::~CompactViewMode()
{

}

QString CompactViewMode::name() const
{
    return i18nc("compact viewing mode", "Compact");
}

void CompactViewMode::setShown(bool shown)
{
    setVisible(shown);

    if(shown) {
        updateIcons();
    }
}

////////////////////////////////////////////////////////////////////////////////
// TreeViewMode
////////////////////////////////////////////////////////////////////////////////

TreeViewMode::TreeViewMode(PlaylistBox *b) : CompactViewMode(b),
    m_dynamicListsFrozen(false), m_setup(false)
{

}

TreeViewMode::~TreeViewMode()
{

}

QString TreeViewMode::name() const
{
    return i18n("Tree");
}

void TreeViewMode::setShown(bool show)
{
    CompactViewMode::setShown(show);

    playlistBox()->setRootIsDecorated(show);

    if(show) {
        if(m_searchCategories.isEmpty())
            setupDynamicPlaylists();
        else {
            for(auto &item : m_searchCategories)
                item->setHidden(false);
        }

        if(!m_setup) {
            m_setup = true;
            playlistBox()->setSortingEnabled(false);
            CollectionList::instance()->setupTreeViewEntries(this);
            playlistBox()->setSortingEnabled(true);
        }
    }
    else {
        for(auto &item : m_searchCategories)
            item->setHidden(true);
    }
}

void TreeViewMode::removeItem(const QString &item, unsigned column)
{
    if(!m_setup)
        return;

    QString itemKey;
    if(column == PlaylistItem::ArtistColumn)
        itemKey = "artists" + item;
    else if(column == PlaylistItem::GenreColumn)
        itemKey = "genres" + item;
    else if(column == PlaylistItem::AlbumColumn)
        itemKey = "albums" + item;
    else {
        qCWarning(JUK_LOG) << "Unhandled column type " << column;
        return;
    }

    if(!m_treeViewItems.contains(itemKey))
        return;

    TreeViewItemPlaylist *itemPlaylist = m_treeViewItems.value(itemKey, 0);

    if(m_dynamicListsFrozen) {
        m_pendingItemsToRemove << itemKey;
        return;
    }

    m_treeViewItems.remove(itemKey);
    itemPlaylist->deleteLater();
    emit signalPlaylistDestroyed(itemPlaylist);
}

void TreeViewMode::addItems(const QStringList &items, unsigned column)
{
    if(!m_setup)
        return;

    QString searchCategory;
    if(column == PlaylistItem::ArtistColumn)
        searchCategory = "artists";
    else if(column == PlaylistItem::GenreColumn)
        searchCategory = "genres";
    else if(column == PlaylistItem::AlbumColumn)
        searchCategory = "albums";
    else {
        qCWarning(JUK_LOG) << "Unhandled column type " << column;
        return;
    }

    ColumnList columns;
    columns.append(column);

    PlaylistSearch::Component::MatchMode mode = PlaylistSearch::Component::ContainsWord;
    if(column != PlaylistItem::ArtistColumn)
        mode = PlaylistSearch::Component::Exact;

    PlaylistSearch::ComponentList components;
    PlaylistList playlists;
    playlists.append(CollectionList::instance());

    QString itemKey;
    PlaylistBox::Item *itemParent = m_searchCategories.value(searchCategory, 0);

    foreach(const QString &item, items) {
        itemKey = searchCategory + item;

        if(m_treeViewItems.contains(itemKey))
            continue;

        components.clear();
        components.append(PlaylistSearch::Component(item, false, columns, mode));

        PlaylistSearch s(playlists, components, PlaylistSearch::MatchAny);

        TreeViewItemPlaylist *p = new TreeViewItemPlaylist(playlistBox(), s, item);
        playlistBox()->setupPlaylist(p, "audio-midi", itemParent);
        m_treeViewItems.insert(itemKey, p);
    }
}

void TreeViewMode::setDynamicListsFrozen(bool frozen)
{
    m_dynamicListsFrozen = frozen;

    if(frozen)
        return;

    foreach(const QString &pendingItem, m_pendingItemsToRemove) {
        m_treeViewItems[pendingItem]->deleteLater();
        m_treeViewItems.remove(pendingItem);
    }

    m_pendingItemsToRemove.clear();
}

void TreeViewMode::setupDynamicPlaylists()
{
    PlaylistBox::Item *i;
    PlaylistBox::Item *collectionItem = PlaylistBox::Item::collectionItem();

    i = new PlaylistBox::Item(collectionItem, "media-optical-audio", i18n("Artists"));
    m_searchCategories.insert("artists", i);

    i = new PlaylistBox::Item(collectionItem, "media-optical-audio", i18n("Albums"));
    m_searchCategories.insert("albums", i);

    i = new PlaylistBox::Item(collectionItem, "media-optical-audio", i18n("Genres"));
    m_searchCategories.insert("genres", i);
}

// vim: set et sw=4 tw=0 sta:
