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

////////////////////////////////////////////////////////////////////////////////
// ViewMode
////////////////////////////////////////////////////////////////////////////////

ViewMode::ViewMode(PlaylistBox *b) :
    m_playlistBox(b),
    m_visible(false)
{

}

ViewMode::~ViewMode()
{

}

void ViewMode::paintCell(PlaylistBox::Item *i,
                         QPainter *painter, 
                         const QColorGroup &colorGroup,
                         int column, int width, int align)
{
    Q_UNUSED(align);

    if(width < i->pixmap(column)->width())
	return;

    PlaylistBox::Item *item = static_cast<PlaylistBox::Item *>(i);

    QFontMetrics fm = painter->fontMetrics();
    QString line = item->text();

    QStringList lines;
    while(!line.isEmpty()) {
        int textLength = line.length();
        while(textLength > 0 && 
              fm.width(line.mid(0, textLength).stripWhiteSpace()) + item->listView()->itemMargin() * 2 > width)
        {
            int i = line.findRev(QRegExp( "\\W"), textLength - 1);
            if(i > 0)
                textLength = i;
            else
                textLength--;
        }
        
        lines.append(line.mid(0, textLength).stripWhiteSpace());
        line = line.mid(textLength);
    }

    int y = item->listView()->itemMargin();
    const QPixmap *pm = item->pixmap(column);

    int height = 3 * item->listView()->itemMargin() + pm->height() + 
        (fm.height() - fm.descent()) * lines.count();
	
    if(item->isSelected()) {
        painter->fillRect(0, 0, width, height, colorGroup.brush(QColorGroup::Highlight));
        painter->setPen(colorGroup.highlightedText());
    }
    else
        painter->eraseRect(0, 0, width, height);

    if (!pm->isNull()) {
        int x = (width - pm->width()) / 2;
        x = QMAX(x, item->listView()->itemMargin());
        painter->drawPixmap(x, y, *pm);
    }
    y += pm->height() + fm.height() - fm.descent();
    for(QStringList::Iterator it = lines.begin(); it != lines.end(); ++it) {
        int x = (width - fm.width(*it)) / 2;
        x = QMAX(x, item->listView()->itemMargin());
        painter->drawText(x, y, *it);
        y += fm.height() - fm.descent();
    }
    item->setHeight(height);
}

void ViewMode::setShown(bool shown)
{
    m_visible = shown;
    if(shown)
        updateIcons(32);
}

void ViewMode::updateIcons(int size)
{
    for(QListViewItemIterator it(m_playlistBox); it.current(); ++it) {
        PlaylistBox::Item *i = static_cast<PlaylistBox::Item *>(*it);
        i->setPixmap(0, SmallIcon(i->iconName(), size));
    }
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
#if 0 // don't think this code is needed anymore

    if(width < item->pixmap(column)->width())
	return;

    QFontMetrics fm = painter->fontMetrics();
    QString line = item->text();

    int baseWidth = item->pixmap(column)->width() + item->listView()->itemMargin() * 4;
    if(baseWidth + fm.width(line) > width) {
        int ellipsisLength = fm.width("...");
        if(width > baseWidth + ellipsisLength) {
            while(baseWidth + fm.width(line) + ellipsisLength > width)
                line.truncate(line.length() - 1);
            line = line.append("...");
        }
        else
            line = "...";
    }
    item->KListViewItem::setText(column, line);

#endif
    item->KListViewItem::paintCell(painter, colorGroup, column, width, align);
}

void CompactViewMode::setShown(bool shown)
{
    if(shown)
        updateIcons(16);
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
    if(show) {
	updateIcons(16);

	PlaylistBox::Item *collectionItem = PlaylistBox::Item::collectionItem();

	if(!collectionItem)
	    kdDebug(65432) << "TreeViewMode::setShown() - the CollectionList isn't initialized yet." << endl;
	
	if(collectionItem && m_categories.isEmpty()) {

	    PlaylistBox::Item *i;
	    
	    i = new PlaylistBox::Item(collectionItem, "cdimage", i18n("Artists"));
	    m_categories.insert("artists", i);

	    i = new PlaylistBox::Item(collectionItem, "cdimage", i18n("Albums"));
	    m_categories.insert("albums", i);


	    for(QDictIterator<PlaylistBox::Item> it(m_categories); it.current(); ++it)
		it.current()->setSortedFirst(true);
	}
	else {
	    for(QDictIterator<PlaylistBox::Item> it(m_categories); it.current(); ++it)
		it.current()->setVisible(true);
	}
    }
    else {
	for(QDictIterator<PlaylistBox::Item> it(m_categories); it.current(); ++it)
	    it.current()->setVisible(false);
    }

    playlistBox()->setRootIsDecorated(show);
}
