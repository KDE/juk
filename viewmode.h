/***************************************************************************
                          viewmode.h
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

#ifndef VIEWMODE_H
#define VIEWMODE_H

#include <klocale.h>

#include <qstring.h>
#include <qdict.h>

#include "playlistbox.h"

class QPainter;
class QColorGroup;

class ViewMode
{
public:
    ViewMode(PlaylistBox *b);
    virtual ~ViewMode();

    virtual QString name() const      { return i18n("Default"); }
    virtual void setShown(bool shown);

    virtual void paintCell(PlaylistBox::Item *item,
                           QPainter *painter, 
                           const QColorGroup &colorGroup,
                           int column, int width, int align);
    
protected:
    PlaylistBox *playlistBox() const { return m_playlistBox; }
    bool visible() const             { return m_visible; }
    void updateIcons(int size);

private:
    PlaylistBox *m_playlistBox;
    bool m_visible;
};

////////////////////////////////////////////////////////////////////////////////

class CompactViewMode : public ViewMode
{
public:
    CompactViewMode(PlaylistBox *b);
    virtual ~CompactViewMode();
    
    virtual QString name() const { return i18n("Compact"); }
    virtual void setShown(bool shown);

    virtual void paintCell(PlaylistBox::Item *item,
                           QPainter *painter,
                           const QColorGroup &colorGroup,
                           int column, int width, int align);
};

////////////////////////////////////////////////////////////////////////////////

class TreeViewMode : public CompactViewMode
{
public:
    TreeViewMode(PlaylistBox *l);
    virtual ~TreeViewMode();

    virtual QString name() const { return i18n("Tree"); }
    virtual void setShown(bool shown);

private:
    QDict<PlaylistBox::Item> m_categories;
};

#endif
