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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <kdebug.h>

#include <qpainter.h>
#include <qdrawutil.h>
#include <qstringlist.h>
#include <qregexp.h>

#include "listboxpixmap.h"

class ListBoxPixmap::ListBoxPixmapPrivate
{
public:
    ListBoxPixmapPrivate() {
	orientation = Qt::Horizontal;
	lineCount = 1;
    }

    Qt::Orientation orientation;
    uint lineCount;
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
	return QListBoxPixmap::width(listbox);

    return listbox->viewport()->width();
}

int ListBoxPixmap::height(const QListBox *listbox) const 
{
    if(d->orientation == Qt::Horizontal)
	return QListBoxPixmap::height(listbox);

    int min = listbox->fontMetrics().lineSpacing() * d->lineCount + pixmap()->height() + 6;
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
    // If we're using the default orientation, use the default paint method.

    if( d->orientation == Qt::Horizontal ) {
	QListBoxPixmap::paint( painter );
	return;
    }

    QListBox *box = listBox();

    int w = width( box );
    static const int margin = 3;
    int y = margin;
    const QPixmap *pm = pixmap();
    
    if ( !pm->isNull() ) {
	int x = ( w - pm->width() ) / 2;
	x = QMAX( x, margin );
	painter->drawPixmap( x, y, *pm );
    }
    
    if ( !text().isEmpty() ) {
	QFontMetrics fm = painter->fontMetrics();
	y += pm->height() + fm.height() - fm.descent();

	QStringList lines;
	QString line = text();

	while( !line.isEmpty() ) {
	    int textLength = line.length(); 
	    while( textLength > 0 && 
		   fm.width( line.mid( 0, textLength ).stripWhiteSpace() ) + margin * 2 > w &&
		   fm.width( line.mid( 0, textLength ).stripWhiteSpace() ) + margin * 2 > pm->width() ) {
		int i = line.findRev( QRegExp( "\\W" ), textLength - 1 );
		if( i > 0 )
		    textLength = i;
		else
		    textLength--;
	    }
	    
	    lines.append( line.mid( 0, textLength ).stripWhiteSpace() );
	    line = line.mid( textLength );
	}
	if( d->lineCount != lines.count() ) {
	    d->lineCount = lines.count();
	    listBox()->triggerUpdate( true );
	}
	
	for( QStringList::Iterator it = lines.begin(); it != lines.end(); ++it ) {
	    int x = (w - fm.width( *it )) / 2;
	    x = QMAX( x, margin );
	    painter->drawText( x, y, *it );
	    y += fm.height() - fm.descent();
	}


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
