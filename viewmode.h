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

#include <qobject.h>
#include <qstring.h>
#include <qdict.h>

#include "playlistbox.h"

class QPainter;
class QColorGroup;

class SearchPlaylist;

class ViewMode : public QObject
{
    Q_OBJECT

public:
    ViewMode(PlaylistBox *b);
    virtual ~ViewMode();

    virtual QString name() const { return i18n("Default"); }
    virtual void setShown(bool shown);

    virtual void paintCell(PlaylistBox::Item *item,
                           QPainter *painter, 
                           const QColorGroup &colorGroup,
                           int column, int width, int align);

    virtual PlaylistBox::Item *createSearchItem(PlaylistBox *box,
                                                SearchPlaylist *playlist,
                                                const QString &searchCategory);

    virtual bool eventFilter(QObject *watched, QEvent *e);
    

    void queueRefresh() { m_needsRefresh = true; }

protected:
    PlaylistBox *playlistBox() const { return m_playlistBox; }
    bool visible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    void updateIcons(int size);

protected:
    virtual void updateHeights();

signals:
    void signalCreateSearchList(const PlaylistSearch &search,
                                const QString &searchCategory,
                                const QString &name);
    
private:
    QStringList lines(const PlaylistBox::Item *item, const QFontMetrics &fm, int width) const;

    PlaylistBox *m_playlistBox;
    bool m_visible;
    bool m_needsRefresh;
    QMap<PlaylistBox::Item *, QStringList> m_lines;
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
protected:
    /**
     * Override the implementation from the base class.  This isn't needed here.
     */
    virtual void updateHeights();
};

////////////////////////////////////////////////////////////////////////////////

class TreeViewMode : public CompactViewMode
{
    Q_OBJECT

public:
    TreeViewMode(PlaylistBox *l);
    virtual ~TreeViewMode();

    virtual QString name() const { return i18n("Tree"); }
    virtual void setShown(bool shown);

    virtual PlaylistBox::Item *createSearchItem(PlaylistBox *box,
                                                SearchPlaylist *playlist,
                                                const QString &searchCategory);

private:
    void setupCategory(const QString &searchCategory, const QStringList &members,
		       int column, bool exact = true);

private slots:
    /**
     * Setup all of the categories.
     */
    void slotSetupCategories();

private:
    QDict<PlaylistBox::Item> m_searchCategories;    
};

#endif
