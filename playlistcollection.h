/***************************************************************************
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

#ifndef PLAYLIST_COLLECTION_H
#define PLAYLIST_COLLECTION_H

#include "playlistinterface.h"
#include "stringhash.h"
#include "jukIface.h"

#include <kshortcut.h>
#include <kconfig.h>
#include <klocale.h>
#include <kdirlister.h>

class QWidgetStack;
class KAction;
class Playlist;
class PlaylistItem;
class HistoryPlaylist;
class UpcomingPlaylist;

typedef QValueList<PlaylistItem *> PlaylistItemList;

class PlaylistCollection : public PlaylistInterface, CollectionIface
{
    friend class Playlist;
    friend class CollectionList;
    friend class DynamicPlaylist;

public:
    PlaylistCollection(QWidgetStack *playlistStack);
    virtual ~PlaylistCollection();

    virtual QString name() const;
    virtual FileHandle currentFile() const;
    virtual int count() const;
    virtual int time() const;
    virtual void playNext();
    virtual void playPrevious();
    virtual void stop();
    virtual bool playing() const;

    void playFirst();

    virtual QStringList playlists() const;
    virtual void createPlaylist(const QString &name);
    virtual void removeTrack(const QString &playlist, const QStringList &files);

    virtual QString playlist() const;
    virtual void setPlaylist(const QString &playlist);

    virtual QStringList playlistTracks(const QString &playlist) const;
    virtual QString trackProperty(const QString &file, const QString &property) const;

    virtual void open(const QStringList &files = QStringList());
    virtual void open(const QString &playlist, const QStringList &files);
    virtual void addFolder();
    virtual void rename();
    virtual void duplicate();
    virtual void save();
    virtual void saveAs();
    virtual void remove();
    virtual void reload();
    virtual void editSearch();
    virtual void setCanDeletePlaylist(bool canDelete);

    void enableDirWatch(bool enable);

    void removeItems();
    void refreshItems();
    void renameItems();

    PlaylistItemList selectedItems();

    void scanFolders();

    void createPlaylist();
    void createSearchPlaylist();
    void createFolderPlaylist();

    void guessTagFromFile();
    void guessTagFromInternet();

    void setSearchEnabled(bool enable);

    HistoryPlaylist *historyPlaylist() const;
    void setHistoryPlaylistEnabled(bool enable);

    UpcomingPlaylist *upcomingPlaylist() const;
    void setUpcomingPlaylistEnabled(bool enable);

    /**
     * Sets the playlist without initializing it (for use by Cache before GUI
     * is completely setup).  Call setUpcomingPlaylistEnabled(true) when the
     * rest of initialization is ready.
     */
    void setUpcomingPlaylist(UpcomingPlaylist *p) { m_upcomingPlaylist = p; }

    void dirChanged(const QString &path);

    QObject *object() const;

    void newItems(const KFileItemList &list) const;

    class ActionHandler;

protected:
    virtual Playlist *currentPlaylist() const;
    virtual QWidgetStack *playlistStack() const;
    virtual void raise(Playlist *playlist);
    virtual void setupPlaylist(Playlist *playlist, const QString &iconName);

    bool importPlaylists() const;
    bool containsPlaylistFile(const QString &file) const;

    QString playlistNameDialog(const QString &caption = i18n("Create New Playlist"),
                               const QString &suggest = QString::null,
                               bool forceUnique = true) const;
    QString uniquePlaylistName(const QString &suggest = i18n("Playlist")) const;

    void addName(const QString &name);
    void removeName(const QString &name);

    Playlist *playlistByName(const QString &name) const;

private:
    void readConfig();
    void saveConfig();

    QWidgetStack     *m_playlistStack;
    HistoryPlaylist  *m_historyPlaylist;
    UpcomingPlaylist *m_upcomingPlaylist;
    ActionHandler    *m_actionHandler;

    KDirLister  m_dirLister;
    StringHash  m_playlistNames;
    StringHash  m_playlistFiles;
    QStringList m_folderList;
    bool        m_importPlaylists;
    bool        m_searchEnabled;
    bool        m_playing;
};

/**
 * This class is just used as a proxy to handle the signals coming from action
 * activations without requiring PlaylistCollection to be a QObject.
 */

class PlaylistCollection::ActionHandler : public QObject
{
    Q_OBJECT
public:
    ActionHandler(PlaylistCollection *collection);

private:
    KAction *createAction(const QString &text,
                          const char *slot,
                          const char *name,
                          const QString &icon = QString::null,
                          const KShortcut &shortcut = KShortcut());
private slots:
    void slotPlayFirst()    { m_collection->playFirst(); }

    void slotOpen()         { m_collection->open(); }
    void slotAddFolder()    { m_collection->addFolder(); }
    void slotRename()       { m_collection->rename(); }
    void slotDuplicate()    { m_collection->duplicate(); }
    void slotSave()         { m_collection->save(); }
    void slotSaveAs()       { m_collection->saveAs(); }
    void slotReload()       { m_collection->reload(); }
    void slotRemove()       { m_collection->remove(); }
    void slotEditSearch()   { m_collection->editSearch(); }

    void slotRemoveItems()  { m_collection->removeItems(); }
    void slotRefreshItems() { m_collection->refreshItems(); }
    void slotRenameItems()  { m_collection->renameItems(); }

    void slotScanFolders()  { m_collection->scanFolders(); }

    void slotCreatePlaylist()       { m_collection->createPlaylist(); }
    void slotCreateSearchPlaylist() { m_collection->createSearchPlaylist(); }
    void slotCreateFolderPlaylist() { m_collection->createFolderPlaylist(); }

    void slotGuessTagFromFile()     { m_collection->guessTagFromFile(); }
    void slotGuessTagFromInternet() { m_collection->guessTagFromInternet(); }

    void slotSetSearchEnabled(bool enable)           { m_collection->setSearchEnabled(enable); }
    void slotSetHistoryPlaylistEnabled(bool enable)  { m_collection->setHistoryPlaylistEnabled(enable); }
    void slotSetUpcomingPlaylistEnabled(bool enable) { m_collection->setUpcomingPlaylistEnabled(enable); }
    void slotEnableDirWatch(bool enable)             { m_collection->enableDirWatch(enable); }
    void slotDirChanged(const QString &path)         { m_collection->dirChanged(path); }

    void slotNewItems(const KFileItemList &list)     { m_collection->newItems(list); }

signals:
    void signalSelectedItemsChanged();
    void signalCountChanged();

private:
    PlaylistCollection *m_collection;
};

#endif
