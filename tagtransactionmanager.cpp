/***************************************************************************
    begin                : Wed Sep 22 2004
    copyright            : (C) 2004 by Michael Pyne
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

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kapplication.h>

#include <qfileinfo.h>
#include <qdir.h>

#include "tagtransactionmanager.h"
#include "playlistitem.h"
#include "collectionlist.h"
#include "tag.h"
#include "actioncollection.h"

using ActionCollection::action;

TagTransactionManager *TagTransactionManager::m_manager = 0;

TagTransactionAtom::TagTransactionAtom() : m_item(0), m_tag(0)
{
    action("edit_undo")->setEnabled(false);
}

TagTransactionAtom::TagTransactionAtom(const TagTransactionAtom &other) :
    m_item(other.m_item), m_tag(other.m_tag)
{
    other.m_tag = 0; // Only allow one owner
}

TagTransactionAtom::TagTransactionAtom(PlaylistItem *item, Tag *tag) :
    m_item(item), m_tag(tag)
{
}

TagTransactionAtom::~TagTransactionAtom()
{
    delete m_tag;
}

TagTransactionAtom &TagTransactionAtom::operator=(const TagTransactionAtom &other)
{
    m_item = other.m_item;
    m_tag = other.m_tag;

    other.m_tag = 0; // Only allow one owner

    return *this;
}

TagTransactionManager *TagTransactionManager::instance()
{
    return m_manager;
}

void TagTransactionManager::changeTagOnItem(PlaylistItem *item, Tag *newTag)
{
    if(!item) {
        kdWarning(65432) << "Trying to change tag on null PlaylistItem.\n";
        return;
    }

    // Save the CollectionListItem, as it is the most likely to survive long
    // enough for the commit().  I should probably intercept the item deleted
    // signals from CollectionList to ensure that the commit list and the
    // playlists stay in sync.

    m_list.append(TagTransactionAtom(item->collectionItem(), newTag));
}

Tag *TagTransactionManager::duplicateTag(const Tag *tag, const QString &fileName)
{
    QString name = fileName.isNull() ? tag->fileName() : fileName;
    Tag *newTag = new Tag(name, false);

    newTag->setTitle(tag->title());
    newTag->setArtist(tag->artist());
    newTag->setAlbum(tag->album());
    newTag->setGenre(tag->genre());
    newTag->setTrack(tag->track());
    newTag->setYear(tag->year());
    newTag->setComment(tag->comment());

    return newTag;
}

bool TagTransactionManager::commit()
{
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
    kdDebug(65432) << "Undoing " << m_undoList.count() << " changes.\n";
    bool result = processChangeList(true);

    m_undoList.clear();
    action("edit_undo")->setEnabled(false);

    return result;
}

TagTransactionManager::TagTransactionManager(QWidget *parent) : QObject(parent, "tagmanager")
{
    m_manager = this;
}

bool TagTransactionManager::renameFile(const QFileInfo &from, const QFileInfo &to) const
{
   if(!QFileInfo(to.dirPath()).isWritable() || !from.exists())
       return false;

   if(!to.exists() ||
       KMessageBox::warningYesNo(
           static_cast<QWidget *>(parent()),
           i18n("This file already exists.\nDo you want to replace it?"),
           i18n("File Exists")) == KMessageBox::Yes)
   {
       kdDebug(65432) << "Renaming " << from.absFilePath() << " to " << to.absFilePath() << endl;
       QDir currentDir;
       return currentDir.rename(from.absFilePath(), to.absFilePath());
   }

   return false;
}

bool TagTransactionManager::processChangeList(bool undo)
{
    TagAlterationList::ConstIterator it, end;
    QStringList errorItems;

    it = undo ? m_undoList.begin() : m_list.begin();
    end = undo ? m_undoList.end() : m_list.end();

    emit signalAboutToModifyTags();

    for(; it != end; ++it) {
        PlaylistItem *item = (*it).item();
        Tag *tag = (*it).tag();

        QFileInfo newFile(tag->fileName());

        if(item->file().fileInfo().fileName() != newFile.fileName()) {
            if(!renameFile(item->file().fileInfo(), newFile)) {
                errorItems.append(item->text(1) + QString(" - ") + item->text(0));
                continue;
            }
        }

        if(tag->save()) {
            if(!undo)
                m_undoList.append(TagTransactionAtom(item, duplicateTag(item->file().tag())));

            item->file().setFile(tag->fileName());
            item->refreshFromDisk();
            item->repaint();
            item->playlist()->dataChanged();
            item->playlist()->update();
        }
        else {
            Tag *errorTag = item->file().tag();
            QString str = errorTag->artist() + " - " + errorTag->track();

            if(errorTag->artist().isEmpty())
                str = errorTag->track();

            errorItems.append(str);
        }

        kapp->processEvents();
    }

    undo ? m_undoList.clear() : m_list.clear();
    if(!undo && !m_undoList.isEmpty())
        action("edit_undo")->setEnabled(true);
    else
        action("edit_undo")->setEnabled(false);

    if(!errorItems.isEmpty())
        KMessageBox::informationList(static_cast<QWidget *>(parent()),
                i18n("The following files were unable to be changed."),
                errorItems,
                i18n("Error"));

    emit signalDoneModifyingTags();
    return errorItems.isEmpty();
}

#include "tagtransactionmanager.moc"

// vim: set et ts=4 sw=4 tw=0:
