/***************************************************************************
    begin                : Sat Jun 7 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler,
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "viewmode.h"

#include <kiconloader.h>
#include <kstandarddirs.h>
#include <kdebug.h>

#include <QPixmap>
#include <QPainter>
#include <QResizeEvent>

#include "playlistbox.h"
#include "searchplaylist.h"
#include "treeviewitemplaylist.h"
#include "collectionlist.h"

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

}

void ViewMode::paintCell(PlaylistBox::Item *item,
                         QPainter *painter,
                         const QColorGroup &colorGroup,
                         int column, int width, int)
{
    if(width < item->pixmap(column)->width())
        return;

    if(m_needsRefresh)
        updateHeights();

    QFontMetrics fm = painter->fontMetrics();

    int y = item->listView()->itemMargin() + border;
    const QPixmap *pm = item->pixmap(column);

    if(item->isSelected()) {

        painter->eraseRect(0, 0, width, item->height());

        QPen oldPen = painter->pen();
        QPen newPen = oldPen;

        newPen.setWidth(5);
        newPen.setJoinStyle(Qt::RoundJoin);
        newPen.setColor(QPalette::Highlight);

        painter->setPen(newPen);
        painter->drawRect(border, border, width - border * 2, item->height() - border * 2 + 1);
        painter->setPen(oldPen);

        painter->fillRect(border, border, width - border * 2, item->height() - border * 2 + 1,
                          colorGroup.brush(QPalette::Highlight));
        painter->setPen(colorGroup.color( QPalette::HighlightedText));
    }
    else
        painter->eraseRect(0, 0, width, item->height());

    if(!pm->isNull()) {
        int x = (width - pm->width()) / 2;
        x = qMax(x, item->listView()->itemMargin());
        painter->drawPixmap(x, y, *pm);
    }

    y += pm->height() + fm.height() - fm.descent();

    foreach(QString line, m_lines[item]) {
        int x = (width - fm.width(line)) / 2;
        x = qMax(x, item->listView()->itemMargin());
        painter->drawText(x, y, line);
        y += fm.height() - fm.descent();
    }

    if(item == item->listView()->dropItem())
        paintDropIndicator(painter, width, item->height());
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
        updateIcons(32);
        m_needsRefresh = true;
    }
}

void ViewMode::updateIcons(int size)
{
    for(Q3ListViewItemIterator it(m_playlistBox); it.current(); ++it) {
        PlaylistBox::Item *i = static_cast<PlaylistBox::Item *>(*it);
        i->setPixmap(0, SmallIcon(i->iconName(), size));
    }
}

void ViewMode::setupItem(PlaylistBox::Item *item) const
{
    const PlaylistBox *box = item->listView();
    const int width = box->width() - box->verticalScrollBar()->width() - border * 2;
    const int baseHeight = 2 * box->itemMargin() + 32 + border * 2;
    const QFontMetrics fm = box->fontMetrics();
    item->setHeight(baseHeight + (fm.height() - fm.descent()) * lines(item, fm, width).count());
}

void ViewMode::updateHeights()
{
    const int width = m_playlistBox->width() - m_playlistBox->verticalScrollBar()->width() - border * 2;

    const int baseHeight = 2 * m_playlistBox->itemMargin() + 32 + border * 2;
    const QFontMetrics fm = m_playlistBox->fontMetrics();

    for(Q3ListViewItemIterator it(m_playlistBox); it.current(); ++it) {
        PlaylistBox::Item *i = static_cast<PlaylistBox::Item *>(it.current());
        m_lines[i] = lines(i, fm, width);
        const int height = baseHeight + (fm.height() - fm.descent()) * m_lines[i].count();
        i->setHeight(height);
    }

    m_needsRefresh = false;
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

QStringList ViewMode::lines(const PlaylistBox::Item *item,
                            const QFontMetrics &fm,
                            int width)
{
    // Here 32 is a bit arbitrary, but that's the width of the icons in this
    // mode and seems to a reasonable toLower bound.

    if(width < 32)
        return QStringList();

    QString line = item->text();

    QStringList l;

    while(!line.isEmpty()) {
        int textLength = line.length();
        while(textLength > 0 &&
              fm.width(line.mid(0, textLength).trimmed()) +
              item->listView()->itemMargin() * 2 > width)
        {
            int i = line.lastIndexOf(QRegExp( "\\W"), textLength - 1);
            if(i > 0)
                textLength = i;
            else
                textLength--;
        }

        l.append(line.mid(0, textLength).trimmed());
        line = line.mid(textLength);
    }
    return l;
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

void CompactViewMode::paintCell(PlaylistBox::Item *item,
                                QPainter *painter,
                                const QColorGroup &colorGroup,
                                int column, int width, int align)
{
    item->K3ListViewItem::paintCell(painter, colorGroup, column, width, align);
    if(item == item->listView()->dropItem())
        paintDropIndicator(painter, width, item->height());
}

QString CompactViewMode::name() const
{
    return i18nc("compact viewing mode", "Compact");
}

void CompactViewMode::setShown(bool shown)
{
    setVisible(shown);

    if(shown) {
        updateIcons(16);
        updateHeights();
    }
}

void CompactViewMode::updateHeights()
{
    for(Q3ListViewItemIterator it(playlistBox()); it.current(); ++it)
        it.current()->setup();
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
        PlaylistBox::Item *collectionItem = PlaylistBox::Item::collectionItem();

        if(!collectionItem)
            return;

        if(collectionItem && m_searchCategories.isEmpty())
            setupDynamicPlaylists();
        else {
            foreach(PlaylistBox::Item *item, m_searchCategories)
                item->setVisible(true);
        }

        if(!m_setup) {
            m_setup = true;
            playlistBox()->setSorting(-1);
            CollectionList::instance()->setupTreeViewEntries(this);
            playlistBox()->setSorting(0);
            playlistBox()->sort();
        }
    }
    else {
        foreach(PlaylistBox::Item *item, m_searchCategories)
            item->setVisible(false);
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
        kWarning() << "Unhandled column type " << column;
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
        kWarning() << "Unhandled column type " << column;
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

    QString itemKey, item;
    PlaylistBox::Item *itemParent = m_searchCategories.value(searchCategory, 0);

    foreach(item, items) {
        itemKey = searchCategory + item;

        if(m_treeViewItems.contains(itemKey))
            continue;

        components.clear();
        components.append(PlaylistSearch::Component(item, false, columns, mode));

        PlaylistSearch s(playlists, components, PlaylistSearch::MatchAny, false);

        TreeViewItemPlaylist *p = new TreeViewItemPlaylist(playlistBox(), s, item);
        playlistBox()->setupPlaylist(p, "midi", itemParent);
        m_treeViewItems.insert(itemKey, p);
    }
}

void TreeViewMode::setDynamicListsFrozen(bool frozen)
{
    m_dynamicListsFrozen = frozen;

    if(frozen)
        return;

    foreach(QString pendingItem, m_pendingItemsToRemove) {
        m_treeViewItems[pendingItem]->deleteLater();
        m_treeViewItems.remove(pendingItem);
    }

    m_pendingItemsToRemove.clear();
}

void TreeViewMode::setupDynamicPlaylists()
{
    PlaylistBox::Item *i;
    PlaylistBox::Item *collectionItem = PlaylistBox::Item::collectionItem();

    i = new PlaylistBox::Item(collectionItem, "cdimage", i18n("Artists"));
    m_searchCategories.insert("artists", i);

    i = new PlaylistBox::Item(collectionItem, "cdimage", i18n("Albums"));
    m_searchCategories.insert("albums", i);

    i = new PlaylistBox::Item(collectionItem, "cdimage", i18n("Genres"));
    m_searchCategories.insert("genres", i);
}

#include "viewmode.moc"

// vim: set et sw=4 tw=0 sta:
