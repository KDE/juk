#include "playlistview.h"
#include "actioncollection.h"
#include <QContextMenuEvent>
#include <KMenu>
#include <KIconLoader>
#include <KLocalizedString>
#include "k3bexporter.h"
#include <QFileInfo>
#include <QToolTip>
#include "coverinfo.h"
#include <KApplication>
#include <KUrl>
#include <QClipboard>
#include "mediafiles.h"
#include <KIO/NetAccess>
#include <KMimeType>
#include "playlist/playlistheaderview.h"
#include "playlist/playlistsortfilterproxymodel.h"
#include "tracksequencemanager.h"

using namespace ActionCollection;


PlaylistView::PlaylistView(QWidget* parent):
    QTreeView(parent),
    m_contextMenu(0),
    m_editAction(0),
    m_currentColumn(0),
    m_proxyModel(new PlaylistSortFilterProxyModel(this))
{
    setHeader(new PlaylistHeaderView(Qt::Horizontal, this));
    
    setUniformRowHeights(true);
    setSortingEnabled(true);
    setAlternatingRowColors(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setItemsExpandable(false);
    setIndentation(0);
}

void PlaylistView::contextMenuEvent(QContextMenuEvent *event)
{
        // Create the RMB menu on demand.
    if (playlist()->rowCount() == 0 || indexAt(event->pos()).row() == -1)
        return;
    
    if(!m_contextMenu) {

        // ### Probably more of these actions should be ported over to using KActions.

        m_contextMenu = new KMenu(this);

        m_contextMenu->addAction(SmallIcon("go-jump-today"),
            i18n("Add to Play Queue"), this, SLOT(slotAddToUpcoming()));
        m_contextMenu->addSeparator();

        if(!playlist()->readOnly()) {
            m_contextMenu->addAction( action("edit_cut") );
            m_contextMenu->addAction( action("edit_copy") );
            m_contextMenu->addAction( action("edit_paste") );
            m_contextMenu->addSeparator();
            m_contextMenu->addAction( action("removeFromPlaylist") );
        }
        else
            m_contextMenu->addAction( action("edit_copy") );

        m_editAction = m_contextMenu->addAction(i18n("Edit"), this, SLOT(slotRenameTag()));

        m_contextMenu->addAction( action("refresh") );
        m_contextMenu->addAction( action("removeItem") );

        m_contextMenu->addSeparator();

        m_contextMenu->addAction( action("guessTag") );
        m_contextMenu->addAction( action("renameFile") );

        m_contextMenu->addAction( action("coverManager") );

        m_contextMenu->addSeparator();

        m_contextMenu->addAction(
            SmallIcon("folder-new"), i18n("Create Playlist From Selected Items..."), this, SLOT(slotCreateGroup()));

        K3bExporter *exporter = new K3bExporter(playlist());
        KAction *k3bAction = exporter->action();
        if(k3bAction)
            m_contextMenu->addAction( k3bAction );
    }

    // Ignore any columns added by subclasses.

//    column -= columnOffset();
    int column = columnAt(event->x());
    bool showEdit =
        (column == Playlist::TrackColumn) ||
        (column == Playlist::ArtistColumn) ||
        (column == Playlist::AlbumColumn) ||
        (column == Playlist::TrackNumberColumn) ||
        (column == Playlist::GenreColumn) ||
        (column == Playlist::YearColumn);

    if(showEdit)
        m_editAction->setText(i18n("Edit '%1'", model()->headerData(column, Qt::Horizontal).toString()));

    m_editAction->setVisible(showEdit);

    // Disable edit menu if only one file is selected, and it's read-only
    
    FileHandle file = playlist()->fileHandles()[indexAt(event->pos()).row()];

    m_editAction->setEnabled(file.fileInfo().isWritable() || selectedIndexes().count() > 1);

    // View cover is based on if there is a cover to see.  We should only have
    // the remove cover option if the cover is in our database (and not directly
    // embedded in the file, for instance).

    action("viewCover")->setEnabled(file.coverInfo()->hasCover());
    action("removeCover")->setEnabled(file.coverInfo()->coverId() != CoverManager::NoMatch);

//     m_contextMenu->popup(point);
    m_currentColumn = column;
    
    m_contextMenu->exec(event->globalPos());
}

bool PlaylistView::event(QEvent *e)
{
    if (e->type() == QEvent::ToolTip) {
        QHelpEvent *event = static_cast<QHelpEvent*>(e);
        const QModelIndex index = indexAt(event->pos());

        // A tooltip specialized to show full filenames over the file name column.
        if (index.column() == Playlist::FileNameColumn) {
            const QModelIndex fullPathIndex = model()->index(index.row(), Playlist::FullPathColumn);
            const QString text = model()->data(fullPathIndex).toString();
            QToolTip::showText(event->globalPos(), text);
            return true;
        }// else if (column == Playlist::CoverColumn) { TODO
    }

    return QAbstractItemView::event(e);
}

void PlaylistView::slotRefresh()
{
    QModelIndexList l = selectedIndexes();
    KApplication::setOverrideCursor(Qt::waitCursor);
    playlist()->refreshRows(l);
    KApplication::restoreOverrideCursor();
}

void PlaylistView::copy()
{
    QModelIndexList items = selectedIndexes();
    KUrl::List urls;

    foreach (const QModelIndex &i, items) {
        const QModelIndex index = model()->index(i.row(), Playlist::FullPathColumn);
        
        urls << KUrl::fromPath(model()->data(index).toString());
    }

    QMimeData *mimeData = new QMimeData;
    urls.populateMimeData(mimeData);

    QApplication::clipboard()->setMimeData(mimeData, QClipboard::Clipboard);
}

void PlaylistView::paste()
{
    decode(QApplication::clipboard()->mimeData(), currentIndex());
}


void PlaylistView::decode(const QMimeData *s, const QModelIndex &index)
{
    if(!KUrl::List::canDecode(s))
        return;

    const KUrl::List urls = KUrl::List::fromMimeData(s);

    if(urls.isEmpty())
        return;

    // handle dropped images

    if(!MediaFiles::isMediaFile(urls.front().path())) {

        QString file;

        if(urls.front().isLocalFile())
            file = urls.front().path();
        else
            KIO::NetAccess::download(urls.front(), file, 0);

        KMimeType::Ptr mimeType = KMimeType::findByPath(file);

        if(index.isValid() && mimeType->name().startsWith(QLatin1String("image/"))) {
            const QModelIndex coverIndex = model()->index(index.row(), Playlist::CoverColumn);
            model()->setData(coverIndex, QImage(file));
//             item->file().coverInfo()->setCover(QImage(file));
//             refreshAlbum(item->file().tag()->artist(),
//                          item->file().tag()->album());
        }

        KIO::NetAccess::removeTempFile(file);
    }

    QStringList fileList = MediaFiles::convertURLsToLocal(urls, this);

    playlist()->addFiles(fileList, index.row());
}

void PlaylistView::setModel(QAbstractItemModel* newModel)
{
    if (!model())
        QTreeView::setModel(m_proxyModel);
    
    m_proxyModel->setSourceModel(newModel);
}

QAbstractItemModel* PlaylistView::model()
{
    return m_proxyModel->sourceModel();
}


void PlaylistView::setSearch(const PlaylistSearch &s)
{
    m_proxyModel->setSearch(s);
//     m_search = s;
// 
//     if(!m_searchEnabled)
//         return;
// 
//     setItemsVisible(s.matchedItems(), true);
//     setItemsVisible(s.unmatchedItems(), false);
// 
//     TrackSequenceManager::instance()->iterator()->playlistChanged();
}

void PlaylistView::setSearchEnabled(bool enabled)
{
    m_proxyModel->setSearchEnabled(enabled);
//     if(m_searchEnabled == enabled)
//         return;
// 
//     m_searchEnabled = enabled;
// 
//     if(enabled) {
//         setItemsVisible(m_search.matchedItems(), true);
//         setItemsVisible(m_search.unmatchedItems(), false);
//     }
//     else
//         setItemsVisible(items(), true);
}



// ### TODO: View
// PlaylistItemList Playlist::visibleItems()
// {
//     return items(Q3ListViewItemIterator::Visible);
// }

// void Playlist::updateLeftColumn()
// {
//     int newLeftColumn = leftMostVisibleColumn();
// 
//     if(m_leftColumn != newLeftColumn) {
//         updatePlaying();
//         m_leftColumn = newLeftColumn;
//     }
// }

// ### TODO: View
// void Playlist::setItemsVisible(const PlaylistItemList &items, bool visible) // static
// {
//     m_visibleChanged = true;
// 
//     foreach(PlaylistItem *playlistItem, items)
//         playlistItem->setVisible(visible);
// }


// ### TODO: View
// void Playlist::markItemSelected(PlaylistItem *item, bool selected)
// {
//     if(selected && !item->isSelected()) {
//         m_selectedCount++;
//     }
//     else if(!selected && item->isSelected())
//         m_selectedCount--;
// }
// 
// void Playlist::slotRenameFile()
// {
//     FileRenamer renamer;
//     PlaylistItemList items = selectedItems();
// 
//     if(items.isEmpty())
//         return;
// 
//     emit signalEnableDirWatch(false);
// 
//     m_blockDataChanged = true;
//     renamer.rename(items);
//     m_blockDataChanged = false;
//     dataChanged();
// 
//     emit signalEnableDirWatch(true);
// }
// 
// void Playlist::slotViewCover()
// {
//     const PlaylistItemList items = selectedItems();
//     if (items.isEmpty())
//         return;
//     foreach(const PlaylistItem *item, items)
//         item->file().coverInfo()->popup();
// }
// 
// void Playlist::slotRemoveCover()
// {
//     PlaylistItemList items = selectedItems();
//     if(items.isEmpty())
//         return;
//     int button = KMessageBox::warningContinueCancel(this,
//                                                     i18n("Are you sure you want to delete these covers?"),
//                                                     QString(),
//                                                     KGuiItem(i18n("&Delete Covers")));
//     if(button == KMessageBox::Continue)
//         refreshAlbums(items);
// }
// 
// void Playlist::slotShowCoverManager()
// {
//     static CoverDialog *managerDialog = 0;
// 
//     if(!managerDialog)
//         managerDialog = new CoverDialog(this);
// 
//     managerDialog->show();
// }
// 
// void Playlist::slotAddCover(bool retrieveLocal)
// {
//     PlaylistItemList items = selectedItems();
// 
//     if(items.isEmpty())
//         return;
// 
//     if(!retrieveLocal) {
//         m_fetcher->setFile((*items.begin())->file());
//         m_fetcher->searchCover();
//         return;
//     }
// 
//     KUrl file = KFileDialog::getImageOpenUrl(
//         KUrl( "kfiledialog://homedir" ), this, i18n("Select Cover Image File"));
// 
//     if(file.isEmpty())
//         return;
// 
//     QString artist = items.front()->file().tag()->artist();
//     QString album = items.front()->file().tag()->album();
// 
//     coverKey newId = CoverManager::addCover(file, artist, album);
// 
//     if(newId != CoverManager::NoMatch)
//         refreshAlbums(items, newId);
// }
// 
// // Called when image fetcher has added a new cover.
// void Playlist::slotCoverChanged(int coverId)
// {
//     kDebug() << "Refreshing information for newly changed covers.\n";
//     refreshAlbums(selectedItems(), coverId);
// }
// 
// void Playlist::slotGuessTagInfo(TagGuesser::Type type)
// {
//     KApplication::setOverrideCursor(Qt::waitCursor);
//     const PlaylistItemList items = selectedItems();
//     setDynamicListsFrozen(true);
// 
//     m_blockDataChanged = true;
// 
//     foreach(PlaylistItem *item, items) {
//         item->guessTagInfo(type);
//         processEvents();
//     }
// 
//     // MusicBrainz queries automatically commit at this point.  What would
//     // be nice is having a signal emitted when the last query is completed.
// 
//     if(type == TagGuesser::FileName)
//         TagTransactionManager::instance()->commit();
// 
//     m_blockDataChanged = false;
// 
//     dataChanged();
//     setDynamicListsFrozen(false);
//     KApplication::restoreOverrideCursor();
// }
// 

// 
// void Playlist::slotWeightDirty(int column)
// {
//     if(column < 0) {
//         m_weightDirty.clear();
//         for(int i = 0; i < columns(); i++) {
//             if(isColumnVisible(i))
//                 m_weightDirty.append(i);
//         }
//         return;
//     }
// 
//     if(!m_weightDirty.contains(column))
//         m_weightDirty.append(column);
// }
// 
// void Playlist::slotShowPlaying()
// {
//     if(!playingItem())
//         return;
// 
//     Playlist *l = playingItem()->playlist();
// 
//     l->clearSelection();
// 
//     // Raise the playlist before selecting the items otherwise the tag editor
//     // will not update when it gets the selectionChanged() notification
//     // because it will think the user is choosing a different playlist but not
//     // selecting a different item.
// 
//     m_collection->raise(l);
// 
//     l->setSelected(playingItem(), true);
//     l->ensureItemVisible(playingItem());
// }
// 
// void Playlist::slotColumnResizeModeChanged()
// {
//     if(manualResize())
//         setHScrollBarMode(Auto);
//     else
//         setHScrollBarMode(AlwaysOff);
// 
//     if(!manualResize())
//         slotUpdateColumnWidths();
// 
//     SharedSettings::instance()->sync();
// }

// 
// Q3DragObject *Playlist::dragObject(QWidget *parent)
// {
//     PlaylistItemList items = selectedItems();
//     KUrl::List urls;
// 
//     foreach(PlaylistItem *item, items) {
//         urls << KUrl::fromPath(item->file().absFilePath());
//     }
// 
//     K3URLDrag *urlDrag = new K3URLDrag(urls, parent);
// 
//     urlDrag->setPixmap(BarIcon("audio-x-generic"));
// 
//     return urlDrag;
// }
// 
// void Playlist::contentsDragEnterEvent(QDragEnterEvent *e)
// {
//     K3ListView::contentsDragEnterEvent(e);
// 
//     if(CoverDrag::isCover(e->mimeData())) {
//         setDropHighlighter(true);
//         setDropVisualizer(false);
// 
//         e->accept();
//         return;
//     }
// 
//     setDropHighlighter(false);
//     setDropVisualizer(true);
// 
//     KUrl::List urls;
//     if(!K3URLDrag::decode(e, urls) || urls.isEmpty()) {
//         e->ignore();
//         return;
//     }
// 
//     e->accept();
//     return;
// }
// 
// bool Playlist::acceptDrag(QDropEvent *e) const
// {
//     return CoverDrag::isCover(e->mimeData()) || K3URLDrag::canDecode(e);
// }
// 
// 
// bool Playlist::eventFilter(QObject *watched, QEvent *e)
// {
//     if(watched == header()) {
//         switch(e->type()) {
//         case QEvent::MouseMove:
//         {
//             if((static_cast<QMouseEvent *>(e)->modifiers() & Qt::LeftButton) == Qt::LeftButton &&
//                 !action<KToggleAction>("resizeColumnsManually")->isChecked())
//             {
//                 m_columnWidthModeChanged = true;
// 
//                 action<KToggleAction>("resizeColumnsManually")->setChecked(true);
//                 slotColumnResizeModeChanged();
//             }
// 
//             break;
//         }
//         case QEvent::MouseButtonPress:
//         {
//             if(static_cast<QMouseEvent *>(e)->button() == Qt::RightButton)
//                 m_headerMenu->popup(QCursor::pos());
// 
//             break;
//         }
//         case QEvent::MouseButtonRelease:
//         {
//             if(m_columnWidthModeChanged) {
//                 m_columnWidthModeChanged = false;
//                 notifyUserColumnWidthModeChanged();
//             }
// 
//             if(!manualResize() && m_widthsDirty)
//                 QTimer::singleShot(0, this, SLOT(slotUpdateColumnWidths()));
//             break;
//         }
//         default:
//             break;
//         }
//     }
// 
//     return K3ListView::eventFilter(watched, e);
// }
// 
// void Playlist::keyPressEvent(QKeyEvent *event)
// {
//     if(event->key() == Qt::Key_Up) {
//         Q3ListViewItemIterator selected(this, Q3ListViewItemIterator::IteratorFlag(
//                                            Q3ListViewItemIterator::Selected |
//                                            Q3ListViewItemIterator::Visible));
//         if(selected.current()) {
//             Q3ListViewItemIterator visible(this, Q3ListViewItemIterator::IteratorFlag(
//                                               Q3ListViewItemIterator::Visible));
//             if(selected.current() == visible.current())
//                 KApplication::postEvent(parent(), new FocusUpEvent);
//         }
// 
//     }
// 
//     K3ListView::keyPressEvent(event);
// }
// 
// void Playlist::contentsDropEvent(QDropEvent *e)
// {
//     QPoint vp = contentsToViewport(e->pos());
//     PlaylistItem *item = static_cast<PlaylistItem *>(itemAt(vp));
// 
//     // First see if we're dropping a cover, if so we can get it out of the
//     // way early.
//     if(item && CoverDrag::isCover(e->mimeData())) {
//         coverKey id = CoverDrag::idFromData(e->mimeData());
// 
//         // If the item we dropped on is selected, apply cover to all selected
//         // items, otherwise just apply to the dropped item.
// 
//         if(item->isSelected()) {
//             const PlaylistItemList selItems = selectedItems();
//             foreach(PlaylistItem *playlistItem, selItems) {
//                 playlistItem->file().coverInfo()->setCoverId(id);
//                 playlistItem->refresh();
//             }
//         }
//         else {
//             item->file().coverInfo()->setCoverId(id);
//             item->refresh();
//         }
// 
//         return;
//     }
// 
//     // When dropping on the toUpper half of an item, insert before this item.
//     // This is what the user expects, and also allows the insertion at
//     // top of the list
// 
//     if(!item)
//         item = static_cast<PlaylistItem *>(lastItem());
//     else if(vp.y() < item->itemPos() + item->height() / 2)
//         item = static_cast<PlaylistItem *>(item->itemAbove());
// 
//     m_blockDataChanged = true;
// 
//     if(e->source() == this) {
// 
//         // Since we're trying to arrange things manually, turn off sorting.
// 
//         setSorting(columns() + 1);
// 
//         const QList<Q3ListViewItem *> items = K3ListView::selectedItems();
// 
//         foreach(Q3ListViewItem *listViewItem, items) {
//             if(!item) {
// 
//                 // Insert the item at the top of the list.  This is a bit ugly,
//                 // but I don't see another way.
// 
//                 takeItem(listViewItem);
//                 insertItem(listViewItem);
//             }
//             else
//                 listViewItem->moveItem(item);
// 
//             item = static_cast<PlaylistItem *>(listViewItem);
//         }
//     }
//     else
//         decode(e->mimeData(), item);
// 
//     m_blockDataChanged = false;
// 
//     dataChanged();
//     emit signalPlaylistItemsDropped(this);
//     K3ListView::contentsDropEvent(e);
// }
// 
// void Playlist::contentsMouseDoubleClickEvent(QMouseEvent *e)
// {
//     // Filter out non left button double clicks, that way users don't have the
//     // weird experience of switching songs from a double right-click.
// 
//     if(e->button() == Qt::LeftButton)
//         K3ListView::contentsMouseDoubleClickEvent(e);
// }
// 
// void Playlist::showEvent(QShowEvent *e)
// {
//     if(m_applySharedSettings) {
//         SharedSettings::instance()->apply(this);
//         m_applySharedSettings = false;
//     }
// 
//     K3ListView::showEvent(e);
// }
// 
// void Playlist::viewportPaintEvent(QPaintEvent *pe)
// {
//     // If there are columns that need to be updated, well, update them.
// 
//     if(!m_weightDirty.isEmpty() && !manualResize())
//     {
//         calculateColumnWeights();
//         slotUpdateColumnWidths();
//     }
// 
//     K3ListView::viewportPaintEvent(pe);
// }
// 
// void Playlist::viewportResizeEvent(QResizeEvent *re)
// {
//     // If the width of the view has changed, manually update the column
//     // widths.
// 
//     if(re->size().width() != re->oldSize().width() && !manualResize())
//         slotUpdateColumnWidths();
// 
//     K3ListView::viewportResizeEvent(re);
// }
// 
// void Playlist::insertItem(Q3ListViewItem *item)
// {
//     // Because we're called from the PlaylistItem ctor, item may not be a
//     // PlaylistItem yet (it would be QListViewItem when being inserted.  But,
//     // it will be a PlaylistItem by the time it matters, but be careful if
//     // you need to use the PlaylistItem from here.
// 
//     m_addTime.append(static_cast<PlaylistItem *>(item));
//     K3ListView::insertItem(item);
// }
// 
// void Playlist::takeItem(Q3ListViewItem *item)
// {
//     // See the warning in Playlist::insertItem.
// 
//     m_subtractTime.append(static_cast<PlaylistItem *>(item));
//     K3ListView::takeItem(item);
// }
// 
// int Playlist::addColumn(const QString &label, int)
// {
//     int newIndex = K3ListView::addColumn(label, 30);
//     slotWeightDirty(newIndex);
//     return newIndex;
// }

// ### TODO: View
// void Playlist::updatePlaying() const
// {
//     foreach(const PlaylistItem *item, PlaylistItem::playingItems())
//         item->listView()->triggerUpdate();
// }

// ### TODO: View
// void Playlist::hideColumn(int c, bool updateSearch)
// {
//     foreach (QAction *action, m_headerMenu->actions()) {
//         if(!action)
//             continue;
// 
//         if (action->data().toInt() == c) {
//             action->setChecked(false);
//             break;
//         }
//     }
// 
//     if(!isColumnVisible(c))
//         return;
// 
//     setColumnWidthMode(c, Manual);
//     setColumnWidth(c, 0);
// 
//     // Moving the column to the end seems to prevent it from randomly
//     // popping up.
// 
//     header()->moveSection(c, header()->count());
//     header()->setResizeEnabled(false, c);
// 
//     if(c == m_leftColumn) {
//         updatePlaying();
//         m_leftColumn = leftMostVisibleColumn();
//     }
// 
//     if(!manualResize()) {
//         slotUpdateColumnWidths();
//         triggerUpdate();
//     }
// 
//     if(this != CollectionList::instance())
//         CollectionList::instance()->hideColumn(c, false);
// 
//     if(updateSearch)
//         redisplaySearch();
// }
// 
// void Playlist::showColumn(int c, bool updateSearch)
// {
//     foreach (QAction *action, m_headerMenu->actions()) {
//         if(!action)
//             continue;
// 
//         if (action->data().toInt() == c) {
//             action->setChecked(true);
//             break;
//         }
//     }
// 
//     if(isColumnVisible(c))
//         return;
// 
//     // Just set the width to one to mark the column as visible -- we'll update
//     // the real size in the next call.
// 
//     if(manualResize())
//         setColumnWidth(c, 35); // Make column at least slightly visible.
//     else
//         setColumnWidth(c, 1);
// 
//     header()->setResizeEnabled(true, c);
//     header()->moveSection(c, c); // Approximate old position
// 
//     if(c == leftMostVisibleColumn()) {
//         updatePlaying();
//         m_leftColumn = leftMostVisibleColumn();
//     }
// 
//     if(!manualResize()) {
//         slotUpdateColumnWidths();
//         triggerUpdate();
//     }
// 
//     if(this != CollectionList::instance())
//         CollectionList::instance()->showColumn(c, false);
// 
//     if(updateSearch)
//         redisplaySearch();
// }
// 
// bool Playlist::isColumnVisible(int c) const
// {
//     return columnWidth(c) != 0;
// }
// 
// void Playlist::slotInitialize()
// {

// 
//     setRenameable(PlaylistItem::TrackColumn, true);
//     setRenameable(PlaylistItem::ArtistColumn, true);
//     setRenameable(PlaylistItem::AlbumColumn, true);
//     setRenameable(PlaylistItem::TrackNumberColumn, true);
//     setRenameable(PlaylistItem::GenreColumn, true);
//     setRenameable(PlaylistItem::YearColumn, true);
// 
//     setAllColumnsShowFocus(true);
//     setSelectionMode(Q3ListView::Extended);
//     setShowSortIndicator(true);
//     setDropVisualizer(true);
// 
//     m_columnFixedWidths.resize(columns());
// 
//     //////////////////////////////////////////////////
//     // setup header RMB menu
//     //////////////////////////////////////////////////
// 
// 
//     connect(this, SIGNAL(contextMenuRequested(Q3ListViewItem*,QPoint,int)),
//             this, SLOT(slotShowRMBMenu(Q3ListViewItem*,QPoint,int)));
//     connect(this, SIGNAL(itemRenamed(Q3ListViewItem*,QString,int)),
//             this, SLOT(slotInlineEditDone(Q3ListViewItem*,QString,int)));
//     connect(this, SIGNAL(doubleClicked(Q3ListViewItem*)),
//             this, SLOT(slotPlayCurrent()));
//     connect(this, SIGNAL(returnPressed(Q3ListViewItem*)),
//             this, SLOT(slotPlayCurrent()));
// 
//     connect(header(), SIGNAL(sizeChange(int,int,int)),
//             this, SLOT(slotColumnSizeChanged(int,int,int)));
// 
//     connect(renameLineEdit(), SIGNAL(completionModeChanged(KGlobalSettings::Completion)),
//             this, SLOT(slotInlineCompletionModeChanged(KGlobalSettings::Completion)));
// 
//     connect(action("resizeColumnsManually"), SIGNAL(activated()),
//             this, SLOT(slotColumnResizeModeChanged()));
// 
//     if(action<KToggleAction>("resizeColumnsManually")->isChecked())
//         setHScrollBarMode(Auto);
//     else
//         setHScrollBarMode(AlwaysOff);
// 
//     setAcceptDrops(true);
//     setDropVisualizer(true);
// 
//     m_disableColumnWidthUpdates = false;
// 
//     setShowToolTips(false);
//     /* m_toolTip = new PlaylistToolTip(viewport(), this); */
// }

// ### TODO: View
// void Playlist::setup()
// {
//     setItemMargin(3);
// 
//     connect(header(), SIGNAL(indexChange(int,int,int)), this, SLOT(slotColumnOrderChanged(int,int,int)));
// 
//     connect(m_fetcher, SIGNAL(signalCoverChanged(int)), this, SLOT(slotCoverChanged(int)));
// 
//     // Prevent list of selected items from changing while internet search is in
//     // progress.
//     connect(this, SIGNAL(selectionChanged()), m_fetcher, SLOT(abortSearch()));
// 
//     setSorting(1);
// 
//     // This apparently must be created very early in initialization for other
//     // Playlist code requiring m_headerMenu.
//     m_columnVisibleAction = new KActionMenu(i18n("&Show Columns"), this);
//     ActionCollection::actions()->addAction("showColumns", m_columnVisibleAction);
// 
//     m_headerMenu = m_columnVisibleAction->menu();
// 
//     // TODO: Determine if other stuff in setup must happen before slotInitialize().
// 
//     // Explicitly call slotInitialize() so that the columns are added before
//     // SharedSettings::apply() sets the visible and hidden ones.
//     slotInitialize();
// }
//### TODO: View
// int Playlist::leftMostVisibleColumn() const
// {
//     int i = 0;
//     while(!isColumnVisible(header()->mapToSection(i)) && i < PlaylistItem::lastColumn())
//         i++;
// 
//     return header()->mapToSection(i);
// }

// PlaylistItemList Playlist::items(Q3ListViewItemIterator::IteratorFlag flags)
// {
//     return m_items;
//     PlaylistItemList list;
// 
//     for(Q3ListViewItemIterator it(this, flags); it.current(); ++it)
//         list.append(static_cast<PlaylistItem *>(it.current()));
// 
//     return list;
// }

// ### TODO: View
// void Playlist::calculateColumnWeights()
// {
//     if(m_disableColumnWidthUpdates)
//         return;
// 
//     PlaylistItemList l = items();
//     QList<int>::Iterator columnIt;
// 
//     QVector<double> averageWidth(columns());
//     double itemCount = l.size();
// 
//     QVector<int> cachedWidth;
// 
//     // Here we're not using a real average, but averaging the squares of the
//     // column widths and then using the square root of that value.  This gives
//     // a nice weighting to the longer columns without doing something arbitrary
//     // like adding a fixed amount of padding.
// 
//     foreach(PlaylistItem *item, l) {
//         cachedWidth = item->cachedWidths();
// 
//         // Extra columns start at 0, but those weights aren't shared with all
//         // items.
//         for(int i = 0; i < columnOffset(); ++i) {
//             averageWidth[i] +=
//                 std::pow(double(item->width(fontMetrics(), this, i)), 2.0) / itemCount;
//         }
// 
//         for(int column = columnOffset(); column < columns(); ++column) {
//             averageWidth[column] +=
//                 std::pow(double(cachedWidth[column - columnOffset()]), 2.0) / itemCount;
//         }
//     }
// 
//     if(m_columnWeights.isEmpty())
//         m_columnWeights.fill(-1, columns());
// 
//     foreach(int column, m_weightDirty) {
//         m_columnWeights[column] = int(std::sqrt(averageWidth[column]) + 0.5);
//     }
// 
//     m_weightDirty.clear();
// }

// ### TODO: View
// void Playlist::slotUpdateColumnWidths()
// {
//     if(m_disableColumnWidthUpdates || manualResize())
//         return;
// 
//     // Make sure that the column weights have been initialized before trying to
//     // update the columns.
// 
//     QList<int> visibleColumns;
//     for(int i = 0; i < columns(); i++) {
//         if(isColumnVisible(i))
//             visibleColumns.append(i);
//     }
// 
//     if(count() == 0) {
//         foreach(int column, visibleColumns)
//             setColumnWidth(column, header()->fontMetrics().width(header()->label(column)) + 10);
// 
//         return;
//     }
// 
//     if(m_columnWeights.isEmpty())
//         return;
// 
//     // First build a list of minimum widths based on the strings in the listview
//     // header.  We won't let the width of the column go below this width.
// 
//     QVector<int> minimumWidth(columns(), 0);
//     int minimumWidthTotal = 0;
// 
//     // Also build a list of either the minimum *or* the fixed width -- whichever is
//     // greater.
// 
//     QVector<int> minimumFixedWidth(columns(), 0);
//     int minimumFixedWidthTotal = 0;
// 
//     foreach(int column, visibleColumns) {
//         minimumWidth[column] = header()->fontMetrics().width(header()->label(column)) + 10;
//         minimumWidthTotal += minimumWidth[column];
// 
//         minimumFixedWidth[column] = qMax(minimumWidth[column], m_columnFixedWidths[column]);
//         minimumFixedWidthTotal += minimumFixedWidth[column];
//     }
// 
//     // Make sure that the width won't get any smaller than this.  We have to
//     // account for the scrollbar as well.  Since this method is called from the
//     // resize event this will set a pretty hard toLower bound on the size.
// 
//     setMinimumWidth(minimumWidthTotal + verticalScrollBar()->width());
// 
//     // If we've got enough room for the fixed widths (larger than the minimum
//     // widths) then instead use those for our "minimum widths".
// 
//     if(minimumFixedWidthTotal < visibleWidth()) {
//         minimumWidth = minimumFixedWidth;
//         minimumWidthTotal = minimumFixedWidthTotal;
//     }
// 
//     // We've got a list of columns "weights" based on some statistics gathered
//     // about the widths of the items in that column.  We need to find the total
//     // useful weight to use as a divisor for each column's weight.
// 
//     double totalWeight = 0;
//     foreach(int column, visibleColumns)
//         totalWeight += m_columnWeights[column];
// 
//     // Computed a "weighted width" for each visible column.  This would be the
//     // width if we didn't have to handle the cases of minimum and maximum widths.
// 
//     QVector<int> weightedWidth(columns(), 0);
//     foreach(int column, visibleColumns)
//         weightedWidth[column] = int(double(m_columnWeights[column]) / totalWeight * visibleWidth() + 0.5);
// 
//     // The "extra" width for each column.  This is the weighted width less the
//     // minimum width or zero if the minimum width is greater than the weighted
//     // width.
// 
//     QVector<int> extraWidth(columns(), 0);
// 
//     // This is used as an indicator if we have any columns where the weighted
//     // width is less than the minimum width.  If this is false then we can
//     // just use the weighted width with no problems, otherwise we have to
//     // "readjust" the widths.
// 
//     bool readjust = false;
// 
//     // If we have columns where the weighted width is less than the minimum width
//     // we need to steal that space from somewhere.  The amount that we need to
//     // steal is the "neededWidth".
// 
//     int neededWidth = 0;
// 
//     // While we're on the topic of stealing -- we have to have somewhere to steal
//     // from.  availableWidth is the sum of the amount of space beyond the minimum
//     // width that each column has been allocated -- the sum of the values of
//     // extraWidth[].
// 
//     int availableWidth = 0;
// 
//     // Fill in the values discussed above.
// 
//     foreach(int column, visibleColumns) {
//         if(weightedWidth[column] < minimumWidth[column]) {
//             readjust = true;
//             extraWidth[column] = 0;
//             neededWidth += minimumWidth[column] - weightedWidth[column];
//         }
//         else {
//             extraWidth[column] = weightedWidth[column] - minimumWidth[column];
//             availableWidth += extraWidth[column];
//         }
//     }
// 
//     // The adjustmentRatio is the amount of the "extraWidth[]" that columns will
//     // actually be given.
// 
//     double adjustmentRatio = (double(availableWidth) - double(neededWidth)) / double(availableWidth);
// 
//     // This will be the sum of the total space that we actually use.  Because of
//     // rounding error this won't be the exact available width.
// 
//     int usedWidth = 0;
// 
//     // Now set the actual column widths.  If the weighted widths are all greater
//     // than the minimum widths, just use those, otherwise use the "reajusted
//     // weighted width".
// 
//     foreach(int column, visibleColumns) {
//         int width;
//         if(readjust) {
//             int adjustedExtraWidth = int(double(extraWidth[column]) * adjustmentRatio + 0.5);
//             width = minimumWidth[column] + adjustedExtraWidth;
//         }
//         else
//             width = weightedWidth[column];
// 
//         setColumnWidth(column, width);
//         usedWidth += width;
//     }
// 
//     // Fill the remaining gap for a clean fit into the available space.
// 
//     int remainingWidth = visibleWidth() - usedWidth;
//     setColumnWidth(visibleColumns.back(), columnWidth(visibleColumns.back()) + remainingWidth);
// 
//     m_widthsDirty = false;
// }
// 
// void Playlist::slotAddToUpcoming()
// {
//     m_collection->setUpcomingPlaylistEnabled(true);
//     m_collection->upcomingPlaylist()->appendItems(selectedItems());
// }
// 
// void Playlist::slotRenameTag()
// {
//     // setup completions and validators
// 
//     CollectionList *list = CollectionList::instance();
// 
//     KLineEdit *edit = renameLineEdit();
// 
//     switch(m_currentColumn - columnOffset())
//     {
//     case PlaylistItem::ArtistColumn:
//         edit->completionObject()->setItems(list->uniqueSet(CollectionList::Artists));
//         break;
//     case PlaylistItem::AlbumColumn:
//         edit->completionObject()->setItems(list->uniqueSet(CollectionList::Albums));
//         break;
//     case PlaylistItem::GenreColumn:
//     {
//         QStringList genreList;
//         TagLib::StringList genres = TagLib::ID3v1::genreList();
//         for(TagLib::StringList::Iterator it = genres.begin(); it != genres.end(); ++it)
//             genreList.append(TStringToQString((*it)));
//         edit->completionObject()->setItems(genreList);
//         break;
//     }
//     default:
//         edit->completionObject()->clear();
//         break;
//     }
// 
//     m_editText = currentItem()->text(m_currentColumn);
// 
//     rename(currentItem(), m_currentColumn);
// }

// ### TODO: View
// void Playlist::slotInlineEditDone(Q3ListViewItem *, const QString &, int column)
// {
//     QString text = renameLineEdit()->text();
//     bool changed = false;
// 
//     PlaylistItemList l = selectedItems();
// 
//     // See if any of the files have a tag different from the input.
// 
//     for(PlaylistItemList::ConstIterator it = l.constBegin(); it != l.constEnd() && !changed; ++it)
//         if((*it)->text(column - columnOffset()) != text)
//             changed = true;
// 
//     if(!changed ||
//        (l.count() > 1 && KMessageBox::warningContinueCancel(
//            0,
//            i18n("This will edit multiple files. Are you sure?"),
//            QString(),
//            KGuiItem(i18n("Edit")),
//            KStandardGuiItem::cancel(),
//            "DontWarnMultipleTags") == KMessageBox::Cancel))
//     {
//         return;
//     }
// 
//     foreach(PlaylistItem *item, l)
//         editTag(item, text, column);
// 
//     TagTransactionManager::instance()->commit();
// 
//     CollectionList::instance()->dataChanged();
//     dataChanged();
//     update();
// }

// void Playlist::slotColumnOrderChanged(int, int from, int to)
// {
//     if(from == 0 || to == 0) {
//         updatePlaying();
//         m_leftColumn = header()->mapToSection(0);
//     }
// 
//     SharedSettings::instance()->setColumnOrder(this);
// }
// 
// void Playlist::slotCreateGroup()
// {
//     QString name = m_collection->playlistNameDialog(i18n("Create New Playlist"));
// 
//     if(!name.isEmpty())
//         new Playlist(m_collection, selectedItems(), name);
// }
// 
// void Playlist::notifyUserColumnWidthModeChanged()
// {
//     KMessageBox::information(this,
//                              i18n("Manual column widths have been enabled.  You can "
//                                   "switch back to automatic column sizes in the view "
//                                   "menu."),
//                              i18n("Manual Column Widths Enabled"),
//                              "ShowManualColumnWidthInformation");
// }
// 
// void Playlist::slotColumnSizeChanged(int column, int, int newSize)
// {
//     m_widthsDirty = true;
//     m_columnFixedWidths[column] = newSize;
// }
// 
// void Playlist::slotInlineCompletionModeChanged(KGlobalSettings::Completion mode)
// {
//     SharedSettings::instance()->setInlineCompletionMode(mode);
// }
// 
// void Playlist::slotPlayCurrent()
// {
//     Q3ListViewItemIterator it(this, Q3ListViewItemIterator::Selected);
//     PlaylistItem *next = static_cast<PlaylistItem *>(it.current());
//     TrackSequenceManager::instance()->setNextItem(next);
//     action("forward")->trigger();
// }


////////////////////////////////////////////////////////////////////////////////
// protected slots
////////////////////////////////////////////////////////////////////////////////

// void PlaylistView::slotPopulateBackMenu() const
// {
//     if(!playingItem())
//         return;
// 
//     QMenu *menu = action<KToolBarPopupAction>("back")->menu();
//     menu->clear();
//     m_backMenuItems.clear();
// 
//     int count = 0;
//     PlaylistItemList::ConstIterator it = m_history.constEnd();
// 
//     QAction *action;
// 
//     while(it != m_history.constBegin() && count < 10) {
//         ++count;
//         --it;
//         action = new QAction((*it)->file().tag()->title(), menu);
//         action->setData(count);
//         menu->addAction(action);
//         m_backMenuItems[count] = *it;
//     }
// }

// void PlaylistView::slotPlayFromBackMenu(QAction *backAction) const
// {
//     int number = backAction->data().toInt();
// 
//     if(!m_backMenuItems.contains(number))
//         return;
// 
//     TrackSequenceManager::instance()->setNextItem(m_backMenuItems[number]);
//     action("forward")->trigger();
// }

////////////////////////////////////////////////////////////////////////////////
// protected slots
////////////////////////////////////////////////////////////////////////////////

// void Playlist::slotPopulateBackMenu() const
// {
//     if(!playingItem())
//         return;
// 
//     QMenu *menu = action<KToolBarPopupAction>("back")->menu();
//     menu->clear();
//     m_backMenuItems.clear();
// 
//     int count = 0;
//     PlaylistItemList::ConstIterator it = m_history.constEnd();
// 
//     QAction *action;
// 
//     while(it != m_history.constBegin() && count < 10) {
//         ++count;
//         --it;
//         action = new QAction((*it)->file().tag()->title(), menu);
//         action->setData(count);
//         menu->addAction(action);
//         m_backMenuItems[count] = *it;
//     }
// }
// 
// void Playlist::slotPlayFromBackMenu(QAction *backAction) const
// {
//     int number = backAction->data().toInt();
// 
//     if(!m_backMenuItems.contains(number))
//         return;
// 
//     TrackSequenceManager::instance()->setNextItem(m_backMenuItems[number]);
//     action("forward")->trigger();
// }
// 

// void Playlist::playFirst()
// {
//     TrackSequenceManager::instance()->setNextItem(m_items.first());
//     action("forward")->trigger();
// }
// 
// void Playlist::playNextAlbum()
// {
//     PlaylistItem *current = TrackSequenceManager::instance()->currentItem();
//     if(!current)
//         return; // No next album if we're not already playing.
// 
//     QString currentAlbum = current->file().tag()->album();
//     current = TrackSequenceManager::instance()->nextItem();
// 
//     while(current && current->file().tag()->album() == currentAlbum)
//         current = TrackSequenceManager::instance()->nextItem();
// 
//     TrackSequenceManager::instance()->setNextItem(current);
//     action("forward")->trigger();
// }
// 
// void Playlist::playNext()
// {
//     TrackSequenceManager::instance()->setCurrentPlaylist(this);
//     setPlaying(TrackSequenceManager::instance()->nextItem());
// }
// 
// void Playlist::stop()
// {
//     m_history.clear();
//     setPlaying(0);
// }
// 
// void Playlist::playPrevious()
// {
//     if(!playingItem())
//         return;
// 
//     bool random = action("randomPlay") && action<KToggleAction>("randomPlay")->isChecked();
// 
//     PlaylistItem *previous = 0;
// 
//     if(random && !m_history.isEmpty()) {
//         PlaylistItemList::Iterator last = --m_history.end();
//         previous = *last;
//         m_history.erase(last);
//     }
//     else {
//         m_history.clear();
//         previous = TrackSequenceManager::instance()->previousItem();
//     }
// 
//     
//     if(!previous) {
//         int index = m_items.indexOf(playingItem()) - 1;
//         if (index < 0) index = m_items.size();
//         previous = m_items[index];
//     }
// 
//     setPlaying(previous, false);
// }