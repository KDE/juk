/***************************************************************************
                          viewmode.cpp
                             -------------------
    begin                : Sat Jun 7 2003
    copyright            : (C) 2003 by Scott Wheeler, 
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

#include <kiconloader.h>
#include <kdebug.h>

#include <qpixmap.h>
#include <qpainter.h>

#include "viewmode.h"
#include "playlistbox.h"
#include "searchplaylist.h"
#include "collectionlist.h"

////////////////////////////////////////////////////////////////////////////////
// ViewMode
////////////////////////////////////////////////////////////////////////////////

ViewMode::ViewMode(PlaylistBox *b) : QObject(b),
    m_playlistBox(b),
    m_visible(false),
    m_needsRefresh(false)
{
    connect(this, SIGNAL(signalCreateSearchList(const PlaylistSearch &, const QString &, const QString &)),
            b, SIGNAL(signalCreateSearchList(const PlaylistSearch &, const QString &, const QString &)));

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

    int y = item->listView()->itemMargin();
    const QPixmap *pm = item->pixmap(column);

    if(item->isSelected()) {
        painter->fillRect(0, 0, width, item->height(), colorGroup.brush(QColorGroup::Highlight));
        painter->setPen(colorGroup.highlightedText());
    }
    else
        painter->eraseRect(0, 0, width, item->height());

    if (!pm->isNull()) {
        int x = (width - pm->width()) / 2;
        x = QMAX(x, item->listView()->itemMargin());
        painter->drawPixmap(x, y, *pm);
    }
    y += pm->height() + fm.height() - fm.descent();
    for(QStringList::Iterator it = m_lines[item].begin(); it != m_lines[item].end(); ++it) {
        int x = (width - fm.width(*it)) / 2;
        x = QMAX(x, item->listView()->itemMargin());
        painter->drawText(x, y, *it);
        y += fm.height() - fm.descent();
    }

    if(item == item->listView()->dropItem())
        paintDropIndicator(painter, width, item->height());
}

PlaylistBox::Item *ViewMode::createSearchItem(PlaylistBox *box, SearchPlaylist *playlist,
                                              const QString &)
{
    return new PlaylistBox::Item(box, "midi", playlist->name(), playlist);
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
    for(QListViewItemIterator it(m_playlistBox); it.current(); ++it) {
        PlaylistBox::Item *i = static_cast<PlaylistBox::Item *>(*it);
        i->setPixmap(0, SmallIcon(i->iconName(), size));
    }
}



void ViewMode::updateHeights()
{
    const int width = m_playlistBox->viewport()->width();

    const int baseHeight = 3 * m_playlistBox->itemMargin() + 32;
    const QFontMetrics fm = m_playlistBox->fontMetrics();

    for(QListViewItemIterator it(m_playlistBox); it.current(); ++it) {
        PlaylistBox::Item *i = static_cast<PlaylistBox::Item *>(it.current());
        m_lines[i] = lines(i, fm, width);
        const int height = baseHeight + (fm.height() - fm.descent()) * m_lines[i].count();
        i->setHeight(height);
        i->invalidateHeight();
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
    newPen.setStyle(DotLine);

    painter->setPen(newPen);
    painter->drawRect(border, border, width - border * 2, height - border * 2);
    painter->setPen(oldPen);
}

QStringList ViewMode::lines(const PlaylistBox::Item *item,
			    const QFontMetrics &fm,
			    int width) const
{
    // Here 32 is a bit arbitrary, but that's the width of the icons in this
    // mode and seems to a reasonable lower bound.

    if(width < 32)
	return QStringList();

    QString line = item->text();

    QStringList l;

    while(!line.isEmpty()) {
        int textLength = line.length();
        while(textLength > 0 && 
              fm.width(line.mid(0, textLength).stripWhiteSpace()) +
	      item->listView()->itemMargin() * 2 > width)
        {
            int i = line.findRev(QRegExp( "\\W"), textLength - 1);
            if(i > 0)
                textLength = i;
            else
                textLength--;
        }
        
        l.append(line.mid(0, textLength).stripWhiteSpace());
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
    item->KListViewItem::paintCell(painter, colorGroup, column, width, align);
    if(item == item->listView()->dropItem())
        paintDropIndicator(painter, width, item->height());
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
    for(QListViewItemIterator it(playlistBox()); it.current(); ++it)
	it.current()->setup();
}

////////////////////////////////////////////////////////////////////////////////
// TreeViewMode
////////////////////////////////////////////////////////////////////////////////

TreeViewMode::TreeViewMode(PlaylistBox *b) : CompactViewMode(b)
{

}

TreeViewMode::~TreeViewMode()
{

}

void TreeViewMode::setShown(bool show)
{
    CompactViewMode::setShown(show);

    playlistBox()->setRootIsDecorated(show);

    if(show) {
        PlaylistBox::Item *collectionItem = PlaylistBox::Item::collectionItem();

        if(!collectionItem) {
            connect(playlistBox(), SIGNAL(signalCollectionInitialized()),
                    this, SLOT(slotSetupCategories()));
            return;
        }

        if(collectionItem && m_searchCategories.isEmpty())
            slotSetupCategories();
        else {
            for(QDictIterator<PlaylistBox::Item> it(m_searchCategories); it.current(); ++it)
                it.current()->setVisible(true);
	}
    }
    else {
        for(QDictIterator<PlaylistBox::Item> it(m_searchCategories); it.current(); ++it)
            it.current()->setVisible(false);
    }
}

PlaylistBox::Item *TreeViewMode::createSearchItem(PlaylistBox *, SearchPlaylist *playlist,
						  const QString &searchCategory)
{
    return new PlaylistBox::Item(m_searchCategories[searchCategory], "midi", playlist->name(), playlist);
}

void TreeViewMode::setupCategory(const QString &searchCategory, const QStringList &members, int column, bool exact)
{
    CollectionList *collection = CollectionList::instance();
    QValueList<int> columns;
    columns.append(column);

    KApplication::setOverrideCursor(Qt::waitCursor);

    for(QStringList::ConstIterator it = members.begin(); it != members.end(); ++it) {
        
        PlaylistSearch::ComponentList components;

	PlaylistSearch::Component::MatchMode mode;
	if(exact)
	    mode = PlaylistSearch::Component::Exact;
	else
	    mode = PlaylistSearch::Component::ContainsWord;

        components.append(PlaylistSearch::Component(*it, true, columns, mode));

        PlaylistList playlists;
        playlists.append(collection);

        PlaylistSearch s(playlists, components, PlaylistSearch::MatchAny, false);

        emit signalCreateSearchList(s, searchCategory, *it);

        static int i = 0;
        if(++i % 5 == 0)
            kapp->processEvents();
    }

    KApplication::restoreOverrideCursor();
}

void TreeViewMode::slotSetupCategories()
{
    PlaylistBox::Item *i;
    
    PlaylistBox::Item *collectionItem = PlaylistBox::Item::collectionItem();
    
    i = new PlaylistBox::Item(collectionItem, "cdimage", i18n("Artists"));
    m_searchCategories.insert("artists", i);
    setupCategory("artists", CollectionList::instance()->uniqueSet(CollectionList::Artists),
		  PlaylistItem::ArtistColumn, false);

    i = new PlaylistBox::Item(collectionItem, "cdimage", i18n("Albums"));
    m_searchCategories.insert("albums", i);
    setupCategory("albums", CollectionList::instance()->uniqueSet(CollectionList::Albums),
		  PlaylistItem::AlbumColumn);

    i = new PlaylistBox::Item(collectionItem, "cdimage", i18n("Genres"));
    m_searchCategories.insert("genres", i);
    setupCategory("genres", CollectionList::instance()->uniqueSet(CollectionList::Genres),
		  PlaylistItem::GenreColumn);

    for(QDictIterator<PlaylistBox::Item> it(m_searchCategories); it.current(); ++it)
        it.current()->setSortedFirst(true);
}

#include "viewmode.moc"
