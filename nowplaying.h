/***************************************************************************
    begin                : Tue Nov 9 2004
    copyright            : (C) 2004 by Scott Wheeler
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

#ifndef NOWPLAYING_H
#define NOWPLAYING_H

#include <kactivelabel.h>

#include <qhbox.h>
#include <qlabel.h>

#include "filehandle.h"

class NowPlayingItem;

/**
 * This is the widget that holds all of the other items and handles updating them
 * when the playing item changes.
 */

class NowPlaying : public QHBox
{
    Q_OBJECT

public:
    NowPlaying(QWidget *parent, const char *name = 0);
    void addItem(NowPlayingItem *item);

private slots:
    void slotUpdate();    

private:
    QValueList<NowPlayingItem *> m_items;
};

/**
 * Abstract base for the other NowPlaying items.
 */

class NowPlayingItem
{
public:
    virtual void update(const FileHandle &file) = 0;

protected:
    NowPlayingItem(NowPlaying *parent) { parent->addItem(this); }
};

/**
 * Displays the cover of the currently playing file if available, or hides
 * itself if not.
 */

class CoverItem : public QLabel, public NowPlayingItem
{
public:
    CoverItem(NowPlaying *parent);
    virtual void update(const FileHandle &file);
    virtual void mousePressEvent(QMouseEvent *event);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *e);
    virtual void dropEvent(QDropEvent *e);

private:
    FileHandle m_file;
};

/**
 * A link label that doesn't automatically open Konqueror.
 */

class LinkLabel : public KActiveLabel
{
public:
    LinkLabel(QWidget *parent, const char *name = 0) : KActiveLabel(parent, name) {}
    virtual void openLink(const QString &) {}
};

/**
 * Show the text information on the current track and provides links to the
 * album and artist of the currently playing item.
 */

class TrackItem : public QWidget, public NowPlayingItem
{
public:
    TrackItem(NowPlaying *parent);
    virtual void update(const FileHandle &file);

private:
    LinkLabel *m_label;
};

/**
 * Shows up to 10 items of history and links to those items.
 */

class HistoryItem : public LinkLabel, public NowPlayingItem
{
public:
    HistoryItem(NowPlaying *parent);
    virtual void update(const FileHandle &file);

private:
    FileHandleList m_history;
    LinkLabel *m_label;
};

#endif
