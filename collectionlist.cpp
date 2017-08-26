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

#include "collectionlist.h"

#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kmenu.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <kglobal.h>
#include <kactioncollection.h>
#include <ksavefile.h>
#include <kstandarddirs.h>
#include <ktoolbarpopupaction.h>
#include <kdirwatch.h>

#include <QStringBuilder>
#include <QList>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QApplication>
#include <QTimer>
#include <QTime>
#include <QClipboard>
#include <QFileInfo>
#include <QHeaderView>

#include "playlistcollection.h"
#include "splashscreen.h"
#include "stringshare.h"
#include "cache.h"
#include "actioncollection.h"
#include "tag.h"
#include "viewmode.h"

using ActionCollection::action;

////////////////////////////////////////////////////////////////////////////////
// static methods
////////////////////////////////////////////////////////////////////////////////

CollectionList *CollectionList::m_list = 0;

CollectionList *CollectionList::instance()
{
    return m_list;
}

static QTime stopwatch;

void CollectionList::startLoadingCachedItems()
{
    if(!m_list)
        return;

    kDebug() << "Starting to load cached items";
    stopwatch.start();

    if(!Cache::instance()->prepareToLoadCachedItems()) {
        kError() << "Unable to setup to load cache... perhaps it doesn't exist?";

        completedLoadingCachedItems();
        return;
    }

    kDebug() << "Kicked off first batch";
    QTimer::singleShot(0, this, SLOT(loadNextBatchCachedItems()));
}

void CollectionList::loadNextBatchCachedItems()
{
    Cache *cache = Cache::instance();
    bool done = false;

    for(int i = 0; i < 20; ++i) {
        FileHandle cachedItem(cache->loadNextCachedItem());

        if(cachedItem.isNull()) {
            done = true;
            break;
        }

        // This may have already been created via a loaded playlist.
        if(!m_itemsDict.contains(cachedItem.absFilePath())) {
            CollectionListItem *newItem = new CollectionListItem(this, cachedItem);
            setupItem(newItem);
        }
    }

    SplashScreen::update();

    if(!done) {
        QTimer::singleShot(0, this, SLOT(loadNextBatchCachedItems()));
    }
    else {
        completedLoadingCachedItems();
    }
}

void CollectionList::completedLoadingCachedItems()
{
    // The CollectionList is created with sorting disabled for speed.  Re-enable
    // it here, and perform the sort.
    KConfigGroup config(KSharedConfig::openConfig(), "Playlists");

    Qt::SortOrder order = Qt::DescendingOrder;
    if(config.readEntry("CollectionListSortAscending", true))
        order = Qt::AscendingOrder;

    m_list->sortByColumn(config.readEntry("CollectionListSortColumn", 1), order);

    SplashScreen::finishedLoading();

    kDebug() << "Finished loading cached items, took" << stopwatch.elapsed() << "ms";
    kDebug() << m_itemsDict.size() << "items are in the CollectionList";

    emit cachedItemsLoaded();
}

void CollectionList::initialize(PlaylistCollection *collection)
{
    if(m_list)
        return;

    // We have to delay initialization here because dynamic_cast or comparing to
    // the collection instance won't work in the PlaylistBox::Item initialization
    // won't work until the CollectionList is fully constructed.

    m_list = new CollectionList(collection);
    m_list->setName(i18n("Collection List"));

    collection->setupPlaylist(m_list, "folder-sound");
}

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

CollectionListItem *CollectionList::createItem(const FileHandle &file, QTreeWidgetItem *, bool)
{
    // It's probably possible to optimize the line below away, but, well, right
    // now it's more important to not load duplicate items.

    if(m_itemsDict.contains(file.absFilePath()))
        return 0;

    CollectionListItem *item = new CollectionListItem(this, file);

    if(!item->isValid()) {
        kError() << "CollectionList::createItem() -- A valid tag was not created for \""
                 << file.absFilePath() << "\"" << endl;
        delete item;
        return 0;
    }

    setupItem(item);

    return item;
}

void CollectionList::clearItems(const PlaylistItemList &items)
{
    foreach(PlaylistItem *item, items) {
        delete item;
    }

    dataChanged();
}

void CollectionList::setupTreeViewEntries(ViewMode *viewMode) const
{
    TreeViewMode *treeViewMode = dynamic_cast<TreeViewMode *>(viewMode);
    if(!treeViewMode) {
        kWarning() << "Can't setup entries on a non-tree-view mode!\n";
        return;
    }

    QList<int> columnList;
    columnList << PlaylistItem::ArtistColumn;
    columnList << PlaylistItem::GenreColumn;
    columnList << PlaylistItem::AlbumColumn;

    foreach(int column, columnList)
        treeViewMode->addItems(m_columnTags[column]->keys(), column);
}

void CollectionList::slotNewItems(const KFileItemList &items)
{
    QStringList files;

    for(KFileItemList::ConstIterator it = items.constBegin(); it != items.constEnd(); ++it)
        files.append((*it).url().path());

    addFiles(files);
    update();
}

void CollectionList::slotRefreshItems(const QList<QPair<KFileItem, KFileItem> > &items)
{
    for(int i = 0; i < items.count(); ++i) {
        const KFileItem fileItem = items[i].second;
        CollectionListItem *item = lookup(fileItem.url().path());

        if(item) {
            item->refreshFromDisk();

            // If the item is no longer on disk, remove it from the collection.

            if(item->file().fileInfo().exists())
                item->repaint();
            else
                delete item;
        }
    }

    update();
}

void CollectionList::slotDeleteItem(const KFileItem &item)
{
    delete lookup(item.url().path());
}

void CollectionList::saveItemsToCache() const
{
    kDebug() << "Saving collection list to cache";

    QString cacheFileName =
        KGlobal::dirs()->saveLocation("appdata") % QLatin1String("cache");

    KSaveFile f(cacheFileName);

    if(!f.open(QIODevice::WriteOnly)) {
        kError() << "Error saving cache:" << f.errorString();
        return;
    }

    QByteArray data;
    QDataStream s(&data, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_4_3);

    QHash<QString, CollectionListItem *>::const_iterator it;
    for(it = m_itemsDict.begin(); it != m_itemsDict.end(); ++it) {
        s << it.key();
        s << (*it)->file();
    }

    QDataStream fs(&f);

    qint32 checksum = qChecksum(data.data(), data.size());

    fs << qint32(Cache::playlistItemsCacheVersion)
       << checksum
       << data;

    f.close();

    if(!f.finalize())
        kError() << "Error saving cache:" << f.errorString();
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void CollectionList::paste()
{
    decode(QApplication::clipboard()->mimeData());
}

void CollectionList::clear()
{
    int result = KMessageBox::warningContinueCancel(this,
        i18n("Removing an item from the collection will also remove it from "
             "all of your playlists. Are you sure you want to continue?\n\n"
             "Note, however, that if the directory that these files are in is in "
             "your \"scan on startup\" list, they will be readded on startup."));

    if(result == KMessageBox::Continue) {
        Playlist::clear();
        emit signalCollectionChanged();
    }
}

void CollectionList::slotCheckCache()
{
    PlaylistItemList invalidItems;
    kDebug() << "Starting to check cached items for consistency";
    stopwatch.start();

    int i = 0;
    foreach(CollectionListItem *item, m_itemsDict) {
        if(!item->checkCurrent())
            invalidItems.append(item);
        if(++i == (m_itemsDict.size() / 2))
            kDebug() << "Checkpoint";
    }

    clearItems(invalidItems);

    kDebug() << "Finished consistency check, took" << stopwatch.elapsed() << "ms";
}

void CollectionList::slotRemoveItem(const QString &file)
{
    delete m_itemsDict[file];
}

void CollectionList::slotRefreshItem(const QString &file)
{
    if(m_itemsDict[file])
        m_itemsDict[file]->refresh();
}

////////////////////////////////////////////////////////////////////////////////
// protected methods
////////////////////////////////////////////////////////////////////////////////

CollectionList::CollectionList(PlaylistCollection *collection) :
    Playlist(collection, true),
    m_columnTags(15, 0)
{
    QAction *spaction = ActionCollection::actions()->addAction("showPlaying");
    spaction->setText(i18n("Show Playing"));
    connect(spaction, SIGNAL(triggered(bool)), SLOT(slotShowPlaying()));

    connect(action<KToolBarPopupAction>("back")->menu(), SIGNAL(aboutToShow()),
            this, SLOT(slotPopulateBackMenu()));
    connect(action<KToolBarPopupAction>("back")->menu(), SIGNAL(triggered(QAction*)),
            this, SLOT(slotPlayFromBackMenu(QAction*)));
    setSortingEnabled(false); // Temporarily disable sorting to add items faster.

    m_columnTags[PlaylistItem::ArtistColumn] = new TagCountDict;
    m_columnTags[PlaylistItem::AlbumColumn] = new TagCountDict;
    m_columnTags[PlaylistItem::GenreColumn] = new TagCountDict;

    // Even set to true it wouldn't work with this class due to other checks
    setAllowDuplicates(false);
}

CollectionList::~CollectionList()
{
    KConfigGroup config(KSharedConfig::openConfig(), "Playlists");
    config.writeEntry("CollectionListSortColumn", header()->sortIndicatorSection());
    config.writeEntry("CollectionListSortAscending", header()->sortIndicatorOrder() == Qt::AscendingOrder);

    // In some situations the dataChanged signal from clearItems will cause observers to
    // subsequently try to access a deleted item.  Since we're going away just remove all
    // observers.

    clearObservers();

    // The CollectionListItems will try to remove themselves from the
    // m_columnTags member, so we must make sure they're gone before we
    // are.

    clearItems(items());

    qDeleteAll(m_columnTags);
    m_columnTags.clear();
}

void CollectionList::dropEvent(QDropEvent *e)
{
    if(e->source() == this)
        return; // Don't rearrange in the CollectionList.
    else
        Playlist::dropEvent(e);
}

void CollectionList::dragMoveEvent(QDragMoveEvent *e)
{
    if(e->source() != this)
        Playlist::dragMoveEvent(e);
    else
        e->setAccepted(false);
}

QString CollectionList::addStringToDict(const QString &value, int column)
{
    if(column > m_columnTags.count() || value.trimmed().isEmpty())
        return QString();

    if(m_columnTags[column]->contains(value))
        ++((*m_columnTags[column])[value]);
    else {
        m_columnTags[column]->insert(value, 1);
        emit signalNewTag(value, column);
    }

    return value;
}

QStringList CollectionList::uniqueSet(UniqueSetType t) const
{
    int column;

    switch(t)
    {
    case Artists:
        column = PlaylistItem::ArtistColumn;
    break;

    case Albums:
        column = PlaylistItem::AlbumColumn;
    break;

    case Genres:
        column = PlaylistItem::GenreColumn;
    break;

    default:
        return QStringList();
    }

    return m_columnTags[column]->keys();
}

CollectionListItem *CollectionList::lookup(const QString &file) const
{
    return m_itemsDict.value(file, 0);
}

void CollectionList::removeStringFromDict(const QString &value, int column)
{
    if(column > m_columnTags.count() || value.trimmed().isEmpty())
        return;

    if(m_columnTags[column]->contains(value) &&
       --((*m_columnTags[column])[value])) // If the decrement goes to 0...
    {
        emit signalRemovedTag(value, column);
        m_columnTags[column]->remove(value);
    }
}

void CollectionList::addWatched(const QString &file)
{
    m_dirWatch->addFile(file);
}

void CollectionList::removeWatched(const QString &file)
{
    m_dirWatch->removeFile(file);
}

////////////////////////////////////////////////////////////////////////////////
// CollectionListItem public methods
////////////////////////////////////////////////////////////////////////////////

void CollectionListItem::refresh()
{
    int offset = CollectionList::instance()->columnOffset();
    int columns = lastColumn() + offset + 1;

    sharedData()->metadata.resize(columns);
    sharedData()->cachedWidths.resize(columns);

    for(int i = offset; i < columns; i++) {
        setText(i, text(i));
        int id = i - offset;
        if(id != TrackNumberColumn && id != LengthColumn) {
            // All columns other than track num and length need local-encoded data for sorting

            QString toLower = text(i).toLower();

            // For some columns, we may be able to share some strings

            if((id == ArtistColumn) || (id == AlbumColumn) ||
               (id == GenreColumn)  || (id == YearColumn)  ||
               (id == CommentColumn))
            {
                toLower = StringShare::tryShare(toLower);

                if(id != YearColumn && id != CommentColumn && sharedData()->metadata[id] != toLower) {
                    CollectionList::instance()->removeStringFromDict(sharedData()->metadata[id], id);
                    CollectionList::instance()->addStringToDict(text(i), id);
                }
            }

            sharedData()->metadata[id] = toLower;
        }

        int newWidth = treeWidget()->fontMetrics().width(text(i));
        if(newWidth != sharedData()->cachedWidths[i])
            playlist()->slotWeightDirty(i);

        sharedData()->cachedWidths[i] = newWidth;
    }

    for(PlaylistItemList::Iterator it = m_children.begin(); it != m_children.end(); ++it) {
        (*it)->playlist()->update();
        (*it)->playlist()->dataChanged();
    }
    if(treeWidget()->isVisible())
        treeWidget()->viewport()->update();

    CollectionList::instance()->dataChanged();
    emit CollectionList::instance()->signalCollectionChanged();
}

PlaylistItem *CollectionListItem::itemForPlaylist(const Playlist *playlist)
{
    if(playlist == CollectionList::instance())
        return this;

    PlaylistItemList::ConstIterator it;
    for(it = m_children.constBegin(); it != m_children.constEnd(); ++it)
        if((*it)->playlist() == playlist)
            return *it;
    return 0;
}

void CollectionListItem::updateCollectionDict(const QString &oldPath, const QString &newPath)
{
    CollectionList *collection = CollectionList::instance();

    if(!collection)
        return;

    collection->removeFromDict(oldPath);
    collection->addToDict(newPath, this);
}

void CollectionListItem::repaint() const
{
    // FIXME repaint
    /*QItemDelegate::repaint();
    for(PlaylistItemList::ConstIterator it = m_children.constBegin(); it != m_children.constEnd(); ++it)
        (*it)->repaint();*/
}

////////////////////////////////////////////////////////////////////////////////
// CollectionListItem protected methods
////////////////////////////////////////////////////////////////////////////////

CollectionListItem::CollectionListItem(CollectionList *parent, const FileHandle &file) :
    PlaylistItem(parent),
    m_shuttingDown(false)
{
    parent->addToDict(file.absFilePath(), this);

    sharedData()->fileHandle = file;

    if(file.tag()) {
        refresh();
        parent->dataChanged();
    }
    else {
        kError() << "CollectionListItem::CollectionListItem() -- Tag() could not be created." << endl;
    }

    SplashScreen::increment();
}

CollectionListItem::~CollectionListItem()
{
    m_shuttingDown = true;

    foreach(PlaylistItem *item, m_children)
        delete item;

    CollectionList *l = CollectionList::instance();
    if(l) {
        l->removeFromDict(file().absFilePath());
        l->removeStringFromDict(file().tag()->album(), AlbumColumn);
        l->removeStringFromDict(file().tag()->artist(), ArtistColumn);
        l->removeStringFromDict(file().tag()->genre(), GenreColumn);
    }
}

void CollectionListItem::addChildItem(PlaylistItem *child)
{
    m_children.append(child);
}

void CollectionListItem::removeChildItem(PlaylistItem *child)
{
    if(!m_shuttingDown)
        m_children.removeAll(child);
}

bool CollectionListItem::checkCurrent()
{
    if(!file().fileInfo().exists() || !file().fileInfo().isFile())
        return false;

    if(!file().current()) {
        file().refresh();
        refresh();
    }

    return true;
}

// vim: set et sw=4 tw=0 sta:
