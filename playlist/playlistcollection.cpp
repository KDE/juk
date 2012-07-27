/***************************************************************************
    copyright            : (C) 2004 by Scott Wheeler
    email                : wheeler@kde.org

    copyright            : (C) 2009 by Michael Pyne
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

#include "playlistcollection.h"

#include <kurl.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <kactionmenu.h>
#include <kconfiggroup.h>

#include <config-juk.h>

#include <QObject>
#include <QPixmap>
#include <QStackedWidget>

#include <sys/types.h>
#include <dirent.h>

#include "collectionlist.h"
#include "actioncollection.h"
#include "advancedsearchdialog.h"
#include "coverinfo.h"
#include "playlist/playlists/searchplaylist.h"
#include "playlist/playlists/folderplaylist.h"
#include "playlist/playlists/historyplaylist.h"
#include "playlist/playlists/upcomingplaylist.h"
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

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistCollection::PlaylistCollection(PlayerManager *player, QListView *playlistStack) :
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
    currentChanged();
}

void PlaylistCollection::playNextAlbum()
{
    m_playing = true;
    currentPlaylist()->playNextAlbum();
    currentChanged();
}

void PlaylistCollection::playPrevious()
{
    m_playing = true;
    currentPlaylist()->playPrevious();
    currentChanged();
}

void PlaylistCollection::playNext()
{
    m_playing = true;
    currentPlaylist()->playNext();
    currentChanged();
}

void PlaylistCollection::stop()
{
    m_playing = false;
    currentPlaylist()->stop();
    dataChanged();
}

bool PlaylistCollection::playing() const
{
    return m_playing;
}

QStringList PlaylistCollection::playlists() const
{
    QStringList l;

    foreach(const Playlist *p, m_playlists) {
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

    PlaylistSearch search(playlists, components, PlaylistSearch::MatchAll);

    if(m_showMorePlaylist)
        m_showMorePlaylist->setPlaylistSearch(search);
    else
        m_showMorePlaylist = new SearchPlaylist(this, search, i18n("Now Playing"), false, true);

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
    else
        visiblePlaylist()->addFiles(files);

    dataChanged();
}

void PlaylistCollection::open(const QString &playlist, const QStringList &files)
{
    Playlist *p = playlistByName(playlist);

    if(p)
        p->addFiles(files);
}

void PlaylistCollection::addFolder()
{
    DirectoryList l(m_folderList, m_importPlaylists, JuK::JuKInstance());
    DirectoryList::Result result = l.exec();

    if(result.status == QDialog::Accepted) {

        m_dirLister.blockSignals(true);

        const bool reload = m_importPlaylists != result.addPlaylists;
        m_importPlaylists = result.addPlaylists;

        foreach(const QString &dir, result.addedDirs) {
            m_dirLister.openUrl(KUrl::fromPath(dir), KDirLister::Keep);
            m_folderList.append(dir);
        }

        foreach(const QString &dir, result.removedDirs) {
            m_dirLister.stop(KUrl::fromPath(dir));
            m_folderList.removeAll(dir);
        }

        if(reload)
            open(m_folderList);
        else if(!result.addedDirs.isEmpty())
            open(result.addedDirs);

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

    AdvancedSearchDialog::Result r =
        AdvancedSearchDialog(p->name(), p->playlistSearch(), JuK::JuKInstance()).exec();

    if(r.result == AdvancedSearchDialog::Accepted) {
        p->setPlaylistSearch(r.search);
        p->setName(r.playlistName);
    }
}

void PlaylistCollection::removeItems()
{
    // ### TODO: View
//     visiblePlaylist()->slotRemoveSelectedItems();
}

void PlaylistCollection::refreshItems()
{
    // ### TODO: View
//     visiblePlaylist()->slotRefresh();
}

void PlaylistCollection::renameItems()
{
    // ### TODO: View
//     visiblePlaylist()->slotRenameFile();
}

void PlaylistCollection::addCovers(bool fromFile)
{
    // ### TODO: View
//     visiblePlaylist()->slotAddCover(fromFile);
    dataChanged();
}

void PlaylistCollection::removeCovers()
{
    // ### TODO: View
//     visiblePlaylist()->slotRemoveCover();
    dataChanged();
}

void PlaylistCollection::viewCovers()
{
    // ### TODO: View
//     visiblePlaylist()->slotViewCover();
}

void PlaylistCollection::showCoverManager()
{
    // ### TODO: View
//     visiblePlaylist()->slotShowCoverManager();
}

// ### TODO: View
// PlaylistItemList PlaylistCollection::selectedItems()
// {
//     return visiblePlaylist()->selectedItems();
// }

void PlaylistCollection::scanFolders()
{
    CollectionList::instance()->addFiles(m_folderList);

    if(CollectionList::instance()->count() == 0)
        addFolder();
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

    AdvancedSearchDialog::Result r =
        AdvancedSearchDialog(name, PlaylistSearch(), JuK::JuKInstance()).exec();

    if(r.result == AdvancedSearchDialog::Accepted)
        raise(new SearchPlaylist(this, r.search, r.playlistName));
}

void PlaylistCollection::createFolderPlaylist()
{
    QString folder = KFileDialog::getExistingDirectory();

    if(folder.isEmpty())
        return;

    QString name = uniquePlaylistName(folder.mid(folder.lastIndexOf('/') + 1));
    name = playlistNameDialog(i18n("Create Folder Playlist"), name);

    if(!name.isEmpty())
        raise(new FolderPlaylist(this, folder, name));
}

void PlaylistCollection::guessTagFromFile()
{
    // ### TODO: View
//     visiblePlaylist()->slotGuessTagInfo(TagGuesser::FileName);
}

void PlaylistCollection::guessTagFromInternet()
{
    // ### TODO: View
//     visiblePlaylist()->slotGuessTagInfo(TagGuesser::MusicBrainz);
}

void PlaylistCollection::setSearchEnabled(bool enable)
{
    if(enable == m_searchEnabled)
        return;

    m_searchEnabled = enable;

    // ### TODO: View
//     visiblePlaylist()->setSearchEnabled(enable);
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
        delete m_upcomingPlaylist;
        m_upcomingPlaylist = 0;

        if(raiseCollection) {
            kapp->processEvents(); // Seems to stop a crash, weird.
            raise(CollectionList::instance());
        }
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
    return qobject_cast<Playlist *>(m_playlistStack->model());
}

void PlaylistCollection::raise(Playlist *playlist)
{
    // ### wat
//     if(m_showMorePlaylist && currentPlaylist() == m_showMorePlaylist)
//         m_showMorePlaylist->lower(playlist);
//     if(m_dynamicPlaylist && currentPlaylist() == m_dynamicPlaylist)
//         m_dynamicPlaylist->lower(playlist);

    TrackSequenceManager::instance()->setCurrentPlaylist(playlist);
    playlist->applySharedSettings();
    // ### TODO: View
//     playlist->setSearchEnabled(m_searchEnabled);
    m_playlistStack->setModel(playlist);
    clearShowMore(false);
    dataChanged();
}

void PlaylistCollection::raiseDistraction()
{
    if(m_belowDistraction)
        return;

    m_belowDistraction = currentPlaylist();

    // ### TODO: View
//     if(!m_distraction) {
//         m_distraction = new QWidget(m_playlistStack);
//         m_playlistStack->addWidget(m_distraction);
//     }

//     m_playlistStack->setCurrentWidget(m_distraction);
}

void PlaylistCollection::lowerDistraction()
{
    if(!m_distraction)
        return;

    // ### TODO: View
    //if(m_belowDistraction)
    //    m_playlistStack->setCurrentWidget(m_belowDistraction);

    m_belowDistraction = 0;
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistCollection::setupPlaylist(Playlist *playlist, const QString &)
{
    if(!playlist->fileName().isEmpty())
        m_playlistFiles.insert(playlist->fileName());

    if(!playlist->name().isEmpty())
        m_playlistNames.insert(playlist->name());

    // ### TODO: View
    m_playlists.append(playlist);
//     m_playlistStack->addWidget(playlist);
    QObject::connect(playlist, SIGNAL(selectionChanged()),
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
    QObject *collection = CollectionList::instance();

    m_dirLister.disconnect(object());
    if(enable) {
        QObject::connect(&m_dirLister, SIGNAL(newItems(KFileItemList)),
                object(), SLOT(slotNewItems(KFileItemList)));
        QObject::connect(&m_dirLister, SIGNAL(refreshItems(QList<QPair<KFileItem,KFileItem> >)),
                collection, SLOT(slotRefreshItems(QList<QPair<KFileItem,KFileItem> >)));
        QObject::connect(&m_dirLister, SIGNAL(deleteItem(KFileItem)),
                collection, SLOT(slotDeleteItem(KFileItem)));
    }
}

QString PlaylistCollection::playlistNameDialog(const QString &caption,
                                               const QString &suggest,
                                               bool forceUnique) const
{
    bool ok;

    QString name = KInputDialog::getText(
        caption,
        i18n("Please enter a name for this playlist:"),
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
    CollectionList::instance()->addFiles(QStringList(path));
}

Playlist *PlaylistCollection::playlistByName(const QString &name) const
{
    foreach(Playlist *playlist, m_playlists)
        if (playlist->name() == name) return playlist;

    return 0;
}

void PlaylistCollection::newItems(const KFileItemList &list) const
{
    CollectionList::instance()->slotNewItems(list);
}

////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistCollection::readConfig()
{
    KConfigGroup config(KGlobal::config(), "Playlists");

    m_importPlaylists  = config.readEntry("ImportPlaylists", true);
    m_folderList       = config.readEntry("DirectoryList", QStringList());

    foreach(const QString &folder, m_folderList)
        m_dirLister.openUrl(folder, KDirLister::Keep);
}

void PlaylistCollection::saveConfig()
{
    KConfigGroup config(KGlobal::config(), "Playlists");
    config.writeEntry("ImportPlaylists", m_importPlaylists);
    config.writeEntry("showUpcoming", action("showUpcoming")->isChecked());
    config.writePathEntry("DirectoryList", m_folderList);

    config.sync();
}

////////////////////////////////////////////////////////////////////////////////
// ActionHanlder implementation
////////////////////////////////////////////////////////////////////////////////

PlaylistCollection::ActionHandler::ActionHandler(PlaylistCollection *collection) :
    QObject(0),
    m_collection(collection)
{
    setObjectName( QLatin1String("ActionHandler" ));

    KActionMenu *menu;

    // "New" menu

    menu = new KActionMenu(KIcon("document-new"), i18nc("new playlist", "&New"), actions());
    actions()->addAction("file_new", menu);

    menu->addAction(createAction(i18n("&Empty Playlist..."), SLOT(slotCreatePlaylist()),
                              "newPlaylist", "window-new", KShortcut(Qt::CTRL + Qt::Key_N)));
    menu->addAction(createAction(i18n("&Search Playlist..."), SLOT(slotCreateSearchPlaylist()),
                              "newSearchPlaylist", "edit-find", KShortcut(Qt::CTRL + Qt::Key_F)));
    menu->addAction(createAction(i18n("Playlist From &Folder..."), SLOT(slotCreateFolderPlaylist()),
                              "newDirectoryPlaylist", "document-open", KShortcut(Qt::CTRL + Qt::Key_D)));

    // Guess tag info menu

#ifdef HAVE_TUNEPIMP
    menu = new KActionMenu(i18n("&Guess Tag Information"), actions());
    actions()->addAction("guessTag", menu);

    /* menu->setIcon(SmallIcon("wizard")); */

    menu->addAction(createAction(i18n("From &File Name"), SLOT(slotGuessTagFromFile()),
                              "guessTagFile", "document-import", KShortcut(Qt::CTRL + Qt::Key_G)));
    menu->addAction(createAction(i18n("From &Internet"), SLOT(slotGuessTagFromInternet()),
                              "guessTagInternet", "network-server", KShortcut(Qt::CTRL + Qt::Key_I)));
#else
    createAction(i18n("Guess Tag Information From &File Name"), SLOT(slotGuessTagFromFile()),
                 "guessTag", "document-import", KShortcut(Qt::CTRL + Qt::Key_F));
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
    createAction(i18n("&Rename File"),    SLOT(slotRenameItems()),  "renameFile", "document-save-as", KShortcut(Qt::CTRL + Qt::Key_R));

    menu = new KActionMenu(i18n("Cover Manager"), actions());
    actions()->addAction("coverManager", menu);
    /* menu->setIcon(SmallIcon("image-x-generic")); */
    menu->addAction(createAction(i18n("&View Cover"),
        SLOT(slotViewCovers()), "viewCover", "document-preview"));
    menu->addAction(createAction(i18n("Get Cover From &File..."),
        SLOT(slotAddLocalCover()), "addCover", "document-import", KShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_F)));
    menu->addAction(createAction(i18n("Get Cover From &Internet..."),
        SLOT(slotAddInternetCover()), "webImageCover", "network-server", KShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_G)));
    menu->addAction(createAction(i18n("&Delete Cover"),
        SLOT(slotRemoveCovers()), "removeCover", "edit-delete"));
    menu->addAction(createAction(i18n("Show Cover &Manager"),
        SLOT(slotShowCoverManager()), "showCoverManager"));

    KToggleAction *upcomingAction =
        new KToggleAction(KIcon("go-jump-today"), i18n("Show &Play Queue"), actions());
    actions()->addAction("showUpcoming", upcomingAction);

    connect(upcomingAction, SIGNAL(triggered(bool)),
            this, SLOT(slotSetUpcomingPlaylistEnabled(bool)));
}

KAction *PlaylistCollection::ActionHandler::createAction(const QString &text,
                                                         const char *slot,
                                                         const char *name,
                                                         const QString &icon,
                                                         const KShortcut &shortcut)
{
    KAction *action;
    if(icon.isNull())
        action = new KAction(text, actions());
    else
        action = new KAction(KIcon(icon), text, actions());
    actions()->addAction(name, action);
    connect( action, SIGNAL(triggered(bool)), slot);
    action->setShortcut(shortcut);
    return action;
}

#include "playlistcollection.moc"

// vim: set et sw=4 tw=0 sta:
