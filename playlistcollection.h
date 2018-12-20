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

#ifndef PLAYLIST_COLLECTION_H
#define PLAYLIST_COLLECTION_H

#include "stringhash.h"
#include "playlistinterface.h"

#include <KDirLister>
#include <KLocalizedString>

#include <QPointer>
#include <QKeySequence>

class QPixmap;
class QStackedWidget;

class QAction;

class HistoryPlaylist;
class UpcomingPlaylist;
class SearchPlaylist;
class DynamicPlaylist;
class PlaylistItem;
class Playlist;
class PlayerManager;
class FileHandle;

template<class T>
class QVector;

typedef QVector<PlaylistItem *> PlaylistItemList;
typedef QVector<Playlist *> PlaylistList;

class PlaylistCollection : public PlaylistInterface
{
    friend class Playlist;
    friend class CollectionList;
    friend class DynamicPlaylist;

public:
    PlaylistCollection(PlayerManager *player, QStackedWidget *playlistStack);
    virtual ~PlaylistCollection();

    static PlaylistCollection *instance() { return m_instance; }

    virtual QString name() const;
    virtual FileHandle currentFile() const;
    virtual int count() const;
    virtual int time() const;
    virtual void playNext();
    virtual void playPrevious();
    virtual void stop();
    virtual bool playing() const;

    void playFirst();
    void playNextAlbum();

    virtual QStringList playlists() const;
    virtual void createPlaylist(const QString &name);
    virtual void createDynamicPlaylist(const PlaylistList &playlists);
    virtual void showMore(const QString &artist, const QString &album = QString());
    virtual void removeTrack(const QString &playlist, const QStringList &files);

    virtual QString playlist() const;
    virtual QString playingPlaylist() const;
    virtual void setPlaylist(const QString &playlist);

    virtual QStringList playlistTracks(const QString &playlist) const;
    virtual QString trackProperty(const QString &file, const QString &property) const;
    virtual QPixmap trackCover(const QString &file, const QString &size = "Small") const;

    virtual void open(const QStringList &files = QStringList());
    virtual void open(const QString &playlist, const QStringList &files);
    virtual void addFolder();
    virtual void rename();
    virtual void duplicate();
    virtual void save();
    virtual void saveAs();
    virtual void remove() = 0;
    virtual void reload();
    virtual void editSearch();
    virtual void setDynamicListsFrozen(bool) = 0;

    bool showMoreActive() const;
    void clearShowMore(bool raise = true);
    void enableDirWatch(bool enable);

    void removeItems();
    void refreshItems();
    void renameItems();
    void addCovers(bool fromFile);
    void removeCovers();
    void viewCovers();
    void showCoverManager();

    virtual PlaylistItemList selectedItems();

    // virtual to allow our QWidget subclass to emit a signal after we're done
    virtual void scanFolders();

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

    void dirChanged(const QString &path);

    /**
     * Returns a pointer to the action handler.
     */
    QObject *object() const;

    void newItems(const KFileItemList &list) const;

    /**
     * This is the current playlist in all things relating to the player.  It
     * represents the playlist that either should be played from or is currently
     * playing.
     */
    virtual Playlist *currentPlaylist() const;

    /**
     * This is the currently visible playlist and should be used for all user
     * interaction elements.
     */
    virtual Playlist *visiblePlaylist() const;

    /**
     * Makes \a playlist the currently visible playlist.
     */
    virtual void raise(Playlist *playlist);

    /**
     * @return true, if a playlist with the file name given in @p file is
     * already loaded into this collection, or false otherwise.
     *
     * @note @p file should be the "canonical" full path to the file to avoid
     * problems with duplicates and symlinks.
     */
    bool containsPlaylistFile(const QString &file) const;

    /**
     * @return list of folders to exclude from automatic searching (whether
     * by directory-change watchers or the startup folder scan). The user should
     * still be able to manually add files even under an excluded folder.
     */
    QStringList excludedFolders() const { return m_excludedFolderList; }

    class ActionHandler;
    friend PlaylistCollection::ActionHandler;

protected:
    virtual QStackedWidget *playlistStack() const;
    virtual void setupPlaylist(Playlist *playlist, const QString &iconName);
    virtual void removePlaylist(Playlist *playlist) = 0;

    bool importPlaylists() const;

    QString playlistNameDialog(const QString &caption = i18n("Create New Playlist"),
                               const QString &suggest = QString(),
                               bool forceUnique = true) const;
    QString uniquePlaylistName(const QString &suggest = i18n("Playlist")) const;

    void addNameToDict(const QString &name);
    void addFileToDict(const QString &file);
    void removeNameFromDict(const QString &name);
    void removeFileFromDict(const QString &file);

    Playlist *playlistByName(const QString &name) const;

private:
    void readConfig();
    void saveConfig();

    QStackedWidget   *m_playlistStack;
    HistoryPlaylist  *m_historyPlaylist;
    UpcomingPlaylist *m_upcomingPlaylist;
    ActionHandler    *m_actionHandler;
    PlayerManager    *m_playerManager;

    KDirLister  m_dirLister;
    StringHash  m_playlistNames;
    StringHash  m_playlistFiles;
    QStringList m_folderList;
    QStringList m_excludedFolderList;
    bool        m_importPlaylists;
    bool        m_searchEnabled;
    bool        m_playing;

    QPointer<SearchPlaylist> m_showMorePlaylist;
    QPointer<Playlist> m_belowShowMorePlaylist;
    QPointer<DynamicPlaylist> m_dynamicPlaylist;
    QPointer<Playlist> m_belowDistraction;

    QWidget *m_distraction;

    static PlaylistCollection *m_instance;
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
    QAction *createAction(const QString &text,
                          const char *slot,
                          const char *name,
                          const QString &icon = QString(),
                          const QKeySequence &shortcut = QKeySequence());
private slots:
    void slotStop()          { m_collection->stop(); }
    void slotPlayFirst()     { m_collection->playFirst(); }
    void slotPlayNextAlbum() { m_collection->playNextAlbum(); }

    void slotOpen()         { m_collection->open(); }
    void slotManageFolders()    { m_collection->addFolder(); }
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

    void slotViewCovers()   { m_collection->viewCovers(); }
    void slotRemoveCovers() { m_collection->removeCovers(); }
    void slotAddLocalCover()    { m_collection->addCovers(true); }
    void slotAddInternetCover() { m_collection->addCovers(false); }

    void slotCreatePlaylist()       { m_collection->createPlaylist(); }
    void slotCreateSearchPlaylist() { m_collection->createSearchPlaylist(); }
    void slotCreateFolderPlaylist() { m_collection->createFolderPlaylist(); }

    void slotGuessTagFromFile()     { m_collection->guessTagFromFile(); }
    void slotGuessTagFromInternet() { m_collection->guessTagFromInternet(); }

    void slotSetSearchEnabled(bool enable)           { m_collection->setSearchEnabled(enable); }
    void slotSetUpcomingPlaylistEnabled(bool enable) { m_collection->setUpcomingPlaylistEnabled(enable); }
    void slotShowCoverManager()                      { m_collection->showCoverManager(); }
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

// vim: set et sw=4 tw=0 sta:
