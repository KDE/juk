/***************************************************************************
    begin                : Thu Oct 28 2004
    copyright            : (C) 2004 by Michael Pyne
                         : (C) 2003 Frerich Raabe <raabe@kde.org>
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

#ifndef JUK_FILERENAMER_H
#define JUK_FILERENAMER_H

#include <qstring.h>

#include "filerenamerbase.h"
#include "filerenameroptions.h"
#include "categoryreaderinterface.h"
#include "tagrenameroptions.h"
#include "playlistitem.h"

// Predeclare some classes

namespace TagLib {
    class FileRef;
};

class ExampleOptionsDialog;
class QCheckBox;
class QLayout;
class QLayoutItem;
class QPushButton;
class QVBox;
class PlaylistItem;

// Used to decide what direction the FileRenamerWidget will move rows in.
enum MovementDirection { MoveUp, MoveDown };

/**
 * This is used by FileRenamerWidget to store information about a particular
 * tag type, including its position, the QFrame holding the information,
 * the up, down, and enable buttons, and the user-selected renaming options.
 */
struct Row
{
    Row() : widget(0), upButton(0), downButton(0), enableButton(0) {}

    QWidget *widget;
    QPushButton *upButton, *downButton;
    QCheckBox *enableButton;
    TagRenamerOptions options;
    TagType category;
    QString name;
};

/**
 * An implementation of CategoryReaderInterface that reads the user's settings
 * from the global KConfig configuration object, and reads track information
 * from whatever the given PlaylistItem is.  You can assign different
 * PlaylistItems in order to change the returned tag category information.
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 */
class ConfigCategoryReader : public CategoryReaderInterface
{
public:
    // ConfigCategoryReader specific members

    ConfigCategoryReader();

    const PlaylistItem *playlistItem() const { return m_currentItem; }
    void setPlaylistItem(const PlaylistItem *item) { m_currentItem = item; }

    // CategoryReaderInterface reimplementations

    virtual QString categoryValue(TagType type) const;
    virtual QString prefix(TagType category) const;
    virtual QString suffix(TagType category) const;
    virtual TagRenamerOptions::EmptyActions emptyAction(TagType category) const;
    virtual QString emptyText(TagType category) const;
    virtual QValueList<TagType> categoryOrder() const;
    virtual QString separator() const;
    virtual QString musicFolder() const;
    virtual int trackWidth() const;
    virtual bool hasFolderSeparator(int index) const;
    virtual bool isDisabled(TagType category) const;

private:
    const PlaylistItem *m_currentItem;
    TagRenamerOptions m_options[NumTypes];
    QValueList<TagType> m_categoryOrder;
    QString m_separator;
    QString m_musicFolder;
    bool m_folderSeparators[NumTypes - 1];
};

/**
 * This class implements a dialog that allows the user to alter the behavior
 * of the file renamer.  It supports 6 different genre types at this point,
 * and it shouldn't be too difficult to extend that in the future if needed.
 * It allows the user to open an external dialog, which will let the user see
 * an example of what their current options will look like, by either allowing
 * the user to type in some sample information, or by loading a file and
 * reading tags from there.
 *
 * It also implements the CategoryReaderInterface in order to implement the
 * example filename functionality.
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 */
class FileRenamerWidget : public FileRenamerBase, public CategoryReaderInterface
{
    Q_OBJECT

public:
    FileRenamerWidget(QWidget *parent);
    ~FileRenamerWidget();

    /**
     * This function saves all of the category options to the global KConfig
     * object.  You must call this manually, FileRenamerWidget doesn't call it
     * automatically so that situations where the user hits "Cancel" work
     * correctly.
     */
    void saveConfig();

protected slots:
    /**
     * This function should be called whenever the example text may need to be
     * changed.  For example, when the user selects a different separator or
     * changes the example text, this slot should be called.
     */
    virtual void exampleTextChanged();

    /**
     * This function shows the example dialog if it is hidden, and hides the
     * example dialog if it is shown.
     */
    virtual void toggleExampleDialog();

private:
    /**
     * This function initializes the category options by loading the data from
     * the global KConfig object.  This is called automatically in the constructor.
     */
    void loadConfig();

    /**
     * This function sets up the internal view by creating the checkboxes and
     * the rows for each category.
     */
    void createTagRows();

    /**
     * This function returns the container widget that holds the row for
     * \p category.
     *
     * @param category the category to retrieve the widget for.
     * @return the widget holding the row for the category.
     */
    QWidget *widgetForCategory(TagType category) const
    {
        return m_rows[findIndex(category)].widget;
    }

    /**
     * Returns the value for \p category by retrieving the tag from m_exampleFile.
     * If \p category is Track, then an appropriate fixup will be applied if needed
     * to match the user's desired minimum width.
     *
     * @param category the category to retrieve the value for.
     * @return the string representation of the value for \p category.
     */
    QString fileCategoryValue(TagType category) const;

    /**
     * Returns the value for \p category by reading the user entry for that
     * category. If \p category is Track, then an appropriate fixup will be applied
     * if needed to match the user's desired minimum width.
     *
     * @param category the category to retrieve the value for.
     * @return the string representation of the value for \p category.
     */
    virtual QString categoryValue(TagType category) const;

    /**
     * Returns the user-specified prefix string for \p category.
     *
     * @param category the category to retrieve the value for.
     * @return user-specified prefix string for \p category.
     */
    virtual QString prefix(TagType category) const
    {
        return m_rows[findIndex(category)].options.prefix();
    }

    /**
     * Returns the user-specified suffix string for \p category.
     *
     * @param category the category to retrieve the value for.
     * @return user-specified suffix string for \p category.
     */
    virtual QString suffix(TagType category) const
    {
        return m_rows[findIndex(category)].options.suffix();
    }

    /**
     * Returns the user-specified empty action for \p category.
     *
     * @param category the category to retrieve the value for.
     * @return user-specified empty action for \p category.
     */
    virtual TagRenamerOptions::EmptyActions emptyAction(TagType category) const
    {
        return m_rows[findIndex(category)].options.emptyAction();
    }

    /**
     * Returns the user-specified empty text for \p category.  This text might
     * be used to replace an empty value.
     *
     * @param category the category to retrieve the value for.
     * @return the user-specified empty text for \p category.
     */
    virtual QString emptyText(TagType category) const
    {
        return m_rows[findIndex(category)].options.emptyText();
    }

    /**
     * @return list of TagTypes corresponding to the user-specified category order.
     */
    virtual QValueList<TagType> categoryOrder() const;

    /**
     * @return string that separates the tag values in the file name.
     */
    virtual QString separator() const;

    /**
     * @return local path to the music folder used to store renamed files.
     */
    virtual QString musicFolder() const;

    /**
     * @return the minimum width of the track category.
     */
    virtual int trackWidth() const
    {
        return m_rows[findIndex(Track)].options.trackWidth();
    }
    
    /**
     * @param  index, the 0-based index for the folder boundary.
     * @return true if there should be a folder separator between category
     *         index and index + 1, and false otherwise.  Note that for purposes
     *         of this function, only categories that are required or non-empty
     *         should count.
     */
    virtual bool hasFolderSeparator(int index) const;

    /**
     * @param category The category to get the status of.
     * @return true if \p category is disabled by the user, and false otherwise.
     */
    virtual bool isDisabled(TagType category) const
    {
        return m_rows[findIndex(category)].options.disabled();
    }

    /**
     * This moves the widget \p l in the direction given by \p direction, taking
     * care to make sure that the checkboxes are not moved, and that they are
     * enabled or disabled as appropriate for the new layout, and that the up and
     * down buttons are also adjusted as necessary.
     *
     * @param l the widget to move
     * @param direction the direction to move
     */
    void moveItem(QWidget *l, MovementDirection direction);

    /**
     * This function actually performs the work of showing the options dialog for
     * \p category.
     *
     * @param category the category to show the options dialog for.
     */
    void showCategoryOptions(TagType category);

    /**
     * This function enables or disables the widget in the row indexed by \p index,
     * controlled by \p enable.  This function also makes sure that checkboxes are
     * enabled or disabled as appropriate if they no longer make sense due to the
     * adjacent category being enabled or disabled.
     *
     * @param index the index of the row to change.  This is *not* the category to
     *        change.
     * @param enable enables the category if true, disables if false.
     */
    void setCategoryEnabled(int index, bool enable);

    /**
     * This function enables all of the up buttons.
     */
    void enableAllUpButtons();

    /**
     * This function enables all of the down buttons.
     */
    void enableAllDownButtons();

    /**
     * This function returns the position in the m_rows index that contains
     * \p item.
     *
     * @param item the widget to find.
     * @return the indexed position of the widget, or -1 if it couldn't be found.
     */
    int findIndex(QWidget *item) const;

    /**
     * This function returns the position in the m_rows index that contains
     * \p category.
     *
     * @param category the category to find.
     * @return the indexed position of the category, or -1 if it couldn't be found.
     */
    int findIndex(TagType category) const;

private slots:
    /**
     * This function reads the tags from \p file and ensures that the dialog will
     * use those tags until a different file is selected or dataSelected() is
     * called.
     *
     * @param file the path to the local file to read.
     */
    virtual void fileSelected(const QString &file);

    /**
     * This function reads the tags from the user-supplied examples and ensures
     * that the dialog will use those tags until a file is selected using
     * fileSelected().
     */
    virtual void dataSelected();

    /**
     * This function brings up a dialog that allows the user to edit the options
     * for \p category.  \p category is an int so make using it with QSignalMapper
     * easier.
     *
     * @param category the category to bring up the options for.
     */
    virtual void showCategoryOption(int category);

    /**
     * This function updates the information for the given category to match
     * whatever state its match check box is in.  This roundabout way is done due
     * to QSignalMapper, and the call is forwarded to setCategoryEnabled().
     *
     * @param category The category to update
     */
    virtual void toggleCategory(int category);

    /**
     * This function moves \p category up in the layout.
     *
     * @param category the category of the widget to move up.
     */
    virtual void moveItemUp(int category);

    /**
     * This function moves \p category down in the layout.
     *
     * @param category the category of the widget to move down.
     */
    virtual void moveItemDown(int category);

    /**
     * This slot should be called whenever the example input dialog is shown.
     */
    virtual void exampleDialogShown();

    /**
     * This slot should be called whever the example input dialog is hidden.
     */
    virtual void exampleDialogHidden();

private:
    /// This is the frame that holds all of the category widgets and checkboxes.
    QVBox *m_mainFrame;

    /**
     * This array should always be accessed using integer indices, as the index
     * represents the physical position of each entry.  m_rows[0] is the top entry,
     * and m_rows[NumTypes - 1] is the bottom.  You can use findIndex() to get the
     * m_rows index for a given category.
     */
    Row m_rows[NumTypes];

    /** 
     * This holds an array of checkboxes that allow the user to insert folder
     * separators in between categories.
     */
    QCheckBox *m_folderSwitches[NumTypes - 1];

    ExampleOptionsDialog *m_exampleDialog;

    /// This is true if we're reading example tags from m_exampleFile.
    bool m_exampleFromFile;
    QString m_exampleFile;
};

class PlaylistItem;

class FileRenamer
{
public:
    FileRenamer();

    void rename(PlaylistItem *item);
    void rename(const PlaylistItemList &items);

    static QString fileName(const CategoryReaderInterface &interface);

    /**
     * Adjusts \p track if necessary in order to make it at least the minimum
     * number of characters specified by the user.
     *
     * @param track the string representation of the track value.
     * @param interface the CategoryReaderInterface to read information from.
     * @return track padding with zeroes so that it fits the user's requirements
     *         for minimum width.
     */
    static QString fixupTrack(const QString &track, const CategoryReaderInterface &interface);

private:
    /**
     * Attempts to rename the file from \a src to \a dest.  Returns true if the
     * operation succeeded.
     */
    bool moveFile(const QString &src, const QString &dest);
};

#endif /* JUK_FILERENAMER_H */

// vim: set et sw=4 ts=8:
