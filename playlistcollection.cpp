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

#include "collectionlist.h"
#include "playlistcollection.h"
#include "actioncollection.h"
#include "advancedsearchdialog.h"
#include "searchplaylist.h"
#include "folderplaylist.h"
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
    m_importPlaylists(true),
    m_searchEnabled(true),
    m_playing(false)
{
    m_actionHandler = new ActionHandler(this);
    PlayerManager::instance()->setPlaylistInterface(this);
}

PlaylistCollection::~PlaylistCollection()
{
    delete m_actionHandler;
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
    update();
}

void PlaylistCollection::playPrevious()
{
    m_playing = true;
    currentPlaylist()->playPrevious();
    update();
}

void PlaylistCollection::playNext()
{
    m_playing = true;
    currentPlaylist()->playNext();
    update();
}

void PlaylistCollection::stop()
{
    m_playing = false;
    currentPlaylist()->stop();
    update();
}

bool PlaylistCollection::playing() const
{
    return m_playing;
}

void PlaylistCollection::open(const QStringList &l)
{
    QStringList files = l;

    if(files.isEmpty())
        files = MediaFiles::openDialog(widget);

    
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


        if(result.addPlaylists && !m_importPlaylists)
            open(m_folderList);
        else
            open(result.addedDirs);

        m_importPlaylists = result.addPlaylists;
    }
}

void PlaylistCollection::rename()
{
    QString name = playlistNameDialog(i18n("Rename"), currentPlaylist()->name());

    if(name.isNull())
        return;

    currentPlaylist()->setName(name);

    // currentPlaylist()->slotRenameFile();
    // refresh editor
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
}

void PlaylistCollection::setupPlaylist(Playlist *playlist, const QString &)
{
    // Add this playlist file to our list.  If it's already in the list remove
    // the item.

    if(!playlist->fileName().isEmpty() &&
       m_playlistFiles.insert(playlist->fileName()))
    {
        playlist->deleteLater();
    }

    if(!playlist->name().isNull())
        m_playlistNames.insert(playlist->name());

    QObject::connect(playlist, SIGNAL(selectionChanged()),
                     object(), SIGNAL(signalSelectedItemsChanged()));
}

bool PlaylistCollection::importPlaylists() const
{
    return m_importPlaylists;
}

QString PlaylistCollection::playlistNameDialog(const QString &caption,
                                               const QString &suggest) const
{
    bool ok;

    QString name = KInputDialog::getText(
        caption,
        i18n("Please enter a name for this playlist:"), uniquePlaylistName(suggest), &ok);

    return ok ? uniquePlaylistName(name) : QString::null;
}


QString PlaylistCollection::uniquePlaylistName(const QString &suggest) const
{
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


////////////////////////////////////////////////////////////////////////////////
// private methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistCollection::readConfig()
{
    if(!m_restore)
        return;

    KConfigGroup config(KGlobal::config(), "Playlists");

    m_importPlaylists = config.readBoolEntry("ImportPlaylists", true);
    m_folderList      = config.readPathListEntry("DirectoryList");

    // QTimer::singleShot(0, this, SLOT(slotScanDirectories()));
    // connect(&m_dirWatch, SIGNAL(dirty(const QString &)),
    // this, SLOT(slotDirChanged(const QString &)));

    for(QStringList::ConstIterator it = m_folderList.begin();
        it != m_folderList.end(); ++it)
    {
        m_dirWatch.addDir(*it, false, true);
    }

    m_dirWatch.startScan();
}

void PlaylistCollection::saveConfig()
{
    if(!m_restore)
        return;

    KConfigGroup config(KGlobal::config(), "Playlists");
    config.writeEntry("ImportPlaylists", m_importPlaylists);
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
    menu->insert(createAction(i18n("&Search Playlist"), SLOT(slotCreateSearchPlaylist()),
                              "newSearchPlaylist", "find", "CTRL+f"));
    menu->insert(createAction(i18n("Playlist From &Folder..."), SLOT(slotCreateFolderPlaylist()),
                              "newDirectoryPlaylist", "fileopen", "CTRL+d"));

    // Guess tag info menu

#if HAVE_MUSICBRAINZ
    menu = new KActionMenu(i18n("&Guess Tag Information"), QString::null, actions(), "guessTag");
    menu->setIconSet(SmallIconSet("wizard"));

    menu->insert(createAction(i18n("From &Filename"), SLOT(slotGuessTagFromFile()),
                              "guessTagFile", "fileimport", "CTRL+g"));
    menu->insert(createAction(i18n("From &Internet"), SLOT(slotGuessTagFromInternet()),
                              "guessTagInternet", "connect_established", "CTRL+i"));
#else
    createAction(i18n("Guess Tag Information From &Filename"), SLOT(slotGuessTagFromFile()),
                 "guessTag", "fileimport", "CTRL+f");
#endif


    createAction(i18n("Play First Song"), SLOT(slotPlayFirst()),    "playFirst");
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
    createAction(i18n("Refresh Items"),   SLOT(slotRefreshItems()), "refresh", "reload");
    createAction(i18n("&Rename File"),    SLOT(slotRenameItems()),  "renameFile", "filesaveas", "CTRL+r");
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
