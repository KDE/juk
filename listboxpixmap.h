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

#ifndef LISTBOXPIXMAP_H
#define LISTBOXPIXMAP_H

#include <qlistbox.h>

class QPainter;

class ListBoxPixmap : public QListBoxPixmap
{
public: 
    ListBoxPixmap(QListBox *listbox, const QPixmap &pixmap);
    ListBoxPixmap(const QPixmap &pixmap);
    ListBoxPixmap(QListBox *listbox, const QPixmap &pixmap, QListBoxItem *after);
    ListBoxPixmap(QListBox *listbox, const QPixmap &pix, const QString &text);
    ListBoxPixmap(const QPixmap &pix, const QString &text);
    ListBoxPixmap(QListBox *listbox, const QPixmap &pix, const QString &text, QListBoxItem *after);
    virtual ~ListBoxPixmap();

    /**
     * returns the width of this item.
     */
    virtual int width(const QListBox *) const;

    /**
     * returns the height of this item.
     */
    virtual int height(const QListBox *) const;
    
    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation);

protected:
    virtual void paint(QPainter *painter);

private:
    void init();

    class ListBoxPixmapPrivate;
    ListBoxPixmapPrivate *d;
};

#endif
