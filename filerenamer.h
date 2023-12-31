/**
 * Copyright (C) 2004, 2007 Michael Pyne <mpyne@kde.org>
 * Copyright (C) 2003 Frerich Raabe <raabe@kde.org>
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

#ifndef JUK_FILERENAMER_H
#define JUK_FILERENAMER_H

#include <QString>
#include <QVector>
#include <QMap>

#include "ui_filerenamerbase.h"
#include "categoryreaderinterface.h"
#include "tagrenameroptions.h"

class QCheckBox;
class QPushButton;
class QUrl;

class ExampleOptionsDialog;
class PlaylistItem;

typedef QVector<PlaylistItem *> PlaylistItemList;

// Used to decide what direction the FileRenamerWidget will move rows in.
enum MovementDirection { MoveUp, MoveDown };

/**
 * This is used by FileRenamerWidget to store information about a particular
 * tag type, including its position, the QFrame holding the information,
 * the up, down, and enable buttons, and the user-selected renaming options.
 */
struct Row final
{
    QWidget *widget = nullptr;

    QPushButton *upButton      = nullptr;
    QPushButton *downButton    = nullptr;
    QPushButton *optionsButton = nullptr;
    QPushButton *enableButton  = nullptr;

    TagRenamerOptions options;
    CategoryID category; // Includes category and a disambiguation id.
    int position; ///< Position in the GUI (0 == top)
    QString name;
};

/**
 * A list of rows, each of which may have its own category options and other
 * associated data.  There is no relation between the order of rows in the vector and their
 * GUI layout.  Instead, each Row has a position member which indicates what GUI position it
 * takes up.  The index into the vector is known as the row identifier (which is unique but
 * not necessarily constant).
 */
typedef QVector<Row> Rows;

/**
 * Associates a CategoryID combination with a set of options.
 *
 * Used for ConfigCategoryReader
 */
typedef QMap<CategoryID, TagRenamerOptions> CategoryOptionsMap;

/**
 * An implementation of CategoryReaderInterface that reads the user's settings
 * from the global KConfig configuration object, and reads track information
 * from whatever the given PlaylistItem is.  You can assign different
 * PlaylistItems in order to change the returned tag category information.
 *
 * @author Michael Pyne <mpyne@kde.org>
 */
class ConfigCategoryReader final : public CategoryReaderInterface
{
public:
    // ConfigCategoryReader specific members

    ConfigCategoryReader();

    const PlaylistItem *playlistItem() const { return m_currentItem; }
    void setPlaylistItem(const PlaylistItem *item) { m_currentItem = item; }

    // CategoryReaderInterface reimplementations

    virtual QString categoryValue(TagType type) const override;
    virtual QString prefix(const CategoryID &category) const override;
    virtual QString suffix(const CategoryID &category) const override;
    virtual TagRenamerOptions::EmptyActions emptyAction(const CategoryID &category) const override;
    virtual QString emptyText(const CategoryID &category) const override;
    virtual QList<CategoryID> categoryOrder() const override;
    virtual QString separator() const override;
    virtual QString musicFolder() const override;
    virtual int trackWidth(int categoryNum) const override;
    virtual bool hasFolderSeparator(int index) const override;
    virtual bool isDisabled(const CategoryID &category) const override;

private:
    const PlaylistItem *m_currentItem = nullptr;
    CategoryOptionsMap m_options;
    QList<CategoryID> m_categoryOrder;
    QString m_separator;
    QString m_musicFolder;
    QVector<bool> m_folderSeparators;
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
 * @author Michael Pyne <mpyne@kde.org>
 */
class FileRenamerWidget final : public QWidget, public CategoryReaderInterface
{
    Q_OBJECT

public:
    explicit FileRenamerWidget(QWidget *parent);
    ~FileRenamerWidget();

    /// Maximum number of total categories the widget will allow.
    static int const MAX_CATEGORIES = 16;

    /**
     * This function saves all of the category options to the global KConfig
     * object.  You must call this manually, FileRenamerWidget doesn't call it
     * automatically so that situations where the user hits "Cancel" work
     * correctly.
     */
    void saveConfig();

signals:
    void accepted(); // for the QDialogButtonBox
    void rejected();

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

    /**
     * This function inserts the currently selected category, so that the
     * user can use duplicate tags in the file renamer.
     */
    virtual void insertCategory();

private:
    /**
     * This function initializes the category options by loading the data from
     * the global KConfig object.  This is called automatically in the constructor.
     */
    void loadConfig();

    /**
     * This function adds a "Insert Folder separator" checkbox to the end of
     * the current layout.  The setting defaults to being unchecked.
     */
    void addFolderSeparatorCheckbox();

    /**
     * This function creates a row in the main view for category, appending it
     * to the end.  It handles connecting signals to the mapper and such as
     * well.
     *
     * @param category Type of row to append.
     * @return identifier of newly added row.
     */
    int addRowCategory(TagType category);

    /**
     * Removes the given row, updating the other rows to have the correct
     * number of categoryNumber.
     *
     * @param id The identifier of the row to remove.
     * @return true if the delete succeeded, false otherwise.
     */
    bool removeRow(int id);

    /**
     * Installs button signal handlers for the buttons in @p row so that they
     * are called in response to GUI events, and removes any existing handlers.
     */
    void assignPositionHandlerForRow(Row &row);

    /**
     * This function sets up the internal view by creating the checkboxes and
     * the rows for each category.
     */
    void createTagRows();

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
    virtual QString categoryValue(TagType category) const override;

    /**
     * Returns the user-specified prefix string for \p category.
     *
     * @param category the category to retrieve the value for.
     * @return user-specified prefix string for \p category.
     */
    virtual QString prefix(const CategoryID &category) const override
    {
        return m_rows[findIdentifier(category)].options.prefix();
    }

    /**
     * Returns the user-specified suffix string for \p category.
     *
     * @param category the category to retrieve the value for.
     * @return user-specified suffix string for \p category.
     */
    virtual QString suffix(const CategoryID &category) const override
    {
        return m_rows[findIdentifier(category)].options.suffix();
    }

    /**
     * Returns the user-specified empty action for \p category.
     *
     * @param category the category to retrieve the value for.
     * @return user-specified empty action for \p category.
     */
    virtual TagRenamerOptions::EmptyActions emptyAction(const CategoryID &category) const override
    {
        return m_rows[findIdentifier(category)].options.emptyAction();
    }

    /**
     * Returns the user-specified empty text for \p category.  This text might
     * be used to replace an empty value.
     *
     * @param category the category to retrieve the value for.
     * @return the user-specified empty text for \p category.
     */
    virtual QString emptyText(const CategoryID &category) const override
    {
        return m_rows[findIdentifier(category)].options.emptyText();
    }

    /**
     * @return list of CategoryIDs corresponding to the user-specified category order.
     */
    virtual QList<CategoryID> categoryOrder() const override;

    /**
     * @return string that separates the tag values in the file name.
     */
    virtual QString separator() const override;

    /**
     * @return local path to the music folder used to store renamed files.
     */
    virtual QString musicFolder() const override;

    /**
     * @param categoryNum Zero-based number of category to get results for (if more than one).
     * @return the minimum width of the track category.
     */
    virtual int trackWidth(int categoryNum) const override
    {
        CategoryID id(Track, categoryNum);
        return m_rows[findIdentifier(id)].options.trackWidth();
    }

    /**
     * @param  index, the 0-based index for the folder boundary.
     * @return true if there should be a folder separator between category
     *         index and index + 1, and false otherwise.  Note that for purposes
     *         of this function, only categories that are required or non-empty
     *         should count.
     */
    virtual bool hasFolderSeparator(int index) const override;

    /**
     * @param category The category to get the status of.
     * @return true if \p category is disabled by the user, and false otherwise.
     */
    virtual bool isDisabled(const CategoryID &category) const override
    {
        return m_rows[findIdentifier(category)].options.disabled();
    }

    /**
     * This moves the widget \p l in the direction given by \p direction, taking
     * care to make sure that the checkboxes are not moved, and that they are
     * enabled or disabled as appropriate for the new layout, and that the up and
     * down buttons are also adjusted as necessary.
     *
     * @param id the identifier of the row to move
     * @param direction the direction to move
     */
    void moveItem(int id, MovementDirection direction);

    /**
     * This function actually performs the work of showing the options dialog for
     * \p category.
     *
     * @param category the category to show the options dialog for.
     */
    void showCategoryOptions(TagType category);

    /**
     * This function enables or disables the widget in the row identified by \p id,
     * controlled by \p enable.  This function also makes sure that checkboxes are
     * enabled or disabled as appropriate if they no longer make sense due to the
     * adjacent category being enabled or disabled.
     *
     * @param id the identifier of the row to change.  This is *not* the category to
     *        change.
     * @param enable enables the category if true, disables if false.
     */
    void setCategoryEnabled(int id, bool enable);

    /**
     * This function returns the identifier of the row at \p position.
     *
     * @param position The position to find the identifier of.
     * @return The unique id of the row at \p position.
     */
    int idOfPosition(int position) const;

    /**
     * This function returns the identifier of the row in the m_rows index that
     * contains \p category and matches \p categoryNum.
     *
     * @param category the category to find.
     * @return the identifier of the category, or MAX_CATEGORIES if it couldn't
     *         be found.
     */
    int findIdentifier(const CategoryID &category) const;

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
     * for \p id.
     *
     * @param id the unique id to bring up the options for.
     */
    virtual void showCategoryOption(int id);

    /**
     * This function removes the row identified by id and updates the internal data to be
     * consistent again, by forwarding the call to removeRow().
     *
     * @param id The unique id to update
     */
    virtual void slotRemoveRow(int id);

    /**
     * This function moves \p category up in the layout.
     *
     * @param id the unique id of the widget to move up.
     */
    virtual void moveItemUp(int id);

    /**
     * This function moves \p category down in the layout.
     *
     * @param id the unique id of the widget to move down.
     */
    virtual void moveItemDown(int id);

    /**
     * This slot should be called whenever the example input dialog is shown.
     */
    virtual void exampleDialogShown();

    /**
     * This slot should be called whenever the example input dialog is hidden.
     */
    virtual void exampleDialogHidden();

private:
    /// This is the frame that holds all of the category widgets and checkboxes.
    QFrame *m_mainFrame;

    Ui::FileRenamerBase *m_ui;

    /**
     * This is the meat of the widget, it holds the rows for the user configuration.  It is
     * initially created such that m_rows[0] is the top and row + 1 is the row just below.
     * However, this is NOT NECESSARILY true, so don't rely on this.  As soon as the user
     * clicks an arrow to move a row then the order will be messed up.  Use row.position to
     * determine where the row is in the GUI.
     *
     * @see idOfPosition
     * @see findIdentifier
     */
    Rows m_rows;

    /**
     * This holds an array of checkboxes that allow the user to insert folder
     * separators in between categories.
     */
    QVector<QCheckBox *> m_folderSwitches;

    ExampleOptionsDialog *m_exampleDialog;

    /// This is true if we're reading example tags from m_exampleFile.
    bool m_exampleFromFile;
    QString m_exampleFile;
};

/**
 * This class contains the backend code to actually implement the file renaming.  It performs
 * the function of moving the files from one location to another, constructing the file name
 * based off of the user's options (see ConfigCategoryReader) and of setting folder icons
 * if appropriate.
 *
 * @author Michael Pyne <mpyne@kde.org>
 */
class FileRenamer final
{
public:
    FileRenamer();

    /**
     * Renames the filename on disk of the file represented by item according
     * to the user configuration stored in KConfig.
     *
     * @param item The item to rename.
     */
    void rename(PlaylistItem *item);

    /**
     * Renames the filenames on disk of the files given in items according to
     * the user configuration stored in KConfig.
     *
     * @param items The items to rename.
     */
    void rename(const PlaylistItemList &items);

    /**
     * Returns the file name that would be generated based on the options read from
     * interface, which must implement CategoryReaderInterface.  (A whole interface is used
     * so that we can re-use the code to generate filenames from a in-memory GUI and from
     * KConfig).
     *
     * @param interface object to read options/data from.
     */
    static QString fileName(const CategoryReaderInterface &interface);

private:
    /**
     * Sets the folder icon for elements of the destination path for item (if
     * there is not already a folder icon set, and if the folder's name has
     * the album name.
     */
    void setFolderIcon(const QUrl &dst, const PlaylistItem *item);

    /**
     * Attempts to rename the file from \a src to \a dest.  Returns true if the
     * operation succeeded.
     */
    bool moveFile(const QString &src, const QString &dest);
};

#endif /* JUK_FILERENAMER_H */

// vim: set et sw=4 tw=0 sta:
