/*
    Copyright (c) 2001 Matthias Hoelzer-Kluepfel <mhk@kde.org>
    Copyright (c) 2002 Daniel Molkentin <molkentin@kde.org>
    Copyright (c) 2002 Scott Wheeler <wheeler@kde.org>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <qdrawutil.h>

#include "listboxpixmap.h"

class ListBoxPixmap::ListBoxPixmapPrivate
{
public:
    ListBoxPixmapPrivate() {
	orientation = Qt::Horizontal;
    }

    Qt::Orientation orientation;
};

ListBoxPixmap::ListBoxPixmap(QListBox *listbox, const QPixmap &pixmap) 
    : QListBoxPixmap(listbox, pixmap)
{
    init();
}

ListBoxPixmap::ListBoxPixmap(const QPixmap &pixmap) 
    : QListBoxPixmap(pixmap)
{
    init();
}

ListBoxPixmap::ListBoxPixmap(QListBox *listbox, const QPixmap &pixmap, QListBoxItem *after) 
    : QListBoxPixmap(listbox, pixmap, after)
{
    init();
}

ListBoxPixmap::ListBoxPixmap(QListBox *listbox, const QPixmap &pix, const QString &text)
    : QListBoxPixmap(listbox, pix, text)
{
    init();
}

ListBoxPixmap::ListBoxPixmap(const QPixmap &pix, const QString &text)
    : QListBoxPixmap(pix, text)
{
    init();
}

ListBoxPixmap::ListBoxPixmap(QListBox *listbox, const QPixmap &pix, const QString &text, QListBoxItem *after)
    : QListBoxPixmap(listbox, pix, text, after)
{
    init();
}

ListBoxPixmap::~ListBoxPixmap()
{
    delete d;
}

int ListBoxPixmap::width(const QListBox *listbox) const
{
    if(d->orientation == Qt::Horizontal)
	QListBoxPixmap::width(listbox);

    return listbox->viewport()->width();
}

int ListBoxPixmap::height(const QListBox *listbox) const 
{
    if(d->orientation == Qt::Horizontal)
	QListBoxPixmap::height(listbox);

    int min = listbox->fontMetrics().lineSpacing() + pixmap()->height() + 6;
    return min;
}

Qt::Orientation ListBoxPixmap::orientation() const
{
    return d->orientation;
}

void ListBoxPixmap::setOrientation(Qt::Orientation o)
{
    d->orientation = o;
    listBox()->repaint();
}

void ListBoxPixmap::paint(QPainter *painter)
{
    // Ripped out of Kaplan, which Danimo said was just ripped out of something 
    // else.
    
    if(d->orientation == Qt::Horizontal) {
	QListBoxPixmap::paint(painter);
	return;
    }

    QListBox *box = listBox();

    int w = width( box );
    static const int margin = 3;
    int y = margin;
    const QPixmap *pm = pixmap();
    
    if ( !pm->isNull() ) {
	int x = (w - pm->width()) / 2;
	x = QMAX( x, margin );
	painter->drawPixmap( x, y, *pm );
    }
    
    if ( !text().isEmpty() ) {
	QFontMetrics fm = painter->fontMetrics();
	y += pm->height() + fm.height() - fm.descent();
	int x = (w - fm.width( text() )) / 2;
	x = QMAX( x, margin );
	painter->drawText( x, y, text() );
    }
    // draw sunken
    if ( isCurrent() || isSelected() ) {
	qDrawShadePanel( painter, 1, 0, w -2, height(box),
			 box->colorGroup(), true, 1, 0L );
    }
}

void ListBoxPixmap::init()
{
    d = new ListBoxPixmapPrivate();
}
