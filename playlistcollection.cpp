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

#include <config.h>
#include <qobjectlist.h>

#include <sys/types.h>
#include <dirent.h>

#include "collectionlist.h"
#include "playlistcollection.h"
#include "actioncollection.h"
#include "advancedsearchdialog.h"
#include "searchplaylist.h"
#include "folderplaylist.h"
#include "historyplaylist.h"
#include "upcomingplaylist.h"
#include "directorylist.h"
#include "mediafiles.h"
#include "playermanager.h"

#include <kiconloader.h>
#include <kactionclasses.h>
#include <kapplication.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kfiledialog.h>

#include <qwidgetstack.h>

#define widget (kapp->mainWidget())

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistCollection::PlaylistCollection(QWidgetStack *playlistStack) :
    m_playlistStack(playlistStack),
    m_historyPlaylist(0),
    m_upcomingPlaylist(0),
    m_importPlaylists(true),
    m_searchEnabled(true),
    m_playing(false)
{
    m_actionHandler = new ActionHandler(this);
    PlayerManager::instance()->setPlaylistInterface(this);
    readConfig();
}

PlaylistCollection::~PlaylistCollection()
{
    saveConfig();
    delete m_actionHandler;
    PlayerManager::instance()->setPlaylistInterface(0);
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
    currentChanged();
}

bool PlaylistCollection::playing() const
{
    return m_playing;
}

QStringList PlaylistCollection::playlists() const
{
    QStringList l;

    QObjectList *childList = m_playlistStack->queryList("Playlist");
    QObject *obj;
    for(obj = childList->first(); obj; obj = childList->next()) {
        Playlist *p = static_cast<Playlist*>(obj);
        l.append(p->name());
    }

    delete childList;
    return l;
}

void PlaylistCollection::createPlaylist(const QString &name)
{
    raise(new Playlist(this, name));
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
    if(currentPlaylist())
        return currentPlaylist()->name();
    return QString::null;
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

    return item ? item->file().property(property) : QString::null;
}

void PlaylistCollection::open(const QStringList &l)
{
    QStringList files = l;

    if(files.isEmpty())
        files = MediaFiles::openDialog(widget);

    if(files.isEmpty())
        return;

    if(currentPlaylist() == CollectionList::instance() ||
       KMessageBox::questionYesNo(
           widget,
           i18n("Do you want to add these items to the current list or to the collection list?"),
           QString::null,
           KGuiItem(i18n("Current")),
           KGuiItem(i18n("Collection"))) == KMessageBox::No)
    {
        CollectionList::instance()->addFiles(files, m_importPlaylists);
    }
    else
        currentPlaylist()->addFiles(files, m_importPlaylists);

    dataChanged();
}

void PlaylistCollection::open(const QString &playlist, const QStringList &files)
{
    Playlist *p = playlistByName(playlist);

    if(p)
        p->addFiles(files, m_importPlaylists);
}

void PlaylistCollection::addFolder()
{
    DirectoryList l(m_folderList, m_importPlaylists, widget, "directoryList");
    DirectoryList::Result result = l.exec();

    if(result.status == QDialog::Accepted) {

        for(QStringList::Iterator it = result.addedDirs.begin();
            it != result.addedDirs.end(); it++)
        {
            m_dirWatch.addDir(*it, false, true);
	    m_folderList.append(*it);
        }

        for(QStringList::Iterator it = result.removedDirs.begin();
            it !=  result.removedDirs.end(); it++)
        {
            m_dirWatch.removeDir(*it);
            m_folderList.remove(*it);
        }

        m_importPlaylists = result.addPlaylists;

        if(result.addPlaylists && !m_importPlaylists)
            open(m_folderList);
        else if(!result.addedDirs.isEmpty())
            open(result.addedDirs);
    }
}

void PlaylistCollection::rename()
{
    QString old = currentPlaylist()->name();
    QString name = playlistNameDialog(i18n("Rename"), old, false);

    m_playlistNames.remove(old);

    if(name.isNull())
        return;

    currentPlaylist()->setName(name);
}

void PlaylistCollection::duplicate()
{
    QString name = playlistNameDialog(i18n("Duplicate"), currentPlaylist()->name());
    if(name.isNull())
        return;
    raise(new Playlist(this, currentPlaylist()->items(), name));
}

void PlaylistCollection::save()
{
    currentPlaylist()->save();
}

void PlaylistCollection::saveAs()
{
    currentPlaylist()->saveAs();
}

void PlaylistCollection::remove()
{
    // implemented in subclass
}

void PlaylistCollection::reload()
{
    if(currentPlaylist() == CollectionList::instance())
        CollectionList::instance()->addFiles(m_folderList, m_importPlaylists);
    else
        currentPlaylist()->slotReload();

}

void PlaylistCollection::editSearch()
{
    SearchPlaylist *p = dynamic_cast<SearchPlaylist *>(currentPlaylist());

    if(!p)
        return;

    AdvancedSearchDialog::Result r =
        AdvancedSearchDialog(p->name(), p->playlistSearch(), widget).exec();

    if(r.result == AdvancedSearchDialog::Accepted) {
        p->setPlaylistSearch(r.search);
        p->setName(r.playlistName);
    }
}

void PlaylistCollection::removeItems()
{
    currentPlaylist()->slotRemoveSelectedItems();
}

void PlaylistCollection::refreshItems()
{
    currentPlaylist()->slotRefresh();
}

void PlaylistCollection::renameItems()
{
    currentPlaylist()->slotRenameFile();
}

PlaylistItemList PlaylistCollection::selectedItems()
{
    return currentPlaylist()->selectedItems();
}

void PlaylistCollection::scanFolders()
{
    CollectionList::instance()->addFiles(m_folderList, m_importPlaylists);

    if(CollectionList::instance()->count() == 0)
        addFolder();
}

void PlaylistCollection::createPlaylist()
{
    QString name = playlistNameDialog();
    if(!name.isNull())
        raise(new Playlist(this, name));
}

void PlaylistCollection::createSearchPlaylist()
{
    QString name = uniquePlaylistName(i18n("Search Playlist"));

    AdvancedSearchDialog::Result r =
        AdvancedSearchDialog(name, PlaylistSearch(), widget).exec();

    if(r.result == AdvancedSearchDialog::Accepted)
        raise(new SearchPlaylist(this, r.search, r.playlistName));
}

void PlaylistCollection::createFolderPlaylist()
{
    QString folder = KFileDialog::getExistingDirectory();

    if(folder.isEmpty())
        return;

    QString name = uniquePlaylistName(folder.mid(folder.findRev('/') + 1));
    name = playlistNameDialog(i18n("Create Folder Playlist"), name);

    if(!name.isNull())
        raise(new FolderPlaylist(this, folder, name));
}

void PlaylistCollection::guessTagFromFile()
{
    currentPlaylist()->slotGuessTagInfo(TagGuesser::FileName);
}

void PlaylistCollection::guessTagFromInternet()
{
    currentPlaylist()->slotGuessTagInfo(TagGuesser::MusicBrainz);
}

void PlaylistCollection::setSearchEnabled(bool enable)
{
    if(enable == m_searchEnabled)
        return;

    m_searchEnabled = enable;

    currentPlaylist()->setSearchEnabled(enable);
}

HistoryPlaylist *PlaylistCollection::historyPlaylist() const
{
    return m_historyPlaylist;
}

void PlaylistCollection::setHistoryPlaylistEnabled(bool enable)
{
//    kdDebug(65432) << k_funcinfo << enable << endl;

    if((enable && m_historyPlaylist) || (!enable && !m_historyPlaylist))
        return;

    if(enable) {
        action<KToggleAction>("showHistory")->setChecked(true);
        m_historyPlaylist = new HistoryPlaylist(this);
        m_historyPlaylist->setName(i18n("History"));
        setupPlaylist(m_historyPlaylist, "history");
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
    if((action<KToggleAction>("showUpcoming")->isChecked() && m_upcomingPlaylist) ||
	(!enable && !m_upcomingPlaylist))
    {
        return;
    }

    if(enable) {
        action<KToggleAction>("showUpcoming")->setChecked(true);
	if(!m_upcomingPlaylist)
	    m_upcomingPlaylist = new UpcomingPlaylist(this);

	m_upcomingPlaylist->initialize();

        setupPlaylist(m_upcomingPlaylist, "today");
    }
    else {
	action<KToggleAction>("showUpcoming")->setChecked(false);
	bool raiseCollection = m_playlistStack->visibleWidget() == m_upcomingPlaylist;
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

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

Playlist *PlaylistCollection::currentPlaylist() const
{
    return static_cast<Playlist *>(m_playlistStack->visibleWidget());
}

QWidgetStack *PlaylistCollection::playlistStack() const
{
    return m_playlistStack;
}

void PlaylistCollection::raise(Playlist *playlist)
{
    playlist->setSearchEnabled(m_searchEnabled);
    m_playlistStack->raiseWidget(playlist);
    dataChanged();
}

void PlaylistCollection::setupPlaylist(Playlist *playlist, const QString &)
{
    if(!playlist->fileName().isNull())
        m_playlistFiles.insert(playlist->fileName());

    if(!playlist->name().isNull())
        m_playlistNames.insert(playlist->name());

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

void PlaylistCollection::enableDirWatch(bool enable)
{
    if(enable)
        m_dirWatch.startScan(false);
    else
        m_dirWatch.stopScan();
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

    return ok ? uniquePlaylistName(name) : QString::null;
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

void PlaylistCollection::addName(const QString &name)
{
    m_playlistNames.insert(name);
}

void PlaylistCollection::removeName(const QString &name)
{
    m_playlistNames.remove(name);
}

void PlaylistCollection::dirChanged(const QString &path)
{
    CollectionList::instance()->addFiles(path, m_importPlaylists);
}

Playlist *PlaylistCollection::playlistByName(const QString &name) const
{
    QObjectList *l = m_playlistStack->queryList("Playlist");
    Playlist *list = 0;
    QObject *obj;

    for(obj = l->first(); obj; obj = l->next()) {
        Playlist *p = static_cast<Playlist*>(obj);
        if(p->name() == name) {
            list = p;
            break;
        }
    }

    delete l;
    return list;
}


////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistCollection::readConfig()
{
    KConfigGroup config(KGlobal::config(), "Playlists");

    m_importPlaylists  = config.readBoolEntry("ImportPlaylists", true);
    m_folderList       = config.readPathListEntry("DirectoryList");

    QObject::connect(&m_dirWatch, SIGNAL(dirty(const QString &)),
            object(), SLOT(slotDirChanged(const QString &)));

    for(QStringList::ConstIterator it = m_folderList.begin();
        it != m_folderList.end(); ++it)
    {
        m_dirWatch.addDir(*it, false, true);
    }

    m_dirWatch.startScan();
}

void PlaylistCollection::saveConfig()
{
    KConfigGroup config(KGlobal::config(), "Playlists");
    config.writeEntry("ImportPlaylists", m_importPlaylists);
    config.writeEntry("showUpcoming", action<KToggleAction>("showUpcoming")->isChecked());
    config.writePathEntry("DirectoryList", m_folderList);
}

////////////////////////////////////////////////////////////////////////////////
// ActionHanlder implementation
////////////////////////////////////////////////////////////////////////////////

PlaylistCollection::ActionHandler::ActionHandler(PlaylistCollection *collection) :
    QObject(0, "ActionHandler"),
    m_collection(collection)
{
    KActionMenu *menu;

    // "New" menu

    menu = new KActionMenu(i18n("&New"), "filenew", actions(), "file_new");

    menu->insert(createAction(i18n("&Empty Playlist..."), SLOT(slotCreatePlaylist()),
                              "newPlaylist", "window_new", "CTRL+n"));
    menu->insert(createAction(i18n("&Search Playlist..."), SLOT(slotCreateSearchPlaylist()),
                              "newSearchPlaylist", "find", "CTRL+f"));
    menu->insert(createAction(i18n("Playlist From &Folder..."), SLOT(slotCreateFolderPlaylist()),
                              "newDirectoryPlaylist", "fileopen", "CTRL+d"));

    // Guess tag info menu

#if HAVE_MUSICBRAINZ
    menu = new KActionMenu(i18n("&Guess Tag Information"), QString::null, actions(), "guessTag");
    menu->setIconSet(SmallIconSet("wizard"));

    menu->insert(createAction(i18n("From &File Name"), SLOT(slotGuessTagFromFile()),
                              "guessTagFile", "fileimport", "CTRL+g"));
    menu->insert(createAction(i18n("From &Internet"), SLOT(slotGuessTagFromInternet()),
                              "guessTagInternet", "connect_established", "CTRL+i"));
#else
    createAction(i18n("Guess Tag Information From &File Name"), SLOT(slotGuessTagFromFile()),
                 "guessTag", "fileimport", "CTRL+f");
#endif


    createAction(i18n("Play First Track"),SLOT(slotPlayFirst()),    "playFirst");
    createAction(i18n("Open..."),         SLOT(slotOpen()),         "file_open", "fileopen", "CTRL+o");
    createAction(i18n("Add &Folder..."),  SLOT(slotAddFolder()),    "openDirectory", "fileopen");
    createAction(i18n("&Rename..."),      SLOT(slotRename()),       "renamePlaylist", "lineedit");
    createAction(i18n("D&uplicate..."),   SLOT(slotDuplicate()),    "duplicatePlaylist", "editcopy");
    createAction(i18n("Save"),            SLOT(slotSave()),         "file_save", "filesave", "CTRL+s");
    createAction(i18n("Save As..."),      SLOT(slotSaveAs()),       "file_save_as", "filesaveas");
    createAction(i18n("R&emove"),         SLOT(slotRemove()),       "deleteItemPlaylist", "edittrash");
    createAction(i18n("Reload"),          SLOT(slotReload()),       "reloadPlaylist", "reload");
    createAction(i18n("Edit Search..."),  SLOT(slotEditSearch()),   "editSearch", "editclear");

    createAction(i18n("&Delete"),         SLOT(slotRemoveItems()),  "removeItem", "editdelete");
    createAction(i18n("Refresh"),         SLOT(slotRefreshItems()), "refresh", "reload");
    createAction(i18n("&Rename File"),    SLOT(slotRenameItems()),  "renameFile", "filesaveas", "CTRL+r");

    KToggleAction *historyAction =
        new KToggleAction(i18n("Show &History"), "history",  0, actions(), "showHistory");
    historyAction->setCheckedState(i18n("Hide &History"));

    KToggleAction *upcomingAction =
        new KToggleAction(i18n("Show &Play Queue"), "today", 0, actions(), "showUpcoming");
    upcomingAction->setCheckedState(i18n("Hide &Play Queue"));

    connect(action<KToggleAction>("showHistory"), SIGNAL(toggled(bool)),
            this, SLOT(slotSetHistoryPlaylistEnabled(bool)));
    connect(action<KToggleAction>("showUpcoming"), SIGNAL(toggled(bool)),
            this, SLOT(slotSetUpcomingPlaylistEnabled(bool)));
}

KAction *PlaylistCollection::ActionHandler::createAction(const QString &text,
                                                         const char *slot,
                                                         const char *name,
                                                         const QString &icon,
                                                         const KShortcut &shortcut)
{
    if(icon.isNull())
        return new KAction(text, shortcut, this, slot, actions(), name);
    else
        return new KAction(text, icon, shortcut, this, slot, actions(), name);
}


#include "playlistcollection.moc"
