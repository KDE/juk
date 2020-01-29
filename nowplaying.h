/**
 * Copyright (C) 2004 Scott Wheeler <wheeler@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NOWPLAYING_H
#define NOWPLAYING_H

#include <QWidget>
#include <QLabel>
#include <QPointer>
#include <QList>

#include "filehandle.h"
#include "playlistinterface.h"

class QTimer;
class QPoint;

class NowPlayingItem;
class PlaylistCollection;
class Playlist;

/**
 * This is the widget that holds all of the other items and handles updating them
 * when the playing item changes.
 */

class NowPlaying : public QWidget
{
    Q_OBJECT

public:
    NowPlaying(QWidget *parent, PlaylistCollection *collection);
    void addItem(NowPlayingItem *item);
    PlaylistCollection *collection() const;

public slots:
    void slotUpdate(const FileHandle &file);
    void slotReloadCurrentItem();

signals:
    void nowPlayingHidden();

private:
    struct Observer final
    {
        Observer(NowPlaying *parent, PlaylistInterface *playlist)
        {
            connect(&playlist->signaller, &PlaylistInterfaceSignaller::playingItemDataChanged, parent, &NowPlaying::slotReloadCurrentItem);
        }
    };

    Observer m_observer;
    Observer m_collectionListObserver;
    PlaylistCollection *m_collection;
    QList<NowPlayingItem *> m_items;
    FileHandle m_file;
};

/**
 * Abstract base for the other NowPlaying items.
 */

class NowPlayingItem
{
public:
    virtual ~NowPlayingItem() {}
    virtual void update(const FileHandle &file) = 0;
    NowPlaying *parentManager() const { return m_parent; }
protected:
    NowPlayingItem(NowPlaying *parent) : m_parent(parent) { parent->addItem(this); }
private:
    NowPlaying *m_parent;
};

/**
 * Displays the cover of the currently playing file if available, or hides
 * itself if not.
 */

class CoverItem : public QLabel, public NowPlayingItem
{
public:
    explicit CoverItem(NowPlaying *parent);
    virtual void update(const FileHandle &file) override;
    virtual void mouseReleaseEvent(QMouseEvent *event) override;

protected:
    virtual void dragEnterEvent(QDragEnterEvent *e) override;
    virtual void dropEvent(QDropEvent *e) override;

    virtual void mousePressEvent(QMouseEvent *e) override;
    virtual void mouseMoveEvent(QMouseEvent *e) override;

private:
    FileHandle m_file;
    bool m_dragging;
    QPoint m_dragStart;
};

/**
 * Show the text information on the current track and provides links to the
 * album and artist of the currently playing item.
 */

class TrackItem : public QWidget, public NowPlayingItem
{
    Q_OBJECT

public:
    explicit TrackItem(NowPlaying *parent);
    virtual void update(const FileHandle &file) override;

private slots:
    void slotOpenLink(const QString &link);
    void slotUpdate();
    void slotClearShowMore();

private:
    FileHandle m_file;
    QLabel *m_label;
};

#endif

// vim: set et sw=4 tw=0 sta:
