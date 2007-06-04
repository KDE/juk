/***************************************************************************
    begin                : Sat Jun 7 2003
    copyright            : (C) 2003 - 2004 by Scott Wheeler,
    email                : wheeler@kde.org
    copyright            : (c) 2007 Michael Pyne
    email                : michael.pyne@kdemail.net
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

#include <QObject>
#include <QStringList>
#include <QMap>

#include "playlistbox.h"

class QPainter;
class QColorGroup;


class ViewMode : public QObject
{
    Q_OBJECT

public:
    ViewMode(PlaylistBox *b);
    virtual ~ViewMode();

    virtual QString name() const;
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
        Q_UNUSED(items);
        Q_UNUSED(column);
    }

    /**
     * Used for dynamic view modes.  This function will be called when \p item
     * is removed from \p column (even if the view mode hasn't been shown yet).
     */
    virtual void removeItem(const QString &item, unsigned column)
    {
        Q_UNUSED(item);
        Q_UNUSED(column);
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

    virtual QString name() const;
    virtual void setShown(bool shown);

    virtual void paintCell(PlaylistBox::Item *item,
                           QPainter *painter,
                           const QColorGroup &colorGroup,
                           int column, int width, int align);

    virtual void setupItem(PlaylistBox::Item *item) const { item->K3ListViewItem::setup(); }
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

    virtual QString name() const;
    virtual void setShown(bool shown);
    virtual void setupDynamicPlaylists();
    virtual void setDynamicListsFrozen(bool frozen);

    virtual void removeItem(const QString &item, unsigned column);
    virtual void addItems(const QStringList &items, unsigned column);

signals:
    void signalPlaylistDestroyed(Playlist*);

private:
    QMap<QString, PlaylistBox::Item*> m_searchCategories;
    QMap<QString, TreeViewItemPlaylist*> m_treeViewItems;
    QStringList m_pendingItemsToRemove;
    bool m_dynamicListsFrozen;
    bool m_setup;
};

#endif

// vim: set et sw=4 tw=0 sta:
