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

#ifndef _TAGTRANSACTIONMANAGER_H
#define _TAGTRANSACTIONMANAGER_H



class PlaylistItem;
class QWidget;
class Tag;

/**
 * Class to encapsulate a change to the tag, and optionally the file name, of
 * a PlaylistItem.
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 * @see TagTransactionManager
 */
class TagTransactionAtom
{
    public:
    /**
     * Default constructor, for use by QValueList.
     */
    TagTransactionAtom();

    /**
     * Copy constructor.  This takes ownership of the m_tag pointer, so the
     * object being copied no longer has access to the tag.  This function also
     * exists mainly for QValueList's benefit.
     *
     * @param other The TagTransactionAtom to copy.
     */
    TagTransactionAtom(const TagTransactionAtom &other);

    /**
     * Creates an atom detailing a change made by \p tag to \p item.
     *
     * @param tag Contains the new tag to apply to item.
     * @param item The PlaylistItem to change.
     */
    TagTransactionAtom(PlaylistItem *item, Tag *tag);

    /**
     * Destroys the atom.  This function deletes the tag, so make sure you've
     * already copied out any data you need.  The PlaylistItem is unaffected.
     */
    ~TagTransactionAtom();

    /**
     * Assignment operator.  This operator takes ownership of the m_tag pointer,
     * so the object being assigned from no longer has access to the tag.  This
     * function exists mainly for the benefit of QValueList.
     *
     * @param other The TagTransactionAtom to copy from.
     * @return The TagTransactionAtom being assigned to.
     */
    TagTransactionAtom &operator=(const TagTransactionAtom &other);

    /**
     * Accessor function to retrieve the PlaylistItem.
     *
     * @return The PlaylistItem being changed.
     */
    PlaylistItem *item() const { return m_item; }

    /**
     * Accessor function to retrieve the changed Tag.
     *
     * @return The Tag containing the changes to apply to item().
     */
    Tag *tag() const { return m_tag; }

    private:
    PlaylistItem *m_item;
    mutable Tag *m_tag;
};

typedef QValueList<TagTransactionAtom> TagAlterationList;

/**
 * This class manages alterations of a group of PlaylistItem's FileHandles.  What this
 * means in practice is that you will use this class to change the tags and/or
 * filename of a PlaylistItem.
 *
 * This class supports a limited transactional interface.  Once you commit a
 * group of changes, you can call the undo() method to revert back to the way
 * things were (except possibly for file renames).  You can call forget() to
 * forget a series of changes as well.
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 */
class TagTransactionManager : public QObject
{
    Q_OBJECT

    public:
    /**
     * Constructs a TagTransactionManager, owned by @p parent.
     *
     * @param parent The parent QWidget.
     */
    TagTransactionManager(QWidget *parent = 0);

    /**
     * Returns the global TagTransactionManager instance.
     *
     * @return The global TagTransactionManager.
     */
    static TagTransactionManager *instance();

    /**
     * Adds a change to the list of changes to apply.  Internally this
     * function extracts the CollectionListItem of @p item, and uses that
     * instead, so there is no need to do so yourself.
     *
     * @param item The PlaylistItem to change.
     * @param newTag The Tag containing the changed data.
     */
    void changeTagOnItem(PlaylistItem *item, Tag *newTag);

    /**
     * Convienience function to duplicate a Tag object, since the Tag
     * object doesn't have a decent copy constructor.
     *
     * @param tag The Tag to duplicate.
     * @param fileName The filename to assign to the tag.  If QString::null
     *        (the default) is passed, the filename of the existing tag is
     *        used.
     * @bug Tag should have a correct copy ctor and assignment operator.
     * @return The duplicate Tag.
     */
    static Tag *duplicateTag(const Tag *tag, const QString &fileName = QString::null);

    /**
     * Commits the changes to the PlaylistItems.  It is important that the
     * PlaylistItems still exist when you call this function, although this
     * shouldn't be a problem in practice.  After altering the tags, and
     * renaming the files if necessary, you can call undo() to back out the
     * changes.
     *
     * If any errors have occurred, the user will be notified with a dialog
     * box, and those files which were unabled to be altered will be excluded
     * from the undo set.
     *
     * @return true if no errors occurred, false otherwise.
     */
    bool commit();

    /**
     * Clears the current update list.  The current undo list is unaffected.
     */
    void forget();

    /**
     * Undoes the changes caused by commit().  Like commit(), if any errors
     * occur changing the state back (for example, it may be impossible to
     * rename a file back to its original name), the user will be shown notified
     * via a dialog box.
     *
     * After performing the undo operation, it is impossible to call undo()
     * again on the same set of files.  Namely, you can't repeatedly call
     * undo() to switch between two different file states.
     *
     * @return true if no errors occurred, false otherwise.
     */
    bool undo();

    signals:
    void signalAboutToModifyTags();
    void signalDoneModifyingTags();

    private:
    /**
     * Renames the file identified by @p from to have the name given by @p to,
     * prompting the user to confirm if necessary.
     *
     * @param from QFileInfo with the filename of the original file.
     * @param to QFileInfo with the new filename.
     * @return true if no errors occurred, false otherwise.
     */
    bool renameFile(const QFileInfo &from, const QFileInfo &to) const;

    /**
     * Used internally by commit() and undo().  Performs the work of updating
     * the PlaylistItems and then updating the various GUI elements that need
     * to be updated.
     *
     * @param undo true if operating in undo mode, false otherwise.
     */
    bool processChangeList(bool undo = false);

    TagAlterationList m_list; ///< holds a list of changes to commit
    TagAlterationList m_undoList; ///< holds a list of changes to undo
    static TagTransactionManager *m_manager; ///< used by instance()
};

#endif /* _TAGTRANSACTIONMANAGER_H */

// vim: set et ts=4 sw=4 tw=0:
