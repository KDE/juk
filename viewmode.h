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

#ifndef VIEWMODE_H
#define VIEWMODE_H


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

    virtual bool eventFilter(QObject *watched, QEvent *e);
    void queueRefresh() { m_needsRefresh = true; }

    virtual void setupItem(PlaylistBox::Item *item) const;

    virtual void setupDynamicPlaylists() {}
    /**
     * If the view mode has dynamic lists, this function is used to temporarily
     * freeze them to prevent them from deleting dynamic elements.
     */
    virtual void setDynamicListsFrozen(bool /* frozen */) {}

    /**
     * Used for dynamic view modes.  This function will be called when \p items
     * are added to \p column (even if the view mode hasn't been shown yet).
     */
    virtual void addItems(const QStringList &items, unsigned column)
    {
	(void) items;
	(void) column;
    }

    /**
     * Used for dynamic view modes.  This function will be called when \p item
     * is removed from \p column (even if the view mode hasn't been shown yet).
     */
    virtual void removeItem(const QString &item, unsigned column)
    {
	(void) item;
	(void) column;
    }

protected:
    PlaylistBox *playlistBox() const { return m_playlistBox; }
    bool visible() const { return m_visible; }
    void setVisible(bool v) { m_visible = v; }
    void updateIcons(int size);
    virtual void updateHeights();
    static void paintDropIndicator(QPainter *painter, int width, int height);

private:
    static QStringList lines(const PlaylistBox::Item *item, const QFontMetrics &fm, int width);

    PlaylistBox *m_playlistBox;
    bool m_visible;
    bool m_needsRefresh;
    QMap<PlaylistBox::Item *, QStringList> m_lines;
    static const int border = 4;
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

    virtual void setupItem(PlaylistBox::Item *item) const { item->KListViewItem::setup(); }
protected:
    virtual void updateHeights();
};

////////////////////////////////////////////////////////////////////////////////

class TreeViewItemPlaylist;

class TreeViewMode : public CompactViewMode
{
    Q_OBJECT

public:
    TreeViewMode(PlaylistBox *l);
    virtual ~TreeViewMode();

    virtual QString name() const { return i18n("Tree"); }
    virtual void setShown(bool shown);
    virtual void setupDynamicPlaylists();
    virtual void setDynamicListsFrozen(bool frozen);

    virtual void removeItem(const QString &item, unsigned column);
    virtual void addItems(const QStringList &items, unsigned column);

signals:
    void signalPlaylistDestroyed(Playlist*);

private:
    QDict<PlaylistBox::Item> m_searchCategories;    
    QDict<TreeViewItemPlaylist> m_treeViewItems;
    QStringList m_pendingItemsToRemove;
    bool m_dynamicListsFrozen;
    bool m_setup;
};

////////////////////////////////////////////////////////////////////////////////

class CoverManagerMode : public ViewMode
{
    Q_OBJECT

public:
    CoverManagerMode(PlaylistBox *b);
    virtual QString name() const { return i18n("Cover Manager"); }
    //virtual void setShown(bool shown);
};

#endif
