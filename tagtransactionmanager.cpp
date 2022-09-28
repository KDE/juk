/**
 * Copyright (C) 2004 Michael Pyne <mpyne@kde.org>
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

#include "tagtransactionmanager.h"

#include <KMessageBox>
#include <KLocalizedString>

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QFileInfo>

#include "playlistitem.h"
#include "collectionlist.h"
#include "juktag.h"
#include "actioncollection.h"
#include "juk_debug.h"

using ActionCollection::action;

Q_GLOBAL_STATIC(TagTransactionManager, g_tagManager)

TagTransactionManager *TagTransactionManager::instance()
{
    return g_tagManager;
}

TagTransactionAtom::TagTransactionAtom(PlaylistItem *item, Tag *tag)
    : m_item(item)
    , m_tag(tag)
{
}

void TagTransactionManager::changeTagOnItem(PlaylistItem *item, Tag *newTag)
{
    if(!item) {
        qCWarning(JUK_LOG) << "Trying to change tag on null PlaylistItem.\n";
        return;
    }

    // Save the CollectionListItem, as it is the most likely to survive long
    // enough for the commit().  I should probably intercept the item deleted
    // signals from CollectionList to ensure that the commit list and the
    // playlists stay in sync.

    m_list.emplace_back(item->collectionItem(), newTag);
}

Tag *TagTransactionManager::duplicateTag(const Tag *tag, const QString &fileName)
{
    Q_ASSERT(tag);

    QString name = fileName.isEmpty() ? tag->fileName() : fileName;
    Tag *newTag = new Tag(*tag);

    newTag->setFileName(name);
    return newTag;
}

bool TagTransactionManager::commit()
{
    m_undoList.clear();
    bool result = processChangeList();

    m_list.clear();
    return result;
}

void TagTransactionManager::forget()
{
    m_list.clear();
}

bool TagTransactionManager::undo()
{
    qCDebug(JUK_LOG) << "Undoing " << m_undoList.size() << " changes.\n";

    forget();  // Scrap our old changes (although the list should be empty
               // anyways.

    bool result = processChangeList(true);

    m_undoList.clear();
    action("edit_undo")->setEnabled(false);

    return result;
}

bool TagTransactionManager::renameFile(const QFileInfo &from, const QFileInfo &to) const
{
   if(!QFileInfo(to.path()).isWritable() || !from.exists())
       return false;

   if(!to.exists() ||
       KMessageBox::warningContinueCancel(
           static_cast<QWidget *>(parent()),
           i18n("This file already exists.\nDo you want to replace it?"),
           i18n("File Exists"),KGuiItem(i18n("Replace"))) == KMessageBox::Continue)
   {
       qCDebug(JUK_LOG) << "Renaming " << from.absoluteFilePath() << " to " << to.absoluteFilePath();
       QDir currentDir;
       return currentDir.rename(from.absoluteFilePath(), to.absoluteFilePath());
   }

   return false;
}

bool TagTransactionManager::processChangeList(bool undo)
{
    TagAlterationList::const_iterator it, end;
    QStringList errorItems;

    it = undo ? m_undoList.cbegin() : m_list.cbegin();
    end = undo ? m_undoList.cend() : m_list.cend();

    emit signalAboutToModifyTags();

    for(; it != end; ++it) {
        PlaylistItem *item = (*it).item();
        const Tag *tag = (*it).tag();

        QFileInfo newFile(tag->fileName());

        if(item->file().fileInfo().fileName() != newFile.fileName()) {
            if(!renameFile(item->file().fileInfo(), newFile)) {
                errorItems.append(item->text(1) + QString(" - ") + item->text(0));
                continue;
            }
        }

        if(tag->save()) {
            if(!undo)
                m_undoList.emplace_back(item, duplicateTag(item->file().tag()));

            item->setFile(tag->fileName());
            item->refreshFromDisk();
            //FIXME repaint
            //item->repaint();
            item->playlist()->playlistItemsChanged();
            item->playlist()->update();
        }
        else {
            Tag *errorTag = item->file().tag();
            QString str = errorTag->artist() + " - " + errorTag->title();

            if(errorTag->artist().isEmpty())
                str = errorTag->title();

            errorItems.append(str);
        }

        qApp->processEvents();
    }

    undo ? m_undoList.clear() : m_list.clear();
    if(!undo && !m_undoList.empty())
        action("edit_undo")->setEnabled(true);
    else
        action("edit_undo")->setEnabled(false);

    if(!errorItems.isEmpty())
        KMessageBox::errorList(static_cast<QWidget *>(parent()),
                i18n("The following files were unable to be changed."),
                errorItems,
                i18n("Error"));

    emit signalDoneModifyingTags();
    return errorItems.isEmpty();
}

// vim: set et sw=4 tw=0 sta:
