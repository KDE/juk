/**
 * Copyright (C) 2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2009 Michael Pyne <mpyne@kde.org>
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

#include "playlistcollection.h"

#include <kiconloader.h>
#include <kmessagebox.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <kactionmenu.h>
#include <kconfiggroup.h>
#include <KSharedConfig>
#include <kfileitem.h>

#include <config-juk.h>

#include <QAction>
#include <QIcon>
#include <QMutableListIterator>
#include <QObject>
#include <QPixmap>
#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QStackedWidget>

#include <sys/types.h>
#include <dirent.h>

#include "collectionlist.h"
#include "actioncollection.h"
#include "advancedsearchdialog.h"
#include "coverinfo.h"
#include "searchplaylist.h"
#include "folderplaylist.h"
#include "historyplaylist.h"
#include "upcomingplaylist.h"
#include "directorylist.h"
#include "mediafiles.h"
#include "playermanager.h"
#include "tracksequencemanager.h"
#include "juk.h"

//Laurent: readd it
//#include "collectionadaptor.h"

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// static methods
////////////////////////////////////////////////////////////////////////////////

PlaylistCollection *PlaylistCollection::m_instance = 0;

// Returns all folders in input list with their canonical path, if available, or
// unchanged if not.
static QStringList canonicalizeFolderPaths(const QStringList &folders)
{
    QStringList result;

    foreach(const QString &folder, folders) {
        QString canonicalFolder = QDir(folder).canonicalPath();
        result << (!canonicalFolder.isEmpty() ? canonicalFolder : folder);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistCollection::PlaylistCollection(PlayerManager *player, QStackedWidget *playlistStack) :
    m_playlistStack(playlistStack),
    m_historyPlaylist(0),
    m_upcomingPlaylist(0),
    m_playerManager(player),
    m_importPlaylists(true),
    m_searchEnabled(true),
    m_playing(false),
    m_showMorePlaylist(0),
    m_belowShowMorePlaylist(0),
    m_dynamicPlaylist(0),
    m_belowDistraction(0),
    m_distraction(0)
{
    //new CollectionAdaptor( this );
    //QDBus::sessionBus().registerObject("/Collection",this );
    m_instance = this;

    m_actionHandler = new ActionHandler(this);

    // KDirLister's auto error handling seems to crash JuK during startup in
    // readConfig().

    m_dirLister.setAutoErrorHandlingEnabled(false, playlistStack);
    readConfig();
}

PlaylistCollection::~PlaylistCollection()
{
    saveConfig();
    CollectionList::instance()->saveItemsToCache();
    delete m_actionHandler;
    Playlist::setShuttingDown();
}

QString PlaylistCollection::name() const
{
    return currentPlaylist()->name();
}

FileHandle PlaylistCollection::currentFile() const
{
    return currentPlaylist()->currentFile();
}

int PlaylistCollection::count() const
{
    return currentPlaylist()->count();
}

int PlaylistCollection::time() const
{
    return currentPlaylist()->time();
}

void PlaylistCollection::playFirst()
{
    m_playing = true;
    currentPlaylist()->playFirst();
    currentPlayingItemChanged();
}

void PlaylistCollection::playNextAlbum()
{
    m_playing = true;
    currentPlaylist()->playNextAlbum();
    currentPlayingItemChanged();
}

void PlaylistCollection::playPrevious()
{
    m_playing = true;
    currentPlaylist()->playPrevious();
    currentPlayingItemChanged();
}

void PlaylistCollection::playNext()
{
    m_playing = true;
    currentPlaylist()->playNext();
    currentPlayingItemChanged();
}

void PlaylistCollection::stop()
{
    m_playing = false;
    currentPlaylist()->stop();
    playlistItemsChanged();
}

bool PlaylistCollection::playing() const
{
    return m_playing;
}

QStringList PlaylistCollection::playlists() const
{
    QStringList l;

    //(or qFindChildren() if you need MSVC 6 compatibility)
    const auto childList = m_playlistStack->findChildren<Playlist *>("Playlist");
    for(Playlist *p : childList) {
        l.append(p->name());
    }

    return l;
}

void PlaylistCollection::createPlaylist(const QString &name)
{
    raise(new Playlist(this, name));
}

void PlaylistCollection::createDynamicPlaylist(const PlaylistList &playlists)
{
    if(m_dynamicPlaylist)
        m_dynamicPlaylist->setPlaylists(playlists);
    else {
        m_dynamicPlaylist =
            new DynamicPlaylist(playlists, this, i18n("Dynamic List"), "audio-midi", false, true);
        PlaylistCollection::setupPlaylist(m_dynamicPlaylist, QString());
    }

    PlaylistCollection::raise(m_dynamicPlaylist);
}

void PlaylistCollection::showMore(const QString &artist, const QString &album)
{

    PlaylistList playlists;
    PlaylistSearch::ComponentList components;

    if(currentPlaylist() != CollectionList::instance() &&
       currentPlaylist() != m_showMorePlaylist)
    {
        playlists.append(currentPlaylist());
    }

    playlists.append(CollectionList::instance());

    if(!artist.isNull())
    { // Just setting off the artist stuff in its own block.
        ColumnList columns;
        columns.append(PlaylistItem::ArtistColumn);
        PlaylistSearch::Component c(artist, false, columns,
                                    PlaylistSearch::Component::Exact);
        components.append(c);
    }

    if(!album.isNull()) {
        ColumnList columns;
        columns.append(PlaylistItem::AlbumColumn);
        PlaylistSearch::Component c(album, false, columns,
                                    PlaylistSearch::Component::Exact);
        components.append(c);
    }

    auto search = new PlaylistSearch(playlists, components, PlaylistSearch::MatchAll, object());

    if(m_showMorePlaylist)
        m_showMorePlaylist->setPlaylistSearch(search);
    else
        m_showMorePlaylist = new SearchPlaylist(this, *search, i18n("Now Playing"), false, true);

    // The call to raise() below will end up clearing m_belowShowMorePlaylist,
    // so cache the value we want it to have now.
    Playlist *belowShowMore = visiblePlaylist();

    PlaylistCollection::setupPlaylist(m_showMorePlaylist, QString());
    PlaylistCollection::raise(m_showMorePlaylist);

    m_belowShowMorePlaylist = belowShowMore;
}

void PlaylistCollection::removeTrack(const QString &playlist, const QStringList &files)
{
    Playlist *p = playlistByName(playlist);
    PlaylistItemList itemList;
    if(!p)
        return;

    QStringList::ConstIterator it;
    for(it = files.begin(); it != files.end(); ++it) {
        CollectionListItem *item = CollectionList::instance()->lookup(*it);

        if(item) {
            PlaylistItem *playlistItem = item->itemForPlaylist(p);
            if(playlistItem)
                itemList.append(playlistItem);
        }
    }

    p->clearItems(itemList);
}

QString PlaylistCollection::playlist() const
{
    return visiblePlaylist() ? visiblePlaylist()->name() : QString();
}

QString PlaylistCollection::playingPlaylist() const
{
    return currentPlaylist() && m_playing ? currentPlaylist()->name() : QString();
}

void PlaylistCollection::setPlaylist(const QString &playlist)
{
    Playlist *p = playlistByName(playlist);
    if(p)
        raise(p);
}

QStringList PlaylistCollection::playlistTracks(const QString &playlist) const
{
    Playlist *p = playlistByName(playlist);

    if(p)
        return p->files();
    return QStringList();
}

QString PlaylistCollection::trackProperty(const QString &file, const QString &property) const
{
    CollectionList *l = CollectionList::instance();
    CollectionListItem *item = l->lookup(file);

    return item ? item->file().property(property) : QString();
}

QPixmap PlaylistCollection::trackCover(const QString &file, const QString &size) const
{
    if(size.toLower() != "small" && size.toLower() != "large")
        return QPixmap();

    CollectionList *l = CollectionList::instance();
    CollectionListItem *item = l->lookup(file);

    if(!item)
        return QPixmap();

    if(size.toLower() == "small")
        return item->file().coverInfo()->pixmap(CoverInfo::Thumbnail);
    else
        return item->file().coverInfo()->pixmap(CoverInfo::FullSize);
}

void PlaylistCollection::open(const QStringList &l)
{
    QStringList files = l;

    if(files.isEmpty())
        files = MediaFiles::openDialog(JuK::JuKInstance());

    if(files.isEmpty())
        return;

    bool justPlaylists = true;

    for(QStringList::ConstIterator it = files.constBegin(); it != files.constEnd() && justPlaylists; ++it)
        justPlaylists = !MediaFiles::isPlaylistFile(*it);

    if(visiblePlaylist() == CollectionList::instance() || justPlaylists ||
       KMessageBox::questionYesNo(
           JuK::JuKInstance(),
           i18n("Do you want to add these items to the current list or to the collection list?"),
           QString(),
           KGuiItem(i18nc("current playlist", "Current")),
           KGuiItem(i18n("Collection"))) == KMessageBox::No)
    {
        CollectionList::instance()->addFiles(files);
    }
    else {
        visiblePlaylist()->addFiles(files);
    }

    playlistItemsChanged();
}

void PlaylistCollection::open(const QString &playlist, const QStringList &files)
{
    Playlist *p = playlistByName(playlist);

    if(p)
        p->addFiles(files);
}

void PlaylistCollection::addFolder()
{
    DirectoryList l(m_folderList, m_excludedFolderList, m_importPlaylists, JuK::JuKInstance());

    if(l.exec() == QDialog::Accepted) {
        m_dirLister.blockSignals(true);
        DirectoryList::Result result = l.dialogResult();

        const bool reload = m_importPlaylists != result.addPlaylists;

        m_importPlaylists = result.addPlaylists;
        m_excludedFolderList = canonicalizeFolderPaths(result.excludedDirs);

        foreach(const QString &dir, result.addedDirs) {
            m_dirLister.openUrl(QUrl::fromLocalFile(dir), KDirLister::Keep);
            m_folderList.append(dir);
        }

        foreach(const QString &dir, result.removedDirs) {
            m_dirLister.stop(QUrl::fromLocalFile(dir));
            m_folderList.removeAll(dir);
        }

        if(reload) {
            open(m_folderList);
        }
        else if(!result.addedDirs.isEmpty()) {
            open(result.addedDirs);
        }

        saveConfig();

        m_dirLister.blockSignals(false);
    }
}

void PlaylistCollection::rename()
{
    QString old = visiblePlaylist()->name();
    QString name = playlistNameDialog(i18n("Rename"), old, false);

    m_playlistNames.remove(old);

    if(name.isEmpty())
        return;

    visiblePlaylist()->setName(name);
}

void PlaylistCollection::duplicate()
{
    QString name = playlistNameDialog(i18nc("verb, copy the playlist", "Duplicate"),
                                      visiblePlaylist()->name());
    if(name.isEmpty())
        return;

    raise(new Playlist(this, visiblePlaylist()->items(), name));
}

void PlaylistCollection::save()
{
    visiblePlaylist()->save();
}

void PlaylistCollection::saveAs()
{
    visiblePlaylist()->saveAs();
}

void PlaylistCollection::reload()
{
    if(visiblePlaylist() == CollectionList::instance())
        CollectionList::instance()->addFiles(m_folderList);
    else
        visiblePlaylist()->slotReload();

}

void PlaylistCollection::editSearch()
{
    SearchPlaylist *p = dynamic_cast<SearchPlaylist *>(visiblePlaylist());
    if(!p)
        return;

    auto searchDialog = new AdvancedSearchDialog(
            p->name(), *(p->playlistSearch()), JuK::JuKInstance());
    QObject::connect(searchDialog, &QDialog::finished, [searchDialog, p](int result)
            {
                if (result) {
                    p->setPlaylistSearch(searchDialog->resultSearch());
                    p->setName(searchDialog->resultPlaylistName());
                }
                searchDialog->deleteLater();
            });
    searchDialog->exec();
}

void PlaylistCollection::removeItems()
{
    visiblePlaylist()->slotRemoveSelectedItems();
}

void PlaylistCollection::refreshItems()
{
    visiblePlaylist()->slotRefresh();
}

void PlaylistCollection::renameItems()
{
    visiblePlaylist()->slotRenameFile();
}

void PlaylistCollection::addCovers(bool fromFile)
{
    visiblePlaylist()->slotAddCover(fromFile);
    playlistItemsChanged();
}

void PlaylistCollection::removeCovers()
{
    visiblePlaylist()->slotRemoveCover();
    playlistItemsChanged();
}

void PlaylistCollection::viewCovers()
{
    visiblePlaylist()->slotViewCover();
}

void PlaylistCollection::showCoverManager()
{
    visiblePlaylist()->slotShowCoverManager();
}

PlaylistItemList PlaylistCollection::selectedItems()
{
    return visiblePlaylist()->selectedItems();
}

void PlaylistCollection::scanFolders()
{
    // If no music folder was configured, open music folder dialog
    if(m_folderList.count() == 0)
        addFolder();

    CollectionList::instance()->addFiles(m_folderList);

    enableDirWatch(true);
}

void PlaylistCollection::createPlaylist()
{
    QString name = playlistNameDialog();
    if(!name.isEmpty())
        raise(new Playlist(this, name));
}

void PlaylistCollection::createSearchPlaylist()
{
    QString name = uniquePlaylistName(i18n("Search Playlist"));

    auto searchDialog = new AdvancedSearchDialog(
            name, *(new PlaylistSearch(JuK::JuKInstance())), JuK::JuKInstance());
    QObject::connect(searchDialog, &QDialog::finished, [searchDialog, this](int result)
            {
                if (result) {
                    raise(new SearchPlaylist(
                                this,
                                *searchDialog->resultSearch(),
                                searchDialog->resultPlaylistName()));
                }
                searchDialog->deleteLater();
            });
    searchDialog->exec();
}

void PlaylistCollection::createFolderPlaylist()
{
    QString folder = QFileDialog::getExistingDirectory();

    if(folder.isEmpty())
        return;

    QString name = uniquePlaylistName(folder.mid(folder.lastIndexOf('/') + 1));
    name = playlistNameDialog(i18n("Create Folder Playlist"), name);

    if(!name.isEmpty())
        raise(new FolderPlaylist(this, folder, name));
}

void PlaylistCollection::guessTagFromFile()
{
    visiblePlaylist()->slotGuessTagInfo(TagGuesser::FileName);
}

void PlaylistCollection::guessTagFromInternet()
{
    visiblePlaylist()->slotGuessTagInfo(TagGuesser::MusicBrainz);
}

void PlaylistCollection::setSearchEnabled(bool enable)
{
    if(enable == m_searchEnabled)
        return;

    m_searchEnabled = enable;

    visiblePlaylist()->setSearchEnabled(enable);
}

HistoryPlaylist *PlaylistCollection::historyPlaylist() const
{
    return m_historyPlaylist;
}

void PlaylistCollection::setHistoryPlaylistEnabled(bool enable)
{
    if((enable && m_historyPlaylist) || (!enable && !m_historyPlaylist))
        return;

    if(enable) {
        action<KToggleAction>("showHistory")->setChecked(true);
        m_historyPlaylist = new HistoryPlaylist(this);
        m_historyPlaylist->setName(i18n("History"));
        setupPlaylist(m_historyPlaylist, "view-history");

        QObject::connect(m_playerManager, SIGNAL(signalItemChanged(FileHandle)),
                historyPlaylist(), SLOT(appendProposedItem(FileHandle)));
    }
    else {
        delete m_historyPlaylist;
        m_historyPlaylist = 0;
    }
}

UpcomingPlaylist *PlaylistCollection::upcomingPlaylist() const
{
    return m_upcomingPlaylist;
}

void PlaylistCollection::setUpcomingPlaylistEnabled(bool enable)
{
    if((enable && m_upcomingPlaylist) || (!enable && !m_upcomingPlaylist))
        return;

    if(enable) {
        action<KToggleAction>("showUpcoming")->setChecked(true);
        if(!m_upcomingPlaylist)
            m_upcomingPlaylist = new UpcomingPlaylist(this);

        setupPlaylist(m_upcomingPlaylist, "go-jump-today");
    }
    else {
        action<KToggleAction>("showUpcoming")->setChecked(false);
        bool raiseCollection = visiblePlaylist() == m_upcomingPlaylist;

        if(raiseCollection) {
            raise(CollectionList::instance());
        }

        m_upcomingPlaylist->deleteLater();
        m_upcomingPlaylist = 0;
    }
}

QObject *PlaylistCollection::object() const
{
    return m_actionHandler;
}

Playlist *PlaylistCollection::currentPlaylist() const
{
    if(m_belowDistraction)
        return m_belowDistraction;

    if(m_upcomingPlaylist && m_upcomingPlaylist->active())
        return m_upcomingPlaylist;

    if(Playlist::playingItem())
        return Playlist::playingItem()->playlist();
    else
        return visiblePlaylist();
}

Playlist *PlaylistCollection::visiblePlaylist() const
{
    return qobject_cast<Playlist *>(m_playlistStack->currentWidget());
}

void PlaylistCollection::raise(Playlist *playlist)
{
    if(m_showMorePlaylist && currentPlaylist() == m_showMorePlaylist)
        m_showMorePlaylist->lower(playlist);
    if(m_dynamicPlaylist && currentPlaylist() == m_dynamicPlaylist)
        m_dynamicPlaylist->lower(playlist);

    TrackSequenceManager::instance()->setCurrentPlaylist(playlist);
    playlist->applySharedSettings();
    playlist->setSearchEnabled(m_searchEnabled);
    m_playlistStack->setCurrentWidget(playlist);
    clearShowMore(false);
    playlistItemsChanged();
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

QStackedWidget *PlaylistCollection::playlistStack() const
{
    return m_playlistStack;
}

void PlaylistCollection::setupPlaylist(Playlist *playlist, const QString &)
{
    if(!playlist->fileName().isEmpty())
        m_playlistFiles.insert(playlist->fileName());

    if(!playlist->name().isEmpty())
        m_playlistNames.insert(playlist->name());

    m_playlistStack->addWidget(playlist);
    QObject::connect(playlist, SIGNAL(itemSelectionChanged()),
                     object(), SIGNAL(signalSelectedItemsChanged()));
}

bool PlaylistCollection::importPlaylists() const
{
    return m_importPlaylists;
}

bool PlaylistCollection::containsPlaylistFile(const QString &file) const
{
    return m_playlistFiles.contains(file);
}

bool PlaylistCollection::showMoreActive() const
{
    return visiblePlaylist() == m_showMorePlaylist;
}

void PlaylistCollection::clearShowMore(bool raisePlaylist)
{
    if(!m_showMorePlaylist)
        return;

    if(raisePlaylist) {
        if(m_belowShowMorePlaylist)
            raise(m_belowShowMorePlaylist);
        else
            raise(CollectionList::instance());
    }

    m_belowShowMorePlaylist = 0;
}

void PlaylistCollection::enableDirWatch(bool enable)
{
    auto collection = CollectionList::instance();

    m_dirLister.disconnect(object());
    if(enable) {
        QObject::connect(&m_dirLister, &KDirLister::newItems,
                object(), [this](const KFileItemList &items) {
                    this->newItems(items);
                });
        QObject::connect(&m_dirLister, &KDirLister::refreshItems,
                collection, &CollectionList::slotRefreshItems);
        QObject::connect(&m_dirLister, &KDirLister::itemsDeleted,
                collection, &CollectionList::slotDeleteItems);
    }
}

QString PlaylistCollection::playlistNameDialog(const QString &caption,
                                               const QString &suggest,
                                               bool forceUnique) const
{
    bool ok;

    QString name = QInputDialog::getText(
        m_playlistStack,
        caption,
        i18n("Please enter a name for this playlist:"),
        QLineEdit::Normal,
        forceUnique ? uniquePlaylistName(suggest) : suggest,
        &ok);

    return ok ? uniquePlaylistName(name) : QString();
}


QString PlaylistCollection::uniquePlaylistName(const QString &suggest) const
{
    if(suggest.isEmpty())
        return uniquePlaylistName();

    if(!m_playlistNames.contains(suggest))
        return suggest;

    QString base = suggest;
    base.remove(QRegExp("\\s\\([0-9]+\\)$"));

    int count = 1;
    QString s = QString("%1 (%2)").arg(base).arg(count);

    while(m_playlistNames.contains(s)) {
        count++;
        s = QString("%1 (%2)").arg(base).arg(count);
    }

    return s;
}

void PlaylistCollection::addNameToDict(const QString &name)
{
    m_playlistNames.insert(name);
}

void PlaylistCollection::addFileToDict(const QString &file)
{
    m_playlistFiles.insert(file);
}

void PlaylistCollection::removeNameFromDict(const QString &name)
{
    m_playlistNames.remove(name);
}

void PlaylistCollection::removeFileFromDict(const QString &file)
{
    m_playlistFiles.remove(file);
}

void PlaylistCollection::dirChanged(const QString &path)
{
    QString canonicalPath = QDir(path).canonicalPath();
    if(canonicalPath.isEmpty())
        return;

    foreach(const QString &excludedFolder, m_excludedFolderList) {
        if(canonicalPath.startsWith(excludedFolder))
            return;
    }

    CollectionList::instance()->addFiles(QStringList(canonicalPath));
}

Playlist *PlaylistCollection::playlistByName(const QString &name) const
{
    for(int i = 0; i < m_playlistStack->count(); ++i) {
        Playlist *p = qobject_cast<Playlist *>(m_playlistStack->widget(i));
        if(p && p->name() == name)
            return p;
    }

    return 0;
}

void PlaylistCollection::newItems(const KFileItemList &list) const
{
    // Make fast-path for the normal case
    if(m_excludedFolderList.isEmpty()) {
        CollectionList::instance()->slotNewItems(list);
        return;
    }

    // Slow case: Directories to exclude from consideration

    KFileItemList filteredList(list);

    foreach(const QString &excludedFolder, m_excludedFolderList) {
        QMutableListIterator<KFileItem> filteredListIterator(filteredList);

        while(filteredListIterator.hasNext()) {
            const KFileItem fileItem = filteredListIterator.next();

            if(fileItem.url().path().startsWith(excludedFolder))
                filteredListIterator.remove();
        }
    }

    CollectionList::instance()->slotNewItems(filteredList);
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistCollection::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), "Playlists");

    m_importPlaylists    = config.readEntry("ImportPlaylists", true);
    m_folderList         = config.readEntry("DirectoryList", QStringList());
    m_excludedFolderList = canonicalizeFolderPaths(
            config.readEntry("ExcludeDirectoryList", QStringList()));

    for(const auto &folder : qAsConst(m_folderList)) {
        m_dirLister.openUrl(QUrl::fromUserInput(folder), KDirLister::Keep);
    }
}

void PlaylistCollection::saveConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), "Playlists");
    config.writeEntry("ImportPlaylists", m_importPlaylists);
    config.writeEntry("showUpcoming", action("showUpcoming")->isChecked());
    config.writePathEntry("DirectoryList", m_folderList);
    config.writePathEntry("ExcludeDirectoryList", m_excludedFolderList);

    config.sync();
}

////////////////////////////////////////////////////////////////////////////////
// ActionHandler implementation
////////////////////////////////////////////////////////////////////////////////

PlaylistCollection::ActionHandler::ActionHandler(PlaylistCollection *collection) :
    QObject(nullptr),
    m_collection(collection)
{
    setObjectName( QLatin1String("ActionHandler" ));

    KActionMenu *menu;

    // "New" menu

    menu = new KActionMenu(QIcon::fromTheme(QStringLiteral("document-new")), i18nc("new playlist", "&New"), actions());
    actions()->addAction("file_new", menu);

    menu->addAction(createAction(i18n("&Empty Playlist..."), SLOT(slotCreatePlaylist()),
                              "newPlaylist", "window-new", QKeySequence(Qt::CTRL + Qt::Key_N)));
    menu->addAction(createAction(i18n("&Search Playlist..."), SLOT(slotCreateSearchPlaylist()),
                              "newSearchPlaylist", "edit-find", QKeySequence(Qt::CTRL + Qt::Key_F)));
    menu->addAction(createAction(i18n("Playlist From &Folder..."), SLOT(slotCreateFolderPlaylist()),
                              "newDirectoryPlaylist", "document-open", QKeySequence(Qt::CTRL + Qt::Key_D)));

    // Guess tag info menu

#if HAVE_TUNEPIMP
    menu = new KActionMenu(i18n("&Guess Tag Information"), actions());
    actions()->addAction("guessTag", menu);

    menu->setIcon(QIcon::fromTheme("wizard"));

    menu->addAction(createAction(i18n("From &File Name"), SLOT(slotGuessTagFromFile()),
                              "guessTagFile", "document-import", QKeySequence(Qt::CTRL + Qt::Key_G)));
    menu->addAction(createAction(i18n("From &Internet"), SLOT(slotGuessTagFromInternet()),
                              "guessTagInternet", "network-server", QKeySequence(Qt::CTRL + Qt::Key_I)));
#else
    createAction(i18n("Guess Tag Information From &File Name"), SLOT(slotGuessTagFromFile()),
                 "guessTag", "document-import", QKeySequence(Qt::CTRL + Qt::Key_G));
#endif


    createAction(i18n("Play First Track"),SLOT(slotPlayFirst()),     "playFirst");
    createAction(i18n("Play Next Album"), SLOT(slotPlayNextAlbum()), "forwardAlbum", "go-down-search");

    KStandardAction::open(this, SLOT(slotOpen()), actions());
    KStandardAction::save(this, SLOT(slotSave()), actions());
    KStandardAction::saveAs(this, SLOT(slotSaveAs()), actions());

    createAction(i18n("Manage &Folders..."),  SLOT(slotManageFolders()),    "openDirectory", "folder-new");
    createAction(i18n("&Rename..."),      SLOT(slotRename()),       "renamePlaylist", "edit-rename");
    createAction(i18nc("verb, copy the playlist", "D&uplicate..."),
                 SLOT(slotDuplicate()),    "duplicatePlaylist", "edit-copy");
    createAction(i18n("R&emove"),         SLOT(slotRemove()),       "deleteItemPlaylist", "user-trash");
    createAction(i18n("Reload"),          SLOT(slotReload()),       "reloadPlaylist", "view-refresh");
    createAction(i18n("Edit Search..."),  SLOT(slotEditSearch()),   "editSearch");

    createAction(i18n("&Delete"),         SLOT(slotRemoveItems()),  "removeItem", "edit-delete");
    createAction(i18n("Refresh"),         SLOT(slotRefreshItems()), "refresh", "view-refresh");
    createAction(i18n("&Rename File"),    SLOT(slotRenameItems()),  "renameFile", "document-save-as", QKeySequence(Qt::CTRL + Qt::Key_R));

    menu = new KActionMenu(i18n("Cover Manager"), actions());
    actions()->addAction("coverManager", menu);
    menu->setIcon(QIcon::fromTheme("image-x-generic"));
    menu->addAction(createAction(i18n("&View Cover"),
        SLOT(slotViewCovers()), "viewCover", "document-preview"));
    menu->addAction(createAction(i18n("Get Cover From &File..."),
        SLOT(slotAddLocalCover()), "addCover", "document-import", QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_F)));
    menu->addAction(createAction(i18n("Get Cover From &Internet..."),
        SLOT(slotAddInternetCover()), "webImageCover", "network-server", QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_G)));
    menu->addAction(createAction(i18n("&Delete Cover"),
        SLOT(slotRemoveCovers()), "removeCover", "edit-delete"));
    menu->addAction(createAction(i18n("Show Cover &Manager"),
        SLOT(slotShowCoverManager()), "showCoverManager"));

    KToggleAction *upcomingAction =
        new KToggleAction(QIcon::fromTheme(QStringLiteral("go-jump-today")), i18n("Show &Play Queue"), actions());
    actions()->addAction("showUpcoming", upcomingAction);

    connect(upcomingAction, SIGNAL(triggered(bool)),
            this, SLOT(slotSetUpcomingPlaylistEnabled(bool)));

    connect(m_collection->m_playerManager, &PlayerManager::signalStop,
            this, [this]() { m_collection->stop(); });
}

QAction *PlaylistCollection::ActionHandler::createAction(const QString &text,
                                                         const char *slot,
                                                         const char *name,
                                                         const QString &icon,
                                                         const QKeySequence &shortcut)
{
    auto actionCollection = actions();
    QAction *action = new QAction(text, actions());

    if(!icon.isEmpty()) {
        action->setIcon(QIcon::fromTheme(icon));
    }
    connect(action, SIGNAL(triggered(bool)), slot);

    actionCollection->addAction(name, action);

    if (!shortcut.isEmpty()) {
        actionCollection->setDefaultShortcut(action, shortcut);
    }
    return action;
}

// vim: set et sw=4 tw=0 sta:
