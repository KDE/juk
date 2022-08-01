/**
 * Copyright (C) 2002-2004 Scott Wheeler <wheeler@kde.org>
 * Copyright (C) 2008-2018 Michael Pyne <mpyne@kde.org>
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

#include "playlist.h"
#include "juk-exception.h"

#include <KLocalizedString>
#include <KSharedConfig>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kio/copyjob.h>
#include <kactioncollection.h>
#include <kconfiggroup.h>
#include <ktoolbarpopupaction.h>
#include <kactionmenu.h>
#include <ktoggleaction.h>

#include <QCursor>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QHash>
#include <QToolTip>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QList>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QMenu>
#include <QTimer>
#include <QClipboard>
#include <QTextStream>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QPixmap>
#include <QSet>
#include <QStackedWidget>
#include <QScrollBar>
#include <QPainter>
#include <QActionGroup>

#include <QtConcurrent>
#include <QFutureWatcher>

#include <id3v1genres.h>

#include <time.h>
#include <cmath>
#include <algorithm>
#include <random>
#include <utility>

#include "actioncollection.h"
#include "cache.h"
#include "collectionlist.h"
#include "coverdialog.h"
#include "coverinfo.h"
#include "deletedialog.h"
#include "directoryloader.h"
#include "filerenamer.h"
#include "iconsupport.h"
#include "juk_debug.h"
#include "juktag.h"
#include "mediafiles.h"
#include "playlistcollection.h"
#include "playlistitem.h"
#include "playlistsearch.h"
#include "playlistsharedsettings.h"
#include "tagtransactionmanager.h"
#include "upcomingplaylist.h"
#include "webimagefetcher.h"

using namespace ActionCollection; // ""_act and others

/**
 * Used to give every track added in the program a unique identifier. See
 * PlaylistItem
 */
quint32 g_trackID = 0;

/**
 * Just a shortcut of sorts.
 */

static bool manualResize()
{
    return "resizeColumnsManually"_act->isChecked();
}

////////////////////////////////////////////////////////////////////////////////
// static members
////////////////////////////////////////////////////////////////////////////////

bool                    Playlist::m_visibleChanged = false;
bool                    Playlist::m_shuttingDown   = false;
PlaylistItemList        Playlist::m_history;
QVector<PlaylistItem *> Playlist::m_backMenuItems;
int                     Playlist::m_leftColumn     = 0;

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

Playlist::Playlist(
        bool delaySetup, const QString &name,
        PlaylistCollection *collection, const QString &iconName,
        int extraCols)
  : QTreeWidget(collection->playlistStack())
  , m_collection(collection)
  , m_playlistName(name)
  , m_refillDebounce(new QTimer(this))
  , m_fetcher(new WebImageFetcher(this))
{
    setup(extraCols);

    // The timer soaks up repeated events that may cause the random play list
    // to be regenerated repeatedly.
    m_refillDebounce->setInterval(100);
    m_refillDebounce->setSingleShot(true);
    connect(m_refillDebounce, &QTimer::timeout,
            this,             &Playlist::refillRandomList);

    // Any of the random-related actions being triggered will cause the parent
    // group to emit the triggered signal.
    QActionGroup *randomGroup = action("disableRandomPlay")->actionGroup();
    connect(randomGroup,      &QActionGroup::triggered,
            m_refillDebounce, qOverload<>(&QTimer::start));

    // Some subclasses need to do even more handling but will remember to
    // call setupPlaylist
    if(!delaySetup) {
        collection->setupPlaylist(this, iconName);
    }
}

Playlist::Playlist(PlaylistCollection *collection, const QString &name,
                   const QString &iconName)
    : Playlist(false, name, collection, iconName, 0)
{
}

Playlist::Playlist(PlaylistCollection *collection, const PlaylistItemList &items,
                   const QString &name, const QString &iconName)
    : Playlist(false, name, collection, iconName, 0)
{
    createItems(items);
}

Playlist::Playlist(PlaylistCollection *collection, const QFileInfo &playlistFile,
                   const QString &iconName)
    : Playlist(true, QString(), collection, iconName, 0)
{
    m_fileName = playlistFile.canonicalFilePath();

    // Load the file after construction completes so that virtual methods in
    // subclasses can take effect.
    QTimer::singleShot(0, this, [=]() {
        loadFile(m_fileName, playlistFile);
        collection->setupPlaylist(this, iconName);
    });
}

Playlist::Playlist(PlaylistCollection *collection, bool delaySetup, int extraColumns)
    : Playlist(delaySetup, QString(), collection, QStringLiteral("audio-midi"), extraColumns)
{
}

Playlist::~Playlist()
{
    // clearItem() will take care of removing the items from the history,
    // so call clearItems() to make sure it happens.
    //
    // Some subclasses override clearItems and items so we manually dispatch to
    // make clear that it's intentional that those subclassed versions don't
    // get called (because we can't call them)

    m_randomSequence.clear();
    Playlist::clearItems(Playlist::items());

    if(!m_shuttingDown)
        m_collection->removePlaylist(this);
}

QString Playlist::name() const
{
    if(m_playlistName.isEmpty())
        return m_fileName.section(QDir::separator(), -1).section('.', 0, -2);
    else
        return m_playlistName;
}

FileHandle Playlist::currentFile() const
{
    return playingItem() ? playingItem()->file() : FileHandle();
}

void Playlist::playFirst()
{
    QTreeWidgetItemIterator listIt(const_cast<Playlist *>(this), QTreeWidgetItemIterator::NotHidden);
    beginPlayingItem(static_cast<PlaylistItem *>(*listIt));
    refillRandomList();
}

void Playlist::playNextAlbum()
{
    const auto &item = playingItem();
    if(!item || !action("albumRandomPlay")->isChecked()) {
        playNext();
        return;
    }

    const auto currentAlbum = item->file().tag()->album();
    const auto nextAlbumTrack = std::find_if(m_randomSequence.begin(), m_randomSequence.end(),
            [currentAlbum](const PlaylistItem *item) {
                return item->file().tag()->album() != currentAlbum;
            });

    if(nextAlbumTrack == m_randomSequence.end()) {
        // We were on the last album, playNext will handle looping if we should loop
        m_randomSequence.clear();
        playNext();
    }
    else {
        m_randomSequence.erase(m_randomSequence.begin(), nextAlbumTrack);
        beginPlayingItem(*nextAlbumTrack);
    }
}

void Playlist::playNext()
{
    PlaylistItem *next = nullptr;
    auto nowPlaying = playingItem();
    bool doLoop = action("loopPlaylist")->isChecked();

    // Treat an item from a different playlist as if we were being asked to
    // play from a stop
    if(nowPlaying && nowPlaying->playlist() != this) {
        nowPlaying = nullptr;
    }

    if(action("disableRandomPlay")->isChecked()) {
        QTreeWidgetItemIterator listIt = nowPlaying
            ? QTreeWidgetItemIterator(nowPlaying, QTreeWidgetItemIterator::NotHidden)
            : QTreeWidgetItemIterator(this,       QTreeWidgetItemIterator::NotHidden);

        if(*listIt && nowPlaying) {
            ++listIt;
        }

        next = static_cast<PlaylistItem *>(*listIt);

        if(!next && doLoop) {
            playFirst();
            return;
        }
    }
    else {
        // The two random play modes are identical here, the difference is in how the
        // randomized sequence is generated by refillRandomList

        if(m_randomSequence.isEmpty() && (doLoop || !nowPlaying)) {
            refillRandomList();

            // Don't play the same track twice in a row even if it can
            // "randomly" happen
            if(m_randomSequence.front() == nowPlaying) {
                std::swap(m_randomSequence.front(), m_randomSequence.back());
            }
        }

        if(!m_randomSequence.isEmpty()) {
            next = m_randomSequence.takeFirst();
        }
    }

    // Will stop playback if next is still null
    beginPlayingItem(next);
}

void Playlist::stop()
{
    m_history.clear();
    setPlaying(nullptr);
}

void Playlist::playPrevious()
{
    if(!playingItem())
        return;

    bool random = action("randomPlay") && action<KToggleAction>("randomPlay")->isChecked();

    PlaylistItem *previous = nullptr;

    if(random && !m_history.isEmpty()) {
        PlaylistItemList::Iterator last = m_history.end() - 1;
        previous = *last;
        m_history.erase(last);
    }
    else {
        m_history.clear();
        QTreeWidgetItemIterator listIt(playingItem(), QTreeWidgetItemIterator::NotHidden);
        previous = static_cast<PlaylistItem *>(*--listIt);
    }

    beginPlayingItem(previous);
}

void Playlist::setName(const QString &n)
{
    m_collection->addNameToDict(n);
    m_collection->removeNameFromDict(m_playlistName);

    m_playlistName = n;
    emit signalNameChanged(m_playlistName);
}

void Playlist::save()
{
    if(m_fileName.isEmpty())
        return saveAs();

    QFile file(m_fileName);

    if(!file.open(QIODevice::WriteOnly))
        return KMessageBox::error(this, i18n("Could not save to file %1.", m_fileName));

    QTextStream stream(&file);

    const QStringList fileList = files();

    for(const auto &file : fileList) {
        stream << file << '\n';
    }

    file.close();
}

void Playlist::saveAs()
{
    m_collection->removeFileFromDict(m_fileName);

    m_fileName = MediaFiles::savePlaylistDialog(name(), this);

    if(!m_fileName.isEmpty()) {
        m_collection->addFileToDict(m_fileName);

        // If there's no playlist name set, use the file name.
        if(m_playlistName.isEmpty())
            emit signalNameChanged(name());
        save();
    }
}

void Playlist::updateDeletedItem(PlaylistItem *item)
{
    m_members.remove(item->file().absFilePath());
    m_randomSequence.removeAll(item);
    m_history.removeAll(item);
}

void Playlist::clearItem(PlaylistItem *item)
{
    // Automatically updates internal structs via updateDeletedItem
    delete item;

    playlistItemsChanged();
}

void Playlist::clearItems(const PlaylistItemList &items)
{
    for(auto &item : items) {
        delete item;
    }
    playlistItemsChanged();
}

PlaylistItem *Playlist::playingItem() // static
{
    return PlaylistItem::playingItems().isEmpty()
        ? nullptr
        : PlaylistItem::playingItems().front();
}

QStringList Playlist::files() const
{
    QStringList list;

    for(QTreeWidgetItemIterator it(const_cast<Playlist *>(this)); *it; ++it)
        list.append(static_cast<PlaylistItem *>(*it)->file().absFilePath());

    return list;
}

PlaylistItemList Playlist::items()
{
    return items(QTreeWidgetItemIterator::IteratorFlag(0));
}

PlaylistItemList Playlist::visibleItems()
{
    return items(QTreeWidgetItemIterator::NotHidden);
}

PlaylistItemList Playlist::selectedItems()
{
    return items(QTreeWidgetItemIterator::Selected | QTreeWidgetItemIterator::NotHidden);
}

PlaylistItem *Playlist::firstChild() const
{
    return static_cast<PlaylistItem *>(topLevelItem(0));
}

void Playlist::updateLeftColumn()
{
    int newLeftColumn = leftMostVisibleColumn();

    if(m_leftColumn != newLeftColumn) {
        updatePlaying();
        m_leftColumn = newLeftColumn;
    }
}

void Playlist::setItemsVisible(const QModelIndexList &indexes, bool visible) // static
{
    m_visibleChanged = true;

    for(QModelIndex index : indexes)
        itemFromIndex(index)->setHidden(!visible);
}

void Playlist::setSearch(PlaylistSearch* s)
{
    m_search = s;

    if(!m_searchEnabled)
        return;

    for(int row = 0; row < topLevelItemCount(); ++row)
        topLevelItem(row)->setHidden(true);
    setItemsVisible(s->matchedItems(), true);
}

void Playlist::setSearchEnabled(bool enabled)
{
    if(m_searchEnabled == enabled)
        return;

    m_searchEnabled = enabled;

    if(enabled) {
        for(int row = 0; row < topLevelItemCount(); ++row)
            topLevelItem(row)->setHidden(true);
        setItemsVisible(m_search->matchedItems(), true);
    }
    else {
        const auto &playlistItems = items();
        for(PlaylistItem* item : playlistItems)
            item->setHidden(false);
    }
}

// Mostly seems to be for DynamicPlaylist
// TODO: See if this can't all be eliminated by making 'is-playing' a predicate
// of the playlist item itself
void Playlist::synchronizePlayingItems(Playlist *playlist, bool setMaster)
{
    if(!playlist || !playlist->playing())
        return;

    CollectionListItem *base = playingItem()->collectionItem();
    for(QTreeWidgetItemIterator itemIt(playlist); *itemIt; ++itemIt) {
        PlaylistItem *item = static_cast<PlaylistItem *>(*itemIt);
        if(base == item->collectionItem()) {
            item->setPlaying(true, setMaster);
            return;
        }
    }
}

void Playlist::synchronizePlayingItems(const PlaylistList &sources, bool setMaster)
{
    for(auto p : sources) {
        synchronizePlayingItems(p, setMaster);
    }
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::copy()
{
    const PlaylistItemList items = selectedItems();
    QList<QUrl> urls;

    for(const auto &item : items) {
        urls << QUrl::fromLocalFile(item->file().absFilePath());
    }

    QMimeData *mimeData = new QMimeData;
    mimeData->setUrls(urls);

    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);
}

void Playlist::paste()
{
    addFilesFromMimeData(
        QApplication::clipboard()->mimeData(),
        static_cast<PlaylistItem *>(currentItem()));
}

void Playlist::clear()
{
    PlaylistItemList l = selectedItems();
    if(l.isEmpty())
        l = items();

    clearItems(l);
}

void Playlist::slotRefresh()
{
    PlaylistItemList itemList = selectedItems();
    if(itemList.isEmpty())
        itemList = visibleItems();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    for(auto &item : itemList) {
        item->refreshFromDisk();

        if(!item->file().tag() || !item->file().fileInfo().exists()) {
            qCDebug(JUK_LOG) << "Error while trying to refresh the tag.  "
                           << "This file has probably been removed.";
            delete item->collectionItem();
        }

        processEvents();
    }
    QApplication::restoreOverrideCursor();
}

void Playlist::slotOpenItemDir()
{
    PlaylistItemList itemList = selectedItems();
    QList<QUrl> pathList;

    for(auto &item : itemList) {
        QUrl path = QUrl::fromLocalFile(item->file().fileInfo().absoluteDir().absolutePath());
        if(!pathList.contains(path))
            pathList.append(path);
    }

    if (pathList.length() > 4) {
        if(KMessageBox::warningContinueCancel(
            this,
            i18np("You are about to open directory. Are you sure you want to continue?",
                  "You are about to open %1 directories. Are you sure you want to continue?",
                  pathList.length()),
            i18n("Open Containing Folder")
        ) == KMessageBox::Cancel)
        {
            return;
        }
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    for(auto &path : pathList) {
        QDesktopServices::openUrl(path);

        processEvents();
    }
    QApplication::restoreOverrideCursor();
}

void Playlist::slotRenameFile()
{
    FileRenamer renamer;
    PlaylistItemList items = selectedItems();

    if(items.isEmpty())
        return;

    emit signalEnableDirWatch(false);

    m_blockDataChanged = true;
    renamer.rename(items);
    m_blockDataChanged = false;
    playlistItemsChanged();

    emit signalEnableDirWatch(true);
}

void Playlist::slotBeginPlayback()
{
    QTreeWidgetItemIterator visible(this, QTreeWidgetItemIterator::NotHidden);
    PlaylistItem *item = static_cast<PlaylistItem *>(*visible);

    if(item) {
        refillRandomList();
        playNext();
    }
    else {
        action("stop")->trigger();
    }
}

void Playlist::slotViewCover()
{
    const PlaylistItemList items = selectedItems();
    for(const auto &item : items) {
        const auto cover = item->file().coverInfo();

        if(cover->hasCover()) {
            cover->popup();
            return; // If we select multiple items, only show one
        }
    }
}

void Playlist::slotRemoveCover()
{
    PlaylistItemList items = selectedItems();
    if(items.isEmpty())
        return;
    int button = KMessageBox::warningContinueCancel(this,
                                                    i18n("Are you sure you want to delete these covers?"),
                                                    QString(),
                                                    KGuiItem(i18n("&Delete Covers")));
    if(button == KMessageBox::Continue)
        refreshAlbums(items);
}

void Playlist::slotShowCoverManager()
{
    static CoverDialog *managerDialog = 0;

    if(!managerDialog)
        managerDialog = new CoverDialog(this);

    managerDialog->show();
}

void Playlist::slotAddCover(bool retrieveLocal)
{
    PlaylistItemList items = selectedItems();

    if(items.isEmpty())
        return;

    if(!retrieveLocal) {
        m_fetcher->setFile((*items.begin())->file());
        m_fetcher->searchCover();
        return;
    }

    QUrl file = QFileDialog::getOpenFileUrl(
        this, i18n("Select Cover Image File"),
        QUrl::fromLocalFile(QDir::home().path()),
        i18n("Images (*.png *.jpg)"), nullptr,
        {}, QStringList() << QStringLiteral("file")
        );
    if(file.isEmpty())
        return;

    QString artist = items.front()->file().tag()->artist();
    QString album = items.front()->file().tag()->album();

    coverKey newId = CoverManager::addCover(file, artist, album);

    if(newId != CoverManager::NoMatch)
        refreshAlbums(items, newId);
}

// Called when image fetcher has added a new cover.
void Playlist::slotCoverChanged(int coverId)
{
    qCDebug(JUK_LOG) << "Refreshing information for newly changed covers.";
    refreshAlbums(selectedItems(), coverId);
}

void Playlist::slotGuessTagInfo(TagGuesser::Type type)
{
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const PlaylistItemList items = selectedItems();
    setDynamicListsFrozen(true);

    m_blockDataChanged = true;

    for(auto &item : items) {
        item->guessTagInfo(type);
        processEvents();
    }

    // MusicBrainz queries automatically commit at this point.  What would
    // be nice is having a signal emitted when the last query is completed.

    if(type == TagGuesser::FileName)
        TagTransactionManager::instance()->commit();

    m_blockDataChanged = false;

    playlistItemsChanged();
    setDynamicListsFrozen(false);
    QApplication::restoreOverrideCursor();
}

void Playlist::slotReload()
{
    QFileInfo fileInfo(m_fileName);
    if(!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable())
        return;

    clearItems(items());
    loadFile(m_fileName, fileInfo);
}

void Playlist::refillRandomList()
{
    qCDebug(JUK_LOG) << "Refilling random items.";

    if(action("disableRandomPlay")->isChecked()) {
        m_randomSequence.clear();
        return;
    }

    PlaylistItemList randomItems = visibleItems();

    // See https://www.pcg-random.org/posts/cpp-seeding-surprises.html
    std::random_device rdev;
    uint64_t rseed = (uint64_t(rdev()) << 32) | rdev();
    std::linear_congruential_engine<
        uint64_t, 6364136223846793005U, 1442695040888963407U, 0U
        > knuth_lcg(rseed);

    std::shuffle(randomItems.begin(), randomItems.end(), knuth_lcg);

    if(action("albumRandomPlay")->isChecked()) {
        std::sort(randomItems.begin(), randomItems.end(),
            [](PlaylistItem *a, PlaylistItem *b) {
                return a->file().tag()->album() < b->file().tag()->album();
            });

        // If there is an item playing from our playlist already, move its
        // album to the front

        const auto wasPlaying = playingItem();
        if(wasPlaying && wasPlaying->playlist() == this) {
            const auto playingAlbum = wasPlaying->file().tag()->album();
            std::stable_partition(randomItems.begin(), randomItems.end(),
                [playingAlbum](const PlaylistItem *item) {
                    return item->file().tag()->album() == playingAlbum;
                });
        }
    }

    std::swap(m_randomSequence, randomItems);
}

void Playlist::slotWeightDirty(int column)
{
    if(column < 0) {
        m_weightDirty.clear();
        for(int i = 0; i < columnCount(); i++) {
            if(!isColumnHidden(i))
                m_weightDirty.append(i);
        }
        return;
    }

    if(!m_weightDirty.contains(column))
        m_weightDirty.append(column);
}

void Playlist::slotShowPlaying()
{
    if(!playingItem())
        return;

    Playlist *l = playingItem()->playlist();

    l->clearSelection();

    // Raise the playlist before selecting the items otherwise the tag editor
    // will not update when it gets the selectionChanged() notification
    // because it will think the user is choosing a different playlist but not
    // selecting a different item.

    m_collection->raise(l);

    l->setCurrentItem(playingItem());
    l->scrollToItem(playingItem(), QAbstractItemView::PositionAtCenter);
}

void Playlist::slotColumnResizeModeChanged()
{
    if(manualResize()) {
        header()->setSectionResizeMode(QHeaderView::Interactive);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        header()->setSectionResizeMode(QHeaderView::Fixed);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    if(!manualResize())
        slotUpdateColumnWidths();

    SharedSettings::instance()->sync();
}

void Playlist::playlistItemsChanged()
{
    if(m_blockDataChanged)
        return;
    PlaylistInterface::playlistItemsChanged();
}

////////////////////////////////////////////////////////////////////////////////
// protected members
////////////////////////////////////////////////////////////////////////////////

void Playlist::removeFromDisk(const PlaylistItemList &items)
{
    if(!isVisible() || items.isEmpty()) {
        return;
    }

    QStringList files;
    for(const auto &item : items) {
        files.append(item->file().absFilePath());
    }

    DeleteDialog dialog(this);

    m_blockDataChanged = true;

    if(dialog.confirmDeleteList(files)) {
        bool shouldDelete = dialog.shouldDelete();
        QStringList errorFiles;

        for(const auto &item : items) {
            if(playingItem() == item)
                action("forward")->trigger();

            QString removePath = item->file().absFilePath();
            QUrl removeUrl = QUrl::fromLocalFile(removePath);
            if((!shouldDelete && KIO::trash(removeUrl)->exec()) ||
               (shouldDelete && QFile::remove(removePath)))
            {
                delete item->collectionItem();
            }
            else
                errorFiles.append(item->file().absFilePath());
        }

        if(!errorFiles.isEmpty()) {
            QString errorMsg = shouldDelete ?
                    i18n("Could not delete these files") :
                    i18n("Could not move these files to the Trash");
            KMessageBox::errorList(this, errorMsg, errorFiles);
        }
    }

    m_blockDataChanged = false;

    playlistItemsChanged();
}

void Playlist::synchronizeItemsTo(const PlaylistItemList &itemList)
{
    // direct call to ::items to avoid infinite loop, bug 402355
    m_randomSequence.clear();
    clearItems(Playlist::items());
    createItems(itemList);
}

void Playlist::beginPlayingItem(PlaylistItem *itemToPlay)
{
    if(itemToPlay) {
        setPlaying(itemToPlay, true);
        m_collection->requestPlaybackFor(itemToPlay->file());
    }
    else {
        setPlaying(nullptr);
        action("stop")->trigger();
    }
}

void Playlist::dragEnterEvent(QDragEnterEvent *e)
{
    if(CoverDrag::isCover(e->mimeData())) {
        setDropIndicatorShown(false);
        e->accept();
        return;
    }

    if(e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty()) {
        setDropIndicatorShown(true);
        e->acceptProposedAction();
    }
    else
        e->ignore();
}

void Playlist::addFilesFromMimeData(const QMimeData *urls, PlaylistItem *after)
{
    if(!urls->hasUrls()) {
        return;
    }

    addFiles(QUrl::toStringList(urls->urls(), QUrl::PreferLocalFile), after);
}

bool Playlist::eventFilter(QObject *watched, QEvent *e)
{
    if(watched == header()) {
        switch(e->type()) {
        case QEvent::MouseMove:
        {
            if((static_cast<QMouseEvent *>(e)->modifiers() & Qt::LeftButton) == Qt::LeftButton &&
                !action<KToggleAction>("resizeColumnsManually")->isChecked())
            {
                m_columnWidthModeChanged = true;

                action<KToggleAction>("resizeColumnsManually")->setChecked(true);
                slotColumnResizeModeChanged();
            }

            break;
        }
        case QEvent::MouseButtonPress:
        {
            if(static_cast<QMouseEvent *>(e)->button() == Qt::RightButton)
                m_headerMenu->popup(QCursor::pos());

            break;
        }
        case QEvent::MouseButtonRelease:
        {
            if(m_columnWidthModeChanged) {
                m_columnWidthModeChanged = false;
                notifyUserColumnWidthModeChanged();
            }

            if(!manualResize() && m_widthsDirty)
                QTimer::singleShot(0, this, &Playlist::slotUpdateColumnWidths);
            break;
        }
        default:
            break;
        }
    }

    return QTreeWidget::eventFilter(watched, e);
}

void Playlist::keyPressEvent(QKeyEvent *event)
{
    if(event->key() == Qt::Key_Up) {
        if(const auto activeItem = currentItem()) {
            QTreeWidgetItemIterator visible(this, QTreeWidgetItemIterator::NotHidden);
            if(activeItem == *visible) {
                emit signalMoveFocusAway();
                event->accept();
            }
        }
    }
    else if(event->key() == Qt::Key_Return && !event->isAutoRepeat()) {
        event->accept();
        slotPlayCurrent();
        return; // event completely handled already
    }

    QTreeWidget::keyPressEvent(event);
}

QStringList Playlist::mimeTypes() const
{
    return QStringList("text/uri-list");
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
QMimeData* Playlist::mimeData(const QList<QTreeWidgetItem *> items) const
#else
QMimeData* Playlist::mimeData(const QList<QTreeWidgetItem *> &items) const
#endif
{
    QList<QUrl> urls;
    for(const auto &item : items) {
        urls << QUrl::fromLocalFile(static_cast<const PlaylistItem*>(item)->file().absFilePath());
    }

    QMimeData *urlDrag = new QMimeData();
    urlDrag->setUrls(urls);

    return urlDrag;
}

bool Playlist::dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action)
{
    // TODO: Re-add DND
    Q_UNUSED(parent);
    Q_UNUSED(index);
    Q_UNUSED(data);
    Q_UNUSED(action);

    return false;
}

void Playlist::dropEvent(QDropEvent *e)
{
    QPoint vp = e->pos();
    PlaylistItem *item = static_cast<PlaylistItem *>(itemAt(vp));

    // First see if we're dropping a cover, if so we can get it out of the
    // way early.
    if(item && CoverDrag::isCover(e->mimeData())) {
        coverKey id = CoverDrag::idFromData(e->mimeData());

        // If the item we dropped on is selected, apply cover to all selected
        // items, otherwise just apply to the dropped item.

        if(item->isSelected()) {
            const PlaylistItemList selItems = selectedItems();
            for(auto &playlistItem : selItems) {
                playlistItem->file().coverInfo()->setCoverId(id);
                playlistItem->refresh();
            }
        }
        else {
            item->file().coverInfo()->setCoverId(id);
            item->refresh();
        }

        return;
    }

    // When dropping on the toUpper half of an item, insert before this item.
    // This is what the user expects, and also allows the insertion at
    // top of the list

    QRect rect = visualItemRect(item);
    if(!item)
        item = static_cast<PlaylistItem *>(topLevelItem(topLevelItemCount() - 1));
    else if(vp.y() < rect.y() + rect.height() / 2)
        item = static_cast<PlaylistItem *>(item->itemAbove());

    m_blockDataChanged = true;

    if(e->source() == this) {

        // Since we're trying to arrange things manually, turn off sorting.

        sortItems(columnCount() + 1, Qt::AscendingOrder);

        const QList<QTreeWidgetItem *> items = QTreeWidget::selectedItems();
        int insertIndex = item ? indexOfTopLevelItem(item) : 0;

        // Move items from elsewhere in the playlist

        for(auto &listViewItem : items) {
            auto oldItem = takeTopLevelItem(indexOfTopLevelItem(listViewItem));
            insertTopLevelItem(++insertIndex, oldItem);
        }
    }
    else
        addFilesFromMimeData(e->mimeData(), item);

    m_blockDataChanged = false;

    playlistItemsChanged();
    emit signalPlaylistItemsDropped(this);
    QTreeWidget::dropEvent(e);
}

void Playlist::showEvent(QShowEvent *e)
{
    if(m_applySharedSettings) {
        SharedSettings::instance()->apply(this);
        m_applySharedSettings = false;
    }

    QTreeWidget::showEvent(e);
}

void Playlist::applySharedSettings()
{
    m_applySharedSettings = true;
}

void Playlist::read(QDataStream &s)
{
    s >> m_playlistName
      >> m_fileName;

    // m_fileName is probably empty.
    if(m_playlistName.isEmpty())
        throw BICStreamException();

    // Do not sort. Add the files in the order they were saved.
    setSortingEnabled(false);

    QStringList files;
    s >> files;

    QTreeWidgetItem *after = 0;
    const CollectionList *clInst = CollectionList::instance();

    m_blockDataChanged = true;

    for(const auto &file : qAsConst(files)) {
        if(file.isEmpty())
            throw BICStreamException();

        const auto &cItem = clInst->lookup(file);
        if(cItem) {
            // Reuse FileHandle so the playlist doesn't force TagLib to read it
            // from disk
            after = createItem(cItem->file(), after);
        }
        else {
            after = createItem(FileHandle(file), after);
        }
    }

    m_blockDataChanged = false;

    playlistItemsChanged();
    m_collection->setupPlaylist(this, "audio-midi");
}

void Playlist::paintEvent(QPaintEvent *pe)
{
    // If there are columns that need to be updated, well, update them.

    if(!m_weightDirty.isEmpty() && !manualResize())
    {
        calculateColumnWeights();
        slotUpdateColumnWidths();
    }

    QTreeWidget::paintEvent(pe);
}

void Playlist::resizeEvent(QResizeEvent *re)
{
    // If the width of the view has changed, manually update the column
    // widths.

    if(re->size().width() != re->oldSize().width() && !manualResize())
        slotUpdateColumnWidths();

    QTreeWidget::resizeEvent(re);
}

// Reimplemented to show a visual indication of which of the view's playlist
// items is actually playing.
void Playlist::drawRow(QPainter *p, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    PlaylistItem *item = static_cast<PlaylistItem *>(itemFromIndex(index));
    if(Q_LIKELY(!PlaylistItem::playingItems().contains(item))) {
        return QTreeWidget::drawRow(p, option, index);
    }

    // Seems that the view draws the background now so we have to do this
    // manually
    p->fillRect(option.rect, QPalette{}.midlight());

    QStyleOptionViewItem newOption {option};
    newOption.font.setBold(true);

    QTreeWidget::drawRow(p, newOption, index);
}

void Playlist::insertItem(QTreeWidgetItem *item)
{
    QTreeWidget::insertTopLevelItem(0, item);
}

void Playlist::takeItem(QTreeWidgetItem *item)
{
    int index = indexOfTopLevelItem(item);
    QTreeWidget::takeTopLevelItem(index);
}

PlaylistItem *Playlist::createItem(const FileHandle &file, QTreeWidgetItem *after)
{
    return createItem<PlaylistItem>(file, after);
}

void Playlist::createItems(const PlaylistItemList &siblings, PlaylistItem *after)
{
    createItems<QVector, PlaylistItem, PlaylistItem>(siblings, after);
}

void Playlist::addFiles(const QStringList &files, PlaylistItem *after)
{
    if(Q_UNLIKELY(files.isEmpty())) {
        return;
    }

    m_blockDataChanged = true;
    setEnabled(false);

    QVector<QFuture<void>> pendingFutures;
    for(const auto &file : files) {
        // some files added here will launch threads that we must wait until
        // they're done to cleanup
        auto pendingResult = addUntypedFile(file, after);
        if(!pendingResult.isFinished()) {
            pendingFutures.push_back(pendingResult);
            ++m_itemsLoading;
        }
    }

    // It's possible for no async threads to be launched, and also possible
    // for this function to be called while there were other threads in flight
    if(pendingFutures.isEmpty() && m_itemsLoading == 0) {
        cleanupAfterAllFileLoadsCompleted();
        return;
    }

    // Build handlers for all the still-active loaders on the heap and then
    // return to the event loop.
    for(const auto &future : qAsConst(pendingFutures)) {
        auto loadWatcher = new QFutureWatcher<void>(this);
        loadWatcher->setFuture(future);

        connect(loadWatcher, &QFutureWatcher<void>::finished, this, [=]() {
                if(--m_itemsLoading == 0) {
                    cleanupAfterAllFileLoadsCompleted();
                }

                loadWatcher->deleteLater();
            });
    }
}

void Playlist::refreshAlbums(const PlaylistItemList &items, coverKey id)
{
    QSet<QPair<QString, QString>> albums;
    bool setAlbumCovers = items.count() == 1;

    for(const auto &item : items) {
        const QString artist = item->file().tag()->artist();
        const QString album = item->file().tag()->album();

        albums.insert(qMakePair(artist, album));

        item->file().coverInfo()->setCoverId(id);
        if(setAlbumCovers)
            item->file().coverInfo()->applyCoverToWholeAlbum(true);
    }

    for(const auto &albumPair : qAsConst(albums)) {
        refreshAlbum(albumPair.first, albumPair.second);
    }
}

void Playlist::updatePlaying() const
{
    const auto playingItems = PlaylistItem::playingItems();
    for(const auto &item : playingItems) {
        item->treeWidget()->viewport()->update();
    }
}

void Playlist::refreshAlbum(const QString &artist, const QString &album)
{
    ColumnList columns;
    columns.append(PlaylistItem::ArtistColumn);
    PlaylistSearch::Component artistComponent(artist, false, columns,
                                              PlaylistSearch::Component::Exact);

    columns.clear();
    columns.append(PlaylistItem::AlbumColumn);
    PlaylistSearch::Component albumComponent(album, false, columns,
                                             PlaylistSearch::Component::Exact);

    PlaylistSearch::ComponentList components;
    components.append(artist);
    components.append(album);

    PlaylistList playlists;
    playlists.append(CollectionList::instance());

    PlaylistSearch search(playlists, components);
    const QModelIndexList matches = search.matchedItems();

    for(QModelIndex index: matches)
        static_cast<PlaylistItem*>(itemFromIndex(index))->refresh();
}

void Playlist::hideColumn(int c, bool updateSearch)
{
    const auto headerActions = m_headerMenu->actions();
    for(auto &action : headerActions) {
        if(!action)
            continue;

        if(action->data().toInt() == c) {
            action->setChecked(false);
            break;
        }
    }

    if(isColumnHidden(c)) {
        return;
    }

    QTreeWidget::hideColumn(c);

    if(c == m_leftColumn) {
        updatePlaying();
        m_leftColumn = leftMostVisibleColumn();
    }

    if(!manualResize()) {
        slotUpdateColumnWidths();
        viewport()->update();
    }

    if(this != CollectionList::instance()) {
        CollectionList::instance()->hideColumn(c, false);
    }

    if(updateSearch) {
        redisplaySearch();
    }
}

void Playlist::showColumn(int c, bool updateSearch)
{
    const auto headerActions = m_headerMenu->actions();
    for(auto &action : headerActions) {
        if(!action)
            continue;

        if(action->data().toInt() == c) {
            action->setChecked(true);
            break;
        }
    }

    if(!isColumnHidden(c)) {
        return;
    }

    QTreeWidget::showColumn(c);

    if(c == leftMostVisibleColumn()) {
        updatePlaying();
        m_leftColumn = leftMostVisibleColumn();
    }

    if(!manualResize()) {
        slotUpdateColumnWidths();
        viewport()->update();
    }

    if(this != CollectionList::instance()) {
        CollectionList::instance()->showColumn(c, false);
    }

    if(updateSearch) {
        redisplaySearch();
    }
}

void Playlist::sortByColumn(int column, Qt::SortOrder order)
{
    setSortingEnabled(true);
    QTreeWidget::sortByColumn(column, order);
}

// This function is called during startup so it cannot rely on any virtual
// functions that might be changed by a subclass (virtual functions relying on
// superclasses are fine since the C++ runtime can statically dispatch those).
void Playlist::slotInitialize(int numColumnsToReserve)
{
    // Setup the columns in the list view. We set aside room for
    // subclass-specific extra columns (always added at the beginning, see
    // columnOffset()) and then supplement with columns that apply to every
    // playlist.
    const QStringList standardColHeaders = {
        i18n("Track Name"),
        i18n("Artist"),
        i18n("Album"),
        i18n("Cover"),
        i18nc("cd track number", "Track"),
        i18n("Genre"),
        i18n("Year"),
        i18n("Length"),
        i18n("Bitrate"),
        i18n("Comment"),
        i18n("File Name"),
        i18n("File Name (full path)"),
    };

    QStringList allColHeaders;
    allColHeaders.reserve(numColumnsToReserve + standardColHeaders.size());
    std::fill_n(std::back_inserter(allColHeaders), numColumnsToReserve, i18n("JuK"));
    std::copy  (standardColHeaders.cbegin(), standardColHeaders.cend(),
            std::back_inserter(allColHeaders));

    setHeaderLabels(allColHeaders);
    setAllColumnsShowFocus(true);
    setSelectionMode(QTreeWidget::ExtendedSelection);
    header()->setSortIndicatorShown(true);

    int numColumns = columnCount();

    m_columnFixedWidths.resize(numColumns);
    m_weightDirty.resize(numColumns);
    m_columnWeights.resize(numColumns);

    //////////////////////////////////////////////////
    // setup header RMB menu
    //////////////////////////////////////////////////

    QAction *showAction;
    const auto sharedSettings = SharedSettings::instance();

    for(int i = 0; i < numColumns; ++i) {
        showAction = new QAction(allColHeaders[i], m_headerMenu);
        showAction->setData(i);
        showAction->setCheckable(true);
        showAction->setChecked(sharedSettings->isColumnVisible(i));
        m_headerMenu->addAction(showAction);

        resizeColumnToContents(i);
    }

    connect(m_headerMenu, &QMenu::triggered,
            this, &Playlist::slotToggleColumnVisible);

    connect(this, &QWidget::customContextMenuRequested,
            this, &Playlist::slotShowRMBMenu);

    // Disabled for now because adding new items (File->Open) causes Qt to send
    // an itemChanged signal for unrelated playlist items which can cause the
    // inline editor done slot to mistakenly overwrite tags associated to
    // *other* playlist items. I haven't found a way to determine whether the
    // itemChanged signal is really coming from the inline editor so instead
    // users will need to use the tag editor. :(
    //  -- mpyne 2018-12-20
    //connect(this, &QTreeWidget::itemChanged,
    //        this, &Playlist::slotInlineEditDone);

    connect(action("resizeColumnsManually"), &KToggleAction::triggered,
            this, &Playlist::slotColumnResizeModeChanged);

    if(action<KToggleAction>("resizeColumnsManually")->isChecked()) {
        header()->setSectionResizeMode(QHeaderView::Interactive);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    } else {
        header()->setSectionResizeMode(QHeaderView::Fixed);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    }

    viewport()->setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragEnabled(true);

    // TODO: Retain last-run's sort
    sortByColumn(1, Qt::AscendingOrder);

    m_disableColumnWidthUpdates = false;
}

void Playlist::setupItem(PlaylistItem *item)
{
    item->setTrackId(g_trackID);
    g_trackID++;

    QModelIndex index = indexFromItem(item);
    if(!m_search->isEmpty())
        item->setHidden(!m_search->checkItem(&index));

    if(topLevelItemCount() <= 2 && !manualResize()) {
        slotWeightDirty();
        slotUpdateColumnWidths();
        viewport()->update();
    }
}

void Playlist::setDynamicListsFrozen(bool frozen)
{
    m_collection->setDynamicListsFrozen(frozen);
}

CollectionListItem *Playlist::collectionListItem(const FileHandle &file)
{
    CollectionListItem *item = CollectionList::instance()->lookup(file.absFilePath());

    if(!item) {
        if(!QFile::exists(file.absFilePath())) {
            qCCritical(JUK_LOG) << "File" << file.absFilePath() << "does not exist.";
            return nullptr;
        }

        item = CollectionList::instance()->createItem(file);
    }

    return item;
}

////////////////////////////////////////////////////////////////////////////////
// protected slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::slotPopulateBackMenu() const
{
    if(!playingItem())
        return;

    QMenu *menu = action<KToolBarPopupAction>("back")->menu();
    menu->clear();
    m_backMenuItems.clear();
    m_backMenuItems.reserve(10);

    int count = 0;
    PlaylistItemList::ConstIterator it = m_history.constEnd();

    QAction *action;

    while(it != m_history.constBegin() && count < 10) {
        ++count;
        --it;
        action = new QAction((*it)->file().tag()->title(), menu);
        action->setData(count - 1);
        menu->addAction(action);
        m_backMenuItems << *it;
    }
}

void Playlist::slotPlayFromBackMenu(QAction *backAction)
{
    int number = backAction->data().toInt();

    if(number >= m_backMenuItems.size())
        return;

    auto &nextItem = m_backMenuItems[number];
    beginPlayingItem(nextItem);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void Playlist::setup(int numColumnsToReserve)
{
    m_search = new PlaylistSearch(this);

    setAlternatingRowColors(true);
    setRootIsDecorated(false);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setUniformRowHeights(true);
    setEditTriggers(QAbstractItemView::EditKeyPressed); // Don't edit on double-click

    connect(header(), &QHeaderView::sectionMoved,
            this,     &Playlist::slotColumnOrderChanged);

    connect(m_fetcher, &WebImageFetcher::signalCoverChanged,
            this,      &Playlist::slotCoverChanged);

    // Prevent list of selected items from changing while internet search is in
    // progress.
    connect(this,      &Playlist::itemSelectionChanged,
            m_fetcher, &WebImageFetcher::abortSearch);

    connect(this, &QTreeWidget::itemDoubleClicked,
            this, &Playlist::slotPlayCurrent);

    // Use a timer to soak up the multiple dataChanged signals we're going to get
    auto updateRequestor = new QTimer(this);
    updateRequestor->setSingleShot(true);
    updateRequestor->setInterval(10);

    connect(model(), &QAbstractItemModel::dataChanged,
            updateRequestor, qOverload<>(&QTimer::start));
    connect(updateRequestor, &QTimer::timeout,
            this, &Playlist::slotUpdateTime);

    // This apparently must be created very early in initialization for other
    // Playlist code requiring m_headerMenu.
    m_columnVisibleAction = new KActionMenu(i18n("&Show Columns"), this);
    ActionCollection::actions()->addAction("showColumns", m_columnVisibleAction);

    m_headerMenu = m_columnVisibleAction->menu();

    header()->installEventFilter(this);

    // TODO: Determine if other stuff in setup must happen before slotInitialize().

    // Explicitly call slotInitialize() so that the columns are added before
    // SharedSettings::apply() sets the visible and hidden ones.
    slotInitialize(numColumnsToReserve);
}

void Playlist::loadFile(const QString &fileName, const QFileInfo &fileInfo)
{
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly))
        return;

    QTextStream stream(&file);

    // Turn off non-explicit sorting.

    setSortingEnabled(false);

    m_disableColumnWidthUpdates = true;
    m_blockDataChanged = true;

    PlaylistItem *after = nullptr;

    while(!stream.atEnd()) {
        QString itemName = stream.readLine().trimmed();

        QFileInfo item(itemName);

        if(item.isRelative())
            item.setFile(QDir::cleanPath(fileInfo.absolutePath() + '/' + itemName));

        if(item.exists() && item.isFile() && item.isReadable() &&
           MediaFiles::isMediaFile(item.fileName()))
        {
            after = createItem(FileHandle(item), after);
        }
    }

    m_blockDataChanged = false;
    m_disableColumnWidthUpdates = false;

    file.close();

    playlistItemsChanged();
}

void Playlist::setPlaying(PlaylistItem *item, bool addToHistory)
{
    auto wasPlayingItem = playingItem();
    if(wasPlayingItem == item)
        return;

    if(wasPlayingItem && addToHistory) {
        m_history.append(wasPlayingItem->collectionItem());

        const bool enableBack = !m_history.isEmpty();
        action<KToolBarPopupAction>("back")->menu()->setEnabled(enableBack);
    }

    if(wasPlayingItem) {
        // NB: will clear the whole list of playing items recursively
        wasPlayingItem->setPlaying(false);
    }

    if(item) {
        item->setPlaying(true);

        if(wasPlayingItem
            && m_search->isEmpty()
            && state() == QAbstractItemView::NoState
            && (!m_rmbMenu || m_rmbMenu->isHidden())
            )
        {
            scrollToItem(item, QAbstractItemView::PositionAtCenter);
        }
    }
}

bool Playlist::playing() const
{
    return playingItem() && this == playingItem()->playlist();
}

int Playlist::leftMostVisibleColumn() const
{
    int i = 0;
    while(i < PlaylistItem::lastColumn() && isColumnHidden(i))
        i++;

    return i < PlaylistItem::lastColumn() ? i : 0;
}

// used with slotShowRMBMenu
void Playlist::createPlaylistRMBMenu()
{
    using namespace IconSupport; // ""_icon

    m_rmbMenu = new QMenu(this);

    m_rmbMenu->addAction("go-jump-today"_icon, i18n("Add to Play Queue"),
            this, &Playlist::slotAddToUpcoming);
    m_rmbMenu->addSeparator();

    if(!this->readOnly()) {
        m_rmbMenu->addAction(action("edit_cut"));
        m_rmbMenu->addAction(action("edit_copy"));
        m_rmbMenu->addAction(action("edit_paste"));
        m_rmbMenu->addSeparator();
        m_rmbMenu->addAction(action("removeFromPlaylist"));
    }
    else
        m_rmbMenu->addAction(action("edit_copy"));

    m_rmbEdit = m_rmbMenu->addAction(i18n("Edit"));

    m_rmbMenu->addAction(action("refresh"));
    m_rmbMenu->addAction(action("openItemDir"));
    m_rmbMenu->addAction(action("removeItem"));

    m_rmbMenu->addSeparator();

    m_rmbMenu->addAction(action("guessTag"));
    m_rmbMenu->addAction(action("renameFile"));

    m_rmbMenu->addAction(action("coverManager"));

    m_rmbMenu->addSeparator();

    m_rmbMenu->addAction("folder-new"_icon,
        i18n("Create Playlist From Selected Items..."),
        this, &Playlist::slotCreateGroup);
}

PlaylistItemList Playlist::items(QTreeWidgetItemIterator::IteratorFlags flags)
{
    PlaylistItemList list;

    for(QTreeWidgetItemIterator it(this, flags); *it; ++it)
        list.append(static_cast<PlaylistItem *>(*it));

    return list;
}

void Playlist::calculateColumnWeights()
{
    if(m_disableColumnWidthUpdates)
        return;

    const PlaylistItemList itemList = items();

    QVector<double> averageWidth(columnCount());
    double itemCount = itemList.size();

    // Here we're not using a real average, but averaging the squares of the
    // column widths and then using the square root of that value.  This gives
    // a nice weighting to the longer columns without doing something arbitrary
    // like adding a fixed amount of padding.

    for(const auto &item : itemList) {
        const auto cachedWidth = item->cachedWidths();

        // Extra columns start at 0, but those weights aren't shared with all
        // items.
        for(int i = 0; i < columnOffset(); ++i) {
            const auto width = columnWidth(i);
            averageWidth[i] += width * width / itemCount;
        }

        const auto offset = columnOffset();
        for(int column = offset; column < columnCount(); ++column) {
            const auto width = cachedWidth[column - offset];
            averageWidth[column] += width * width / itemCount;
        }
    }

    if(m_columnWeights.isEmpty())
        m_columnWeights.fill(-1, columnCount());

    for(const auto column : qAsConst(m_weightDirty)) {
        m_columnWeights[column] = int(std::sqrt(averageWidth[column]) + 0.5);
    }

    m_weightDirty.clear();
}

void Playlist::addPlaylistFile(const QString &m3uFile)
{
    if(!m_collection->containsPlaylistFile(m3uFile)) {
        new Playlist(m_collection, QFileInfo(m3uFile));
    }
}

QFuture<void> Playlist::addFilesFromDirectory(const QString &dirPath)
{
    auto loader = new DirectoryLoader(dirPath);

    connect(loader, &DirectoryLoader::loadedPlaylist, this,
        [this](const QString &m3uFile) {
            addPlaylistFile(m3uFile);
        }
    );
    connect(loader, &DirectoryLoader::loadedFiles, this,
        [this](const FileHandleList &newFiles) {
            for(const auto &newFile : newFiles) {
                createItem(newFile);
            }
        }
    );

    auto future = QtConcurrent::run(loader, &DirectoryLoader::startLoading);
    auto loadWatcher = new QFutureWatcher<void>(this);
    connect(loadWatcher, &QFutureWatcher<void>::finished, this, [=]() {
            loader->deleteLater();
            loadWatcher->deleteLater();
        });

    return future;
}

// Returns a future since some codepaths will result in an async operation.
QFuture<void> Playlist::addUntypedFile(const QString &file, PlaylistItem *after)
{
    if(hasItem(file) && !m_allowDuplicates)
        return {};

    const QFileInfo fileInfo(file);
    const QString canonicalPath = fileInfo.canonicalFilePath();

    if(fileInfo.isFile() && fileInfo.isReadable() &&
        MediaFiles::isMediaFile(file))
    {
        FileHandle f(fileInfo);
        f.tag();
        createItem(f, after);
        return {};
    }

    if(MediaFiles::isPlaylistFile(file)) {
        addPlaylistFile(canonicalPath);
        return {};
    }

    if(fileInfo.isDir()) {
        const auto blockedPaths = m_collection->excludedFolders();
        if(std::none_of(blockedPaths.begin(), blockedPaths.end(),
            [canonicalPath](const auto &dir) {
                return canonicalPath.startsWith(dir);
            }))
        {
            return addFilesFromDirectory(canonicalPath);
        }
    }

    return {};
}

// Called directly or after a threaded directory load has completed, managed by
// m_itemsLoading
void Playlist::cleanupAfterAllFileLoadsCompleted()
{
    m_blockDataChanged = false;
    setEnabled(true);

    // Even if doing a manual column weights we'll generally start off with
    // incorrect column sizes so at least figure out a reasonable column size
    // and let user adjust from there.
    if(manualResize()) {
        auto manualResizeAction = action<KToggleAction>("resizeColumnsManually");
        auto wasChecked = manualResizeAction->isChecked();

        manualResizeAction->setChecked(false);
        calculateColumnWeights();
        slotUpdateColumnWidths();
        manualResizeAction->setChecked(wasChecked);
    }

    playlistItemsChanged();
}

////////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void Playlist::slotUpdateColumnWidths()
{
    if(m_disableColumnWidthUpdates || manualResize())
        return;

    // Make sure that the column weights have been initialized before trying to
    // update the columns.

    QVector<int> visibleColumns;
    for(int i = 0; i < columnCount(); i++) {
        if(!isColumnHidden(i))
            visibleColumns.append(i);
    }

    if(visibleColumns.isEmpty()) {
        return;
    }

    // convenience handler for deprecated text metrics
    const auto textWidth = [](const QFontMetrics &fm, const QString &text) {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 11, 0))
        return fm.horizontalAdvance(text);
#else
        return fm.width(text);
#endif
    };

    // No item content to auto-fit around, use the headers for now
    if(count() == 0) {
        for(int column : qAsConst(visibleColumns)) {
            setColumnWidth(column, textWidth(header()->fontMetrics(),headerItem()->text(column)) + 10);
        }

        return;
    }

    if(m_columnWeights.isEmpty())
        return;

    // First build a list of minimum widths based on the strings in the listview
    // header.  We won't let the width of the column go below this width.

    QVector<int> minimumWidth(columnCount(), 0);
    int minimumWidthTotal = 0;

    // Also build a list of either the minimum *or* the fixed width -- whichever is
    // greater.

    QVector<int> minimumFixedWidth(columnCount(), 0);
    int minimumFixedWidthTotal = 0;

    for(int column : qAsConst(visibleColumns)) {
        minimumWidth[column] = textWidth(header()->fontMetrics(), headerItem()->text(column)) + 10;
        minimumWidthTotal += minimumWidth[column];

        minimumFixedWidth[column] = qMax(minimumWidth[column], m_columnFixedWidths[column]);
        minimumFixedWidthTotal += minimumFixedWidth[column];
    }

    // Make sure that the width won't get any smaller than this.  We have to
    // account for the scrollbar as well.  Since this method is called from the
    // resize event this will set a pretty hard toLower bound on the size.

    setMinimumWidth(minimumWidthTotal + verticalScrollBar()->width());

    // If we've got enough room for the fixed widths (larger than the minimum
    // widths) then instead use those for our "minimum widths".

    if(minimumFixedWidthTotal < viewport()->width()) {
        minimumWidth = minimumFixedWidth;
        minimumWidthTotal = minimumFixedWidthTotal;
    }

    // We've got a list of columns "weights" based on some statistics gathered
    // about the widths of the items in that column.  We need to find the total
    // useful weight to use as a divisor for each column's weight.

    double totalWeight = 0;
    for(int column : qAsConst(visibleColumns)) {
        totalWeight += m_columnWeights[column];
    }

    // This can happen during startup, before we have the tracks loaded.
    if(qFuzzyIsNull(totalWeight)) {
        return;
    }

    // Computed a "weighted width" for each visible column.  This would be the
    // width if we didn't have to handle the cases of minimum and maximum widths.

    QVector<int> weightedWidth(columnCount(), 0);
    for(int column : qAsConst(visibleColumns)) {
        weightedWidth[column] = int(double(m_columnWeights[column]) / totalWeight * viewport()->width() + 0.5);
    }

    // The "extra" width for each column.  This is the weighted width less the
    // minimum width or zero if the minimum width is greater than the weighted
    // width.

    QVector<int> extraWidth(columnCount(), 0);

    // This is used as an indicator if we have any columns where the weighted
    // width is less than the minimum width.  If this is false then we can
    // just use the weighted width with no problems, otherwise we have to
    // "readjust" the widths.

    bool readjust = false;

    // If we have columns where the weighted width is less than the minimum width
    // we need to steal that space from somewhere.  The amount that we need to
    // steal is the "neededWidth".

    int neededWidth = 0;

    // While we're on the topic of stealing -- we have to have somewhere to steal
    // from.  availableWidth is the sum of the amount of space beyond the minimum
    // width that each column has been allocated -- the sum of the values of
    // extraWidth[].

    int availableWidth = 0;

    // Fill in the values discussed above.

    for(int column : qAsConst(visibleColumns)) {
        if(weightedWidth[column] < minimumWidth[column]) {
            readjust = true;
            extraWidth[column] = 0;
            neededWidth += minimumWidth[column] - weightedWidth[column];
        }
        else {
            extraWidth[column] = weightedWidth[column] - minimumWidth[column];
            availableWidth += extraWidth[column];
        }
    }

    // The adjustmentRatio is the amount of the "extraWidth[]" that columns will
    // actually be given.

    double adjustmentRatio = (double(availableWidth) - double(neededWidth)) / double(availableWidth);

    // This will be the sum of the total space that we actually use.  Because of
    // rounding error this won't be the exact available width.

    int usedWidth = 0;

    // Now set the actual column widths.  If the weighted widths are all greater
    // than the minimum widths, just use those, otherwise use the "readjusted
    // weighted width".

    for(int column : qAsConst(visibleColumns)) {
        int width;
        if(readjust) {
            int adjustedExtraWidth = int(double(extraWidth[column]) * adjustmentRatio + 0.5);
            width = minimumWidth[column] + adjustedExtraWidth;
        }
        else
            width = weightedWidth[column];

        setColumnWidth(column, width);
        usedWidth += width;
    }

    // Fill the remaining gap for a clean fit into the available space.

    int remainingWidth = viewport()->width() - usedWidth;
    setColumnWidth(visibleColumns.back(), columnWidth(visibleColumns.back()) + remainingWidth);

    m_widthsDirty = false;
}

void Playlist::slotAddToUpcoming()
{
    m_collection->setUpcomingPlaylistEnabled(true);
    m_collection->upcomingPlaylist()->appendItems(selectedItems());
}

void Playlist::slotShowRMBMenu(const QPoint &point)
{
    QTreeWidgetItem *item = itemAt(point);
    int column = columnAt(point.x());
    if(!item)
        return;

    // Create the RMB menu on demand.

    if(!m_rmbMenu) {
        createPlaylistRMBMenu();
    }

    // Ignore any columns added by subclasses.

    const int adjColumn = column - columnOffset();

    bool showEdit =
        (adjColumn == PlaylistItem::TrackColumn) ||
        (adjColumn == PlaylistItem::ArtistColumn) ||
        (adjColumn == PlaylistItem::AlbumColumn) ||
        (adjColumn == PlaylistItem::TrackNumberColumn) ||
        (adjColumn == PlaylistItem::GenreColumn) ||
        (adjColumn == PlaylistItem::YearColumn);

    if(showEdit) {
        m_rmbEdit->setText(i18n("Edit '%1'", item->text(column)));

        m_rmbEdit->disconnect(this);
        connect(m_rmbEdit, &QAction::triggered, this, [this, item, column]() {
            this->editItem(item, column);
        });
    }

    m_rmbEdit->setVisible(showEdit);

    // Disable edit menu if only one file is selected, and it's read-only

    FileHandle file = static_cast<PlaylistItem*>(item)->file();

    m_rmbEdit->setEnabled(file.fileInfo().isWritable() || selectedItems().count() > 1);

    // View cover is based on if there is a cover to see.  We should only have
    // the remove cover option if the cover is in our database (and not directly
    // embedded in the file, for instance).

    action("viewCover")->setEnabled(file.coverInfo()->hasCover());
    action("removeCover")->setEnabled(file.coverInfo()->coverId() != CoverManager::NoMatch);

    m_rmbMenu->popup(mapToGlobal(point));
}

bool Playlist::editTag(PlaylistItem *item, const QString &text, int column)
{
    Tag *newTag = TagTransactionManager::duplicateTag(item->file().tag());

    switch(column - columnOffset())
    {
    case PlaylistItem::TrackColumn:
        newTag->setTitle(text);
        break;
    case PlaylistItem::ArtistColumn:
        newTag->setArtist(text);
        break;
    case PlaylistItem::AlbumColumn:
        newTag->setAlbum(text);
        break;
    case PlaylistItem::TrackNumberColumn:
    {
        bool ok;
        int value = text.toInt(&ok);
        if(ok)
            newTag->setTrack(value);
        break;
    }
    case PlaylistItem::GenreColumn:
        newTag->setGenre(text);
        break;
    case PlaylistItem::YearColumn:
    {
        bool ok;
        int value = text.toInt(&ok);
        if(ok)
            newTag->setYear(value);
        break;
    }
    }

    TagTransactionManager::instance()->changeTagOnItem(item, newTag);
    return true;
}

void Playlist::slotInlineEditDone(QTreeWidgetItem *item, int column)
{
    // The column we get is as passed from QTreeWidget so it does not need
    // adjustment to get the right text from the QTreeWidgetItem

    QString text = item->text(column);
    const PlaylistItemList l = selectedItems();

    // See if any of the files have a tag different from the input.

    const int adjColumn = column - columnOffset();
    bool changed = std::any_of(l.cbegin(), l.cend(),
        [text, adjColumn] (const PlaylistItem *item) { return item->text(adjColumn) != text; }
        );

    if(!changed ||
       (l.count() > 1 && KMessageBox::warningContinueCancel(
           0,
           i18n("This will edit multiple files. Are you sure?"),
           QString(),
           KGuiItem(i18n("Edit")),
           KStandardGuiItem::cancel(),
           "DontWarnMultipleTags") == KMessageBox::Cancel))
    {
        return;
    }

    for(auto &selItem : l) {
        editTag(selItem, text, column);
    }

    TagTransactionManager::instance()->commit();

    CollectionList::instance()->playlistItemsChanged();
    playlistItemsChanged();
}

void Playlist::slotColumnOrderChanged(int, int from, int to)
{
    if(from == 0 || to == 0) {
        updatePlaying();
        m_leftColumn = header()->sectionPosition(0);
    }

    SharedSettings::instance()->setColumnOrder(this);
}

void Playlist::slotToggleColumnVisible(QAction *action)
{
    int column = action->data().toInt();

    if(isColumnHidden(column)) {
        int fileNameColumn = PlaylistItem::FileNameColumn + columnOffset();
        int fullPathColumn = PlaylistItem::FullPathColumn + columnOffset();

        if(column == fileNameColumn && !isColumnHidden(fullPathColumn)) {
            hideColumn(fullPathColumn, false);
            SharedSettings::instance()->toggleColumnVisible(fullPathColumn);
        }
        if(column == fullPathColumn && !isColumnHidden(fileNameColumn)) {
            hideColumn(fileNameColumn, false);
            SharedSettings::instance()->toggleColumnVisible(fileNameColumn);
        }
    }

    if(!isColumnHidden(column))
        hideColumn(column);
    else
        showColumn(column);

    if(column >= columnOffset()) {
        SharedSettings::instance()->toggleColumnVisible(column - columnOffset());
    }
}

void Playlist::slotCreateGroup()
{
    QString name = m_collection->playlistNameDialog(i18n("Create New Playlist"));

    if(!name.isEmpty())
        new Playlist(m_collection, selectedItems(), name);
}

void Playlist::notifyUserColumnWidthModeChanged()
{
    KMessageBox::information(this,
                             i18n("Manual column widths have been enabled. You can "
                                  "switch back to automatic column sizes in the view "
                                  "menu."),
                             i18n("Manual Column Widths Enabled"),
                             "ShowManualColumnWidthInformation");
}

void Playlist::columnResized(int column, int, int newSize)
{
    m_widthsDirty = true;
    m_columnFixedWidths[column] = newSize;
}

void Playlist::slotPlayCurrent()
{
    QTreeWidgetItemIterator it(this, QTreeWidgetItemIterator::Selected);
    PlaylistItem *next = static_cast<PlaylistItem *>(*it);
    beginPlayingItem(next);
    refillRandomList();
}

void Playlist::slotUpdateTime()
{
    int newTime = 0;
    QTreeWidgetItemIterator it(this);
    while(*it) {
        const auto item = static_cast<const PlaylistItem*>(*it);
        ++it;

        newTime += item->file().tag()->seconds();
    }

    m_time = newTime;
}

////////////////////////////////////////////////////////////////////////////////
// helper functions
////////////////////////////////////////////////////////////////////////////////

QDataStream &operator<<(QDataStream &s, const Playlist &p)
{
    s << p.name();
    s << p.fileName();
    s << p.files();

    return s;
}

QDataStream &operator>>(QDataStream &s, Playlist &p)
{
    p.read(s);
    return s;
}

bool processEvents()
{
    static QElapsedTimer time;

    if(time.elapsed() > 100) {
        time.restart();
        qApp->processEvents();
        return true;
    }
    return false;
}

// vim: set et sw=4 tw=0 sta:
