/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
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

#include "playlist.h"
#include "collectionlist.h"
#include "dynamicplaylist.h"
#include "upcomingplaylist.h"
#include "historyplaylist.h"
#include "viewmode.h"
#include "searchplaylist.h"
#include "treeviewitemplaylist.h"
#include "actioncollection.h"
#include "cache.h"
#include "tracksequencemanager.h"
#include "tagtransactionmanager.h"
#include "playermanager.h"
#include "dbuscollectionproxy.h"
#include "juk_debug.h"

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox public methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::PlaylistBox(PlayerManager *player, QWidget *parent, QStackedWidget *playlistStack) :
    QTreeWidget(parent),
    PlaylistCollection(player, playlistStack),
    m_viewModeIndex(0),
    m_hasSelection(false),
    m_doingMultiSelect(false),
    m_dropItem(0),
    m_showTimer(0)
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

    m_contextMenu->addAction( action("file_new") );
    m_contextMenu->addAction( action("renamePlaylist") );
    m_contextMenu->addAction( action("editSearch") );
    m_contextMenu->addAction( action("duplicatePlaylist") );
    m_contextMenu->addAction( action("reloadPlaylist") );
    m_contextMenu->addAction( action("deleteItemPlaylist") );
    m_contextMenu->addAction( action("file_save") );
    m_contextMenu->addAction( action("file_save_as") );

    m_contextMenu->addSeparator();

    // add the view modes stuff

    KSelectAction *viewModeAction =
        new KSelectAction( QIcon::fromTheme(QStringLiteral("view-choose")), i18n("View Modes"), ActionCollection::actions());
    ActionCollection::actions()->addAction("viewModeMenu", viewModeAction);

    ViewMode* viewmode = new ViewMode(this);
    m_viewModes.append(viewmode);
    viewModeAction->addAction(QIcon::fromTheme(QStringLiteral("view-list-details")), viewmode->name());

    CompactViewMode* compactviewmode = new CompactViewMode(this);
    m_viewModes.append(compactviewmode);
    viewModeAction->addAction(QIcon::fromTheme(QStringLiteral("view-list-text")), compactviewmode->name());

    // TODO: Fix the broken tree view mode
#if 0
    TreeViewMode* treeviewmode = new TreeViewMode(this);
    m_viewModes.append(treeviewmode);
    viewModeAction->addAction(QIcon::fromTheme(QStringLiteral("view-list-tree")), treeviewmode->name());
#endif

    CollectionList::initialize(this);

    viewModeAction->setCurrentItem(m_viewModeIndex);
    m_viewModes[m_viewModeIndex]->setShown(true);

    TrackSequenceManager::instance()->setCurrentPlaylist(CollectionList::instance());
    raise(CollectionList::instance());

    m_contextMenu->addAction( viewModeAction );
    connect(viewModeAction, SIGNAL(triggered(int)), this, SLOT(slotSetViewMode(int)));

    connect(this, SIGNAL(itemSelectionChanged()),
            this, SLOT(slotPlaylistChanged()));

    connect(this, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(slotDoubleClicked(QTreeWidgetItem*)));

    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(slotShowContextMenu(QPoint)));

    TagTransactionManager *tagManager = TagTransactionManager::instance();
    connect(tagManager, SIGNAL(signalAboutToModifyTags()), SLOT(slotFreezePlaylists()));
    connect(tagManager, SIGNAL(signalDoneModifyingTags()), SLOT(slotUnfreezePlaylists()));

    setupUpcomingPlaylist();

    connect(CollectionList::instance(), SIGNAL(signalNewTag(QString,uint)),
            this, SLOT(slotAddItem(QString,uint)));
    connect(CollectionList::instance(), SIGNAL(signalRemovedTag(QString,uint)),
            this, SLOT(slotRemoveItem(QString,uint)));
    connect(CollectionList::instance(), SIGNAL(cachedItemsLoaded()),
            this, SLOT(slotLoadCachedPlaylists()));

    m_savePlaylistTimer = 0;

    KToggleAction *historyAction =
        new KToggleAction(QIcon::fromTheme(QStringLiteral("view-history")), i18n("Show &History"), ActionCollection::actions());
    ActionCollection::actions()->addAction("showHistory", historyAction);
    connect(historyAction, SIGNAL(triggered(bool)),
            this, SLOT(slotSetHistoryPlaylistEnabled(bool)));

    m_showTimer = new QTimer(this);
    connect(m_showTimer, SIGNAL(timeout()), SLOT(slotShowDropTarget()));

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
    ItemList items = selectedBoxItems();

    if(items.isEmpty())
        return;

    QStringList files;
    QStringList names;

    foreach(Item *item, items) {
        if(item && item->playlist())
        {
           if (!item->playlist()->fileName().isEmpty() &&
               QFileInfo(item->playlist()->fileName()).exists())
           {
            files.append(item->playlist()->fileName());
           }

           names.append(item->playlist()->name());
        }
    }

    if(!files.isEmpty()) {
        int remove = KMessageBox::warningYesNoCancelList(
            this, i18n("Do you want to delete these files from the disk as well?"), files, QString(), KStandardGuiItem::del(), KGuiItem(i18n("Keep")));

        if(remove == KMessageBox::Yes) {
            QStringList couldNotDelete;
            for(QStringList::ConstIterator it = files.constBegin(); it != files.constEnd(); ++it) {
                if(!QFile::remove(*it))
                    couldNotDelete.append(*it);
            }

            if(!couldNotDelete.isEmpty())
                KMessageBox::errorList(this, i18n("Could not delete these files."), couldNotDelete);
        }
        else if(remove == KMessageBox::Cancel)
            return;
    }
    else if(items.count() > 1 || items.front()->playlist() != upcomingPlaylist()) {
        if(KMessageBox::warningContinueCancelList(this,
                                                  i18n("Are you sure you want to remove these "
                                                       "playlists from your collection?"),
                                                  names,
                                                  i18n("Remove Items?"),
                                                  KGuiItem(i18n("&Remove"), "user-trash")) == KMessageBox::Cancel)
        {
            return;
        }
    }

    PlaylistList removeQueue;

    for(ItemList::ConstIterator it = items.constBegin(); it != items.constEnd(); ++it) {
        if(*it != Item::collectionItem() &&
           (*it)->playlist() &&
           (!(*it)->playlist()->readOnly()))
        {
            removeQueue.append((*it)->playlist());
        }
    }

    // FIXME removing items
    /*if(items.back()->nextSibling() && static_cast<Item *>(items.back()->nextSibling())->playlist())
        setSingleItem(items.back()->nextSibling());
    else {
        Item *i = static_cast<Item *>(items.front()->itemAbove());
        while(i && !i->playlist())
            i = static_cast<Item *>(i->itemAbove());

        if(!i)
            i = Item::collectionItem();

        setSingleItem(i);
    }*/

    for(PlaylistList::ConstIterator it = removeQueue.constBegin(); it != removeQueue.constEnd(); ++it) {
        if(*it != upcomingPlaylist())
            delete *it;
        else {
            action<KToggleAction>("showUpcoming")->setChecked(false);
            setUpcomingPlaylistEnabled(false);
        }
    }
}

void PlaylistBox::setDynamicListsFrozen(bool frozen)
{
    for(QList<ViewMode *>::Iterator it = m_viewModes.begin();
        it != m_viewModes.end();
        ++it)
    {
        (*it)->setDynamicListsFrozen(frozen);
    }
}

void PlaylistBox::slotSavePlaylists()
{
    qCDebug(JUK_LOG) << "Auto-saving playlists.\n";

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
    if(!m_dropItem) {
        qCCritical(JUK_LOG) << "Trying to show the playlist of a null item!\n";
        return;
    }

    raise(m_dropItem->playlist());
}

void PlaylistBox::slotAddItem(const QString &tag, unsigned column)
{
    for(QList<ViewMode *>::Iterator it = m_viewModes.begin(); it != m_viewModes.end(); ++it)
        (*it)->addItems(QStringList(tag), column);
}

void PlaylistBox::slotRemoveItem(const QString &tag, unsigned column)
{
    for(QList<ViewMode *>::Iterator it = m_viewModes.begin(); it != m_viewModes.end(); ++it)
        (*it)->removeItem(tag, column);
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

PlaylistBox::ItemList PlaylistBox::selectedBoxItems() const
{
    ItemList l;

    for(QTreeWidgetItemIterator it(const_cast<PlaylistBox *>(this),
                                 QTreeWidgetItemIterator::Selected); *it; ++it)
        l.append(static_cast<Item *>(*it));

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

    QTreeWidgetItem* hovered_item = itemAt(event->pos());
    if(hovered_item)
        raise(static_cast<Item *>(hovered_item)->playlist());
}


////////////////////////////////////////////////////////////////////////////////
// PlaylistBox private slots
////////////////////////////////////////////////////////////////////////////////

void PlaylistBox::slotPlaylistChanged()
{
    // Don't update while the mouse is pressed down.

    if(m_doingMultiSelect)
        return;

    ItemList items = selectedBoxItems();
    m_hasSelection = !items.isEmpty();

    bool allowReload = false;

    PlaylistList playlists;
    for(ItemList::ConstIterator it = items.constBegin(); it != items.constEnd(); ++it) {

        Playlist *p = (*it)->playlist();
        if(p) {
            if(p->canReload())
                allowReload = true;
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

        if(playlists.front() == upcomingPlaylist())
            action("deleteItemPlaylist")->setText(i18n("Hid&e"));
        else
            action("deleteItemPlaylist")->setText(i18n("R&emove"));
    }
    else if(!playlists.isEmpty())
        createDynamicPlaylist(playlists);
}

void PlaylistBox::slotDoubleClicked(QTreeWidgetItem *item)
{
    if(!item)
        return;

    TrackSequenceManager *manager = TrackSequenceManager::instance();
    Item *playlistItem = static_cast<Item *>(item);

    manager->setCurrentPlaylist(playlistItem->playlist());

    manager->setCurrent(0); // Reset playback
    PlaylistItem *next = manager->nextItem(); // Allow manager to choose

    if(next) {
        emit startFilePlayback(next->file());
        playlistItem->playlist()->setPlaying(next);
    }
    else
        action("stop")->trigger();
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
    connect(m_savePlaylistTimer, SIGNAL(timeout()), SLOT(slotSavePlaylists()));

    clearSelection();
    setCurrentItem(m_playlistDict[CollectionList::instance()]);

    QTimer::singleShot(0, CollectionList::instance(), SLOT(slotCheckCache()));
    QTimer::singleShot(0, object(), SLOT(slotScanFolders()));
}

////////////////////////////////////////////////////////////////////////////////
// PlaylistBox::Item protected methods
////////////////////////////////////////////////////////////////////////////////

PlaylistBox::Item *PlaylistBox::Item::m_collectionItem = 0;

PlaylistBox::Item::Item(PlaylistBox *listBox, const QString &icon, const QString &text, Playlist *l)
    : QObject(listBox), QTreeWidgetItem(listBox, QStringList(text)),
      PlaylistObserver(l),
      m_playlist(l), m_iconName(icon), m_sortedFirst(false)
{
    init();
}

PlaylistBox::Item::Item(Item *parent, const QString &icon, const QString &text, Playlist *l)
    : QObject(parent->listView()), QTreeWidgetItem(parent, QStringList(text)),
    PlaylistObserver(l),
    m_playlist(l), m_iconName(icon), m_sortedFirst(false)
{
    init();
}

PlaylistBox::Item::~Item()
{

}

// FIXME paintcell
/*void PlaylistBox::Item::paintCell(QPainter *painter, const QColorGroup &colorGroup, int column, int width, int align)
{
    PlaylistBox *playlistBox = static_cast<PlaylistBox *>(listView());
    playlistBox->viewMode()->paintCell(this, painter, colorGroup, column, width, align);
}*/

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
    //FIXME viewmode
    //listView()->viewMode()->queueRefresh();
}

void PlaylistBox::Item::playingItemHasChanged()
{
}

void PlaylistBox::Item::playlistItemDataHasChanged()
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
        connect(m_playlist, SIGNAL(signalNameChanged(QString)),
                this, SLOT(slotSetName(QString)));
        connect(m_playlist, SIGNAL(signalEnableDirWatch(bool)),
                list->object(), SLOT(slotEnableDirWatch(bool)));
    }

    if(m_playlist == CollectionList::instance()) {
        m_sortedFirst = true;
        m_collectionItem = this;
        list->viewMode()->setupDynamicPlaylists();
    }

    if(m_playlist == list->historyPlaylist() || m_playlist == list->upcomingPlaylist())
        m_sortedFirst = true;

    setText(1, sortTextFor(itemText));
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
