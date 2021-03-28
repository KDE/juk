/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2021      Michael Pyne  <mpyne@kde.org>
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

#include "playlistbox.h"

#include <kmessagebox.h>
#include <kactioncollection.h>
#include <ktoggleaction.h>
#include <kselectaction.h>
#include <kconfiggroup.h>
#include <KSharedConfig>

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QTimer>
#include <QDragLeaveEvent>
#include <QList>
#include <QDragMoveEvent>
#include <QKeyEvent>
#include <QDropEvent>
#include <QMouseEvent>
#include <QFileInfo>
#include <QTime>
#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QElapsedTimer>

#include "actioncollection.h"
#include "cache.h"
#include "collectionlist.h"
#include "dbuscollectionproxy.h"
#include "dynamicplaylist.h"
#include "historyplaylist.h"
#include "iconsupport.h"
#include "juk_debug.h"
#include "playermanager.h"
#include "playlist.h"
#include "searchplaylist.h"
#include "tagtransactionmanager.h"
#include "treeviewitemplaylist.h"
#include "upcomingplaylist.h"
#include "viewmode.h"

using namespace ActionCollection; // ""_act and others
using namespace IconSupport;      // ""_icon

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::PlaylistBox(PlayerManager *player, QWidget *parent, QStackedWidget *playlistStack)
  : QTreeWidget(parent)
  , PlaylistCollection(player, playlistStack)
{
    readConfig();
    setHeaderLabel("Playlists");
    setRootIsDecorated(false);
    setContextMenuPolicy(Qt::CustomContextMenu);
    viewport()->setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DropOnly);
    setDropIndicatorShown(true);

    setColumnCount(2); // Use fake column for sorting
    setColumnHidden(1, true);
    setSortingEnabled(true);
    sortByColumn(1, Qt::AscendingOrder);

    header()->blockSignals(true);
    header()->hide();
    header()->blockSignals(false);

    setSelectionMode(QAbstractItemView::ExtendedSelection);

    m_contextMenu = new QMenu(this);

    m_contextMenu->addAction(action("file_new"));
    m_contextMenu->addAction(action("renamePlaylist"));
    m_contextMenu->addAction(action("editSearch"));
    m_contextMenu->addAction(action("duplicatePlaylist"));
    m_contextMenu->addAction(action("reloadPlaylist"));
    m_contextMenu->addAction(action("deleteItemPlaylist"));
    m_contextMenu->addAction(action("file_save"));
    m_contextMenu->addAction(action("file_save_as"));

    m_contextMenu->addSeparator();

    // add the view modes stuff

    KSelectAction *viewModeAction =
        new KSelectAction("view-choose"_icon, i18n("View Modes"), ActionCollection::actions());
    ActionCollection::actions()->addAction("viewModeMenu", viewModeAction);

    ViewMode* viewmode = new ViewMode(this);
    m_viewModes.append(viewmode);
    viewModeAction->addAction("view-list-details"_icon, viewmode->name());

    CompactViewMode* compactviewmode = new CompactViewMode(this);
    m_viewModes.append(compactviewmode);
    viewModeAction->addAction("view-list-text"_icon, compactviewmode->name());

    // TODO: Fix the broken tree view mode
#if 0
    TreeViewMode* treeviewmode = new TreeViewMode(this);
    m_viewModes.append(treeviewmode);
    viewModeAction->addAction("view-list-tree"_icon, treeviewmode->name());
#endif

    CollectionList::initialize(this);

    viewModeAction->setCurrentItem(m_viewModeIndex);
    m_viewModes[m_viewModeIndex]->setShown(true);

    raise(CollectionList::instance());

    m_contextMenu->addAction(viewModeAction);
    connect(viewModeAction, &QAction::triggered,
            this, &PlaylistBox::slotSetViewMode);

    connect(this, &PlaylistBox::itemSelectionChanged,
            this, &PlaylistBox::slotPlaylistChanged);

    connect(this, &PlaylistBox::itemDoubleClicked,
            this, &PlaylistBox::slotDoubleClicked);

    connect(this, &PlaylistBox::customContextMenuRequested,
            this, &PlaylistBox::slotShowContextMenu);

    connect(this, &PlaylistBox::signalPlayFile,
            player, qOverload<const FileHandle &>(&PlayerManager::play));

    const auto *tagManager = TagTransactionManager::instance();
    connect(tagManager, &TagTransactionManager::signalAboutToModifyTags,
            this,       &PlaylistBox::slotFreezePlaylists);
    connect(tagManager, &TagTransactionManager::signalDoneModifyingTags,
            this,       &PlaylistBox::slotUnfreezePlaylists);

    setupUpcomingPlaylist();

    const auto *collectionList = CollectionList::instance();
    connect(collectionList, &CollectionList::signalNewTag,
            this,           &PlaylistBox::slotAddItem);
    connect(collectionList, &CollectionList::signalRemovedTag,
            this,           &PlaylistBox::slotRemoveItem);
    connect(collectionList, &CollectionList::cachedItemsLoaded,
            this,           &PlaylistBox::slotLoadCachedPlaylists);

    KToggleAction *historyAction =
        new KToggleAction("view-history"_icon, i18n("Show &History"), ActionCollection::actions());
    ActionCollection::actions()->addAction("showHistory", historyAction);
    connect(historyAction, &KToggleAction::triggered,
            this,          &PlaylistBox::slotSetHistoryPlaylistEnabled);

    m_showTimer = new QTimer(this);
    m_showTimer->setSingleShot(true);
    m_showTimer->setInterval(500);
    connect(m_showTimer, &QTimer::timeout,
            this,        &PlaylistBox::slotShowDropTarget);

    // hook up to the D-Bus
    (void) new DBusCollectionProxy(this, this);
}

PlaylistBox::~PlaylistBox()
{
    PlaylistList l;
    CollectionList *collection = CollectionList::instance();
    for(QTreeWidgetItemIterator it(topLevelItem(0)); *it; ++it) {
        Item *item = static_cast<Item *>(*it);
        if(item->playlist() && item->playlist() != collection)
            l.append(item->playlist());
    }

    Cache::savePlaylists(l);
    saveConfig();

    // Some view modes use event filters onto sibling widgets which may be
    // destroyed before the view mode.
    // Manually delete the view modes instead.
    qDeleteAll(m_viewModes);
    m_viewModes.clear();
}

void PlaylistBox::raise(Playlist *playlist)
{
    if(!playlist)
        return;

    Item *i = m_playlistDict.value(playlist, 0);

    if(i) {
        clearSelection();
        setCurrentItem(i);

        setSingleItem(i);
        scrollToItem(currentItem());
    }
    else
        PlaylistCollection::raise(playlist);

    slotPlaylistChanged();
}

void PlaylistBox::duplicate()
{
    Item *item = static_cast<Item *>(currentItem());
    if(!item || !item->playlist())
        return;

    QString name = playlistNameDialog(i18nc("verb, copy the playlist", "Duplicate"), item->text(0));

    if(name.isNull())
        return;

    Playlist *p = new Playlist(this, name);
    p->createItems(item->playlist()->items());
}

void PlaylistBox::scanFolders()
{
    PlaylistCollection::scanFolders();
    emit startupComplete();
}

bool PlaylistBox::requestPlaybackFor(const FileHandle &file)
{
    emit signalPlayFile(file);
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::paste()
{
    // TODO: Reimplement
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox protected methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::slotFreezePlaylists()
{
    setDynamicListsFrozen(true);
}

void PlaylistBox::slotUnfreezePlaylists()
{
    setDynamicListsFrozen(false);
}

void PlaylistBox::slotPlaylistDataChanged()
{
    if(m_savePlaylistTimer)
        m_savePlaylistTimer->start(); // Restarts the timer if it's already running.
}

void PlaylistBox::slotSetHistoryPlaylistEnabled(bool enable)
{
    setHistoryPlaylistEnabled(enable);
}

void PlaylistBox::setupPlaylist(Playlist *playlist, const QString &iconName)
{
    setupPlaylist(playlist, iconName, nullptr);
}

void PlaylistBox::setupPlaylist(Playlist *playlist, const QString &iconName, Item *parentItem)
{
    connect(playlist, &Playlist::signalPlaylistItemsDropped,
            this,     &PlaylistBox::slotPlaylistItemsDropped);
    connect(playlist, &Playlist::signalMoveFocusAway,
            this,     &PlaylistBox::signalMoveFocusAway);

    PlaylistCollection::setupPlaylist(playlist, iconName);

    if(parentItem)
        new Item(parentItem, iconName, playlist->name(), playlist);
    else
        new Item(this, iconName, playlist->name(), playlist);
}

void PlaylistBox::removePlaylist(Playlist *playlist)
{
    // Could be false if setup() wasn't run yet.
    if(m_playlistDict.contains(playlist)) {
        removeNameFromDict(m_playlistDict[playlist]->text(0));
        delete m_playlistDict[playlist]; // Delete the Item*
    }

    removeFileFromDict(playlist->fileName());
    m_playlistDict.remove(playlist);
}

Qt::DropActions PlaylistBox::supportedDropActions() const
{
    return Qt::CopyAction;
}

bool PlaylistBox::dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action)
{
    Q_UNUSED(index);

    // The *parent* item won't be null, but index should be zero except in the
    // still-broken "tree view" mode.

    if(!parent || action != Qt::CopyAction || !data->hasUrls()) {
        return false;
    }

    auto *playlistItem = static_cast<Item *>(parent);
    if(!playlistItem) {
        return false;
    }

    auto *playlist = playlistItem->playlist();
    const auto droppedUrls = data->urls();
    PlaylistItem *lastItem = nullptr;

    for(const auto &url : droppedUrls) {
        lastItem = playlist->createItem(FileHandle(url.toLocalFile()), lastItem);
    }

    return true;
}

QStringList PlaylistBox::mimeTypes() const
{
    auto result = QTreeWidget::mimeTypes();

    // Need to add Playlists's mime type to convince QTreeWidget to allow it as
    // a drop option.
    result.append(QLatin1String("text/uri-list"));

    return result;
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox private methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::readConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), "PlaylistBox");
    m_viewModeIndex = config.readEntry("ViewMode", 0);

    // TODO Restore ability to use Tree View once fixed.
    if(m_viewModeIndex == 2) {
        m_viewModeIndex = 0;
    }
}

void PlaylistBox::saveConfig()
{
    KConfigGroup config(KSharedConfig::openConfig(), "PlaylistBox");
    config.writeEntry("ViewMode", action<KSelectAction>("viewModeMenu")->currentItem());
    KSharedConfig::openConfig()->sync();
}

void PlaylistBox::remove()
{
    const ItemList items = selectedBoxItems();

    if(items.isEmpty())
        return;

    QStringList files;
    QStringList names;

    for(const auto &item : items) {
        if(!item || !item->playlist()) {
            qFatal("Ran into an empty item or item playlist when removing playlists!");
        }

        if (!item->playlist()->fileName().isEmpty() &&
            QFileInfo::exists(item->playlist()->fileName()))
        {
            files.append(item->playlist()->fileName());
        }

        names.append(item->playlist()->name());
    }

    if(!files.isEmpty()) {
        int remove = KMessageBox::warningYesNoCancelList(
            this, i18n("Do you want to delete these files from the disk as well?"), files, QString(), KStandardGuiItem::del(), KGuiItem(i18n("Keep")));

        if(remove == KMessageBox::Yes) {
            QStringList couldNotDelete;
            for(const auto &playlistFile : qAsConst(files)) {
                if(!QFile::remove(playlistFile))
                    couldNotDelete.append(playlistFile);
            }

            if(!couldNotDelete.isEmpty())
                KMessageBox::errorList(this, i18n("Could not delete these files."), couldNotDelete);
        }
        else if(remove == KMessageBox::Cancel)
            return;
    }
    else if(items.count() > 1 || items.front()->playlist() != upcomingPlaylist()) {
        if(KMessageBox::Cancel == KMessageBox::warningContinueCancelList(
            this,
            i18n("Are you sure you want to remove these "
               "playlists from your collection?"),
            names,
            i18n("Remove Items?"),
            KGuiItem(i18n("&Remove"), "user-trash")))
        {
            return;
        }
    }

    for(const auto &item : items) {
        if(item != Item::collectionItem() &&
           !item->playlist()->readOnly())
        {
            if(item->playlist() != upcomingPlaylist())
                delete item;
            else {
                action<KToggleAction>("showUpcoming")->setChecked(false);
                setUpcomingPlaylistEnabled(false);
            }
        }
    }

    setSingleItem(Item::collectionItem());
}

void PlaylistBox::setDynamicListsFrozen(bool frozen)
{
    for(auto &playlistBoxItem : qAsConst(m_viewModes)) {
        playlistBoxItem->setDynamicListsFrozen(frozen);
    }
}

void PlaylistBox::slotSavePlaylists()
{
    qCDebug(JUK_LOG) << "Auto-saving playlists.";

    PlaylistList l;
    CollectionList *collection = CollectionList::instance();
    for(QTreeWidgetItemIterator it(topLevelItem(0)); *it; ++it) {
        Item *item = static_cast<Item *>(*it);
        if(item->playlist() && item->playlist() != collection)
            l.append(item->playlist());
    }

    Cache::savePlaylists(l);
}

void PlaylistBox::slotShowDropTarget()
{
    if(m_dropItem) raise(m_dropItem->playlist());
}

void PlaylistBox::slotAddItem(const QString &tag, unsigned column)
{
    for(auto &viewMode : qAsConst(m_viewModes)) {
        viewMode->addItems(QStringList(tag), column);
    }
}

void PlaylistBox::slotRemoveItem(const QString &tag, unsigned column)
{
    for(auto &viewMode : qAsConst(m_viewModes)) {
        viewMode->removeItem(tag, column);
    }
}

void PlaylistBox::mousePressEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton)
        m_doingMultiSelect = true;
    QTreeWidget::mousePressEvent(e);
}

void PlaylistBox::mouseReleaseEvent(QMouseEvent *e)
{
    if(e->button() == Qt::LeftButton) {
        m_doingMultiSelect = false;
        slotPlaylistChanged();
    }
    QTreeWidget::mouseReleaseEvent(e);
}

void PlaylistBox::keyPressEvent(QKeyEvent *e)
{
    if((e->key() == Qt::Key_Up || e->key() == Qt::Key_Down) && e->modifiers() == Qt::ShiftModifier)
        m_doingMultiSelect = true;
    QTreeWidget::keyPressEvent(e);
}

void PlaylistBox::keyReleaseEvent(QKeyEvent *e)
{
    if(m_doingMultiSelect && e->key() == Qt::Key_Shift) {
        m_doingMultiSelect = false;
        slotPlaylistChanged();
    }
    QTreeWidget::keyReleaseEvent(e);
}

PlaylistBox::ItemList PlaylistBox::selectedBoxItems()
{
    ItemList l;

    for(QTreeWidgetItemIterator it(this, QTreeWidgetItemIterator::Selected)
            ; *it
            ; ++it)
    {
        l.append(static_cast<Item *>(*it));
    }

    return l;
}

void PlaylistBox::setSingleItem(QTreeWidgetItem *item)
{
    setSelectionMode(QAbstractItemView::SingleSelection);
    setCurrentItem(item);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
}

void PlaylistBox::dragMoveEvent(QDragMoveEvent* event)
{
    QTreeWidget::dragMoveEvent(event);

    Item* hovered_item = static_cast<Item*>(itemAt(event->pos()));
    if(hovered_item != m_dropItem){
        m_dropItem = hovered_item;
        if(m_dropItem) m_showTimer->start();
        else m_showTimer->stop();
    };
}

void PlaylistBox::dragLeaveEvent(QDragLeaveEvent* event)
{
    QTreeWidget::dragLeaveEvent(event);
    m_showTimer->stop();
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox private slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::slotPlaylistChanged()
{
    // Don't update while the mouse is pressed down.

    if(m_doingMultiSelect)
        return;

    const ItemList items = selectedBoxItems();
    m_hasSelection = !items.isEmpty();

    const bool allowReload = std::any_of(items.begin(), items.end(),
        [](const auto &item) {
            return item->playlist() && item->playlist()->canReload();
        });

    PlaylistList playlists;
    for(const auto &playlistBoxItem : items) {
        auto p = playlistBoxItem->playlist();
        if(p) {
            playlists.append(p);
        }
    }

    bool singlePlaylist = playlists.count() == 1;

    if(playlists.isEmpty() ||
       (singlePlaylist &&
        (playlists.front() == CollectionList::instance() ||
         playlists.front()->readOnly())))
    {
        action("file_save")->setEnabled(false);
        action("file_save_as")->setEnabled(false);
        action("renamePlaylist")->setEnabled(false);
        action("deleteItemPlaylist")->setEnabled(false);
    }
    else {
        action("file_save")->setEnabled(true);
        action("file_save_as")->setEnabled(true);
        action("renamePlaylist")->setEnabled(playlists.count() == 1);
        action("deleteItemPlaylist")->setEnabled(true);
    }
    action("reloadPlaylist")->setEnabled(allowReload);
    action("duplicatePlaylist")->setEnabled(!playlists.isEmpty());

    action("editSearch")->setEnabled(singlePlaylist &&
                                     playlists.front()->searchIsEditable());

    if(singlePlaylist) {
        PlaylistCollection::raise(playlists.front());

        if(playlists.front() == upcomingPlaylist()) {
            action("deleteItemPlaylist")->setText(i18n("Hid&e"));
            action("deleteItemPlaylist")->setIcon("list-remove"_icon);
        }
        else {
            action("deleteItemPlaylist")->setText(i18n("R&emove"));
            action("deleteItemPlaylist")->setIcon("user-trash"_icon);
        }
    }
    else if(!playlists.isEmpty())
        createDynamicPlaylist(playlists);
}

void PlaylistBox::slotDoubleClicked(QTreeWidgetItem *item)
{
    if(!item)
        return;
    auto *playlist = static_cast<Item *>(item)->playlist();

    playlist->slotBeginPlayback();
}

void PlaylistBox::slotShowContextMenu(const QPoint &point)
{
    m_contextMenu->popup(mapToGlobal(point));
}

void PlaylistBox::slotPlaylistItemsDropped(Playlist *p)
{
    raise(p);
}

void PlaylistBox::slotSetViewMode(int index)
{
    if(index == m_viewModeIndex)
        return;

    viewMode()->setShown(false);
    m_viewModeIndex = index;
    viewMode()->setShown(true);
}

void PlaylistBox::setupItem(Item *item)
{
    m_playlistDict.insert(item->playlist(), item);
    viewMode()->queueRefresh();
}

void PlaylistBox::setupUpcomingPlaylist()
{
    KConfigGroup config(KSharedConfig::openConfig(), "Playlists");
    bool enable = config.readEntry("showUpcoming", false);

    setUpcomingPlaylistEnabled(enable);
    action<KToggleAction>("showUpcoming")->setChecked(enable);
}


void PlaylistBox::slotLoadCachedPlaylists()
{
    qCDebug(JUK_LOG) << "Loading cached playlists.";
    QElapsedTimer stopwatch;
    stopwatch.start();

    Cache::loadPlaylists(this);

    qCDebug(JUK_LOG) << "Cached playlists loaded, took" << stopwatch.elapsed() << "ms";

    // Auto-save playlists after they change.
    m_savePlaylistTimer = new QTimer(this);
    m_savePlaylistTimer->setInterval(3000); // 3 seconds with no change? -> commit
    m_savePlaylistTimer->setSingleShot(true);
    connect(m_savePlaylistTimer, &QTimer::timeout,
            this, &PlaylistBox::slotSavePlaylists);

    clearSelection();
    setCurrentItem(m_playlistDict[CollectionList::instance()]);

    QTimer::singleShot(0, this, [this]() {
            CollectionList::instance()->slotCheckCache();
            this->scanFolders();
        });
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox::Item protected methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::Item *PlaylistBox::Item::m_collectionItem = nullptr;

PlaylistBox::Item::Item(PlaylistBox *listBox, const QString &icon, const QString &text, Playlist *l)
    : QObject(listBox), QTreeWidgetItem(listBox, QStringList(text)),
      m_playlist(l), m_iconName(icon), m_sortedFirst(false)
{
    init();
}

PlaylistBox::Item::Item(Item *parent, const QString &icon, const QString &text, Playlist *l)
    : QObject(parent->listView()), QTreeWidgetItem(parent, QStringList(text)),
    m_playlist(l), m_iconName(icon), m_sortedFirst(false)
{
    init();
}

PlaylistBox::Item::~Item()
{

}

void PlaylistBox::Item::setup()
{
    listView()->viewMode()->setupItem(this);
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox::Item protected slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::Item::slotSetName(const QString &name)
{
    setText(0, name); // Display name
    setText(1, sortTextFor(name));
    setSelected(true);

    treeWidget()->scrollToItem(this);
}

void PlaylistBox::Item::playlistItemDataChanged()
{
    // This avoids spuriously re-saving all playlists just because play queue
    // changes.
    if(m_playlist != listView()->upcomingPlaylist())
        listView()->slotPlaylistDataChanged();
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox::Item private methods
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::Item::init()
{
    PlaylistBox *list = listView();

    list->setupItem(this);

    const QString itemText(text());
    setIcon(0, QIcon::fromTheme(m_iconName));
    list->addNameToDict(itemText);

    if(m_playlist) {
        connect(m_playlist, &Playlist::signalNameChanged,
                this,       &Item::slotSetName);
        connect(m_playlist, &Playlist::signalEnableDirWatch,
                this, [list](bool enable) {
                    list->enableDirWatch(enable);
                });
    }

    if(m_playlist == CollectionList::instance()) {
        m_sortedFirst = true;
        m_collectionItem = this;
        list->viewMode()->setupDynamicPlaylists();
    }

    if(m_playlist == list->historyPlaylist() || m_playlist == list->upcomingPlaylist())
        m_sortedFirst = true;

    setText(1, sortTextFor(itemText));

    connect(&(m_playlist->signaller), &PlaylistInterfaceSignaller::playingItemDataChanged, this, &PlaylistBox::Item::playlistItemDataChanged);
}

QString PlaylistBox::Item::sortTextFor(const QString &name) const
{
    // Collection List goes before everything, then
    // playlists that 'sort first', then remainder of
    // playlists.
    const auto prefix
        = (playlist() == CollectionList::instance())
            ? QStringLiteral("0")
            : m_sortedFirst
                ? QStringLiteral("1")
                : QStringLiteral("2");
    return prefix + name;
}

// vim: set et sw=4 tw=0 sta:
