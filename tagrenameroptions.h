/***************************************************************************
    begin                : Sun Oct 31 2004
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

#ifndef JUK_TAGRENAMEROPTIONS_H
#define JUK_TAGRENAMEROPTIONS_H

// Insert all new tag types before NumTypes, that way NumTypes will always be
// the count of valid tag types.
enum TagType {
    StartTag, Title = StartTag, Artist, Album,
    Track, Genre, Year, NumTypes, Unknown
};

/**
 * Class that uniquely identifies a user's category (since the user may have
 * the same category more than once in their file renaming structure).
 */
struct CategoryID
{
    CategoryID() : category(Unknown), categoryNumber(0)
    {
    }

    CategoryID(const CategoryID &other) : category(other.category),
                                          categoryNumber(other.categoryNumber)
    {
    }

    CategoryID(TagType cat, unsigned num) : category(cat), categoryNumber(num)
    {
    }

    CategoryID &operator=(const CategoryID &other)
    {
        if(this == &other)
            return *this;

        category = other.category;
        categoryNumber = other.categoryNumber;

        return *this;
    }

    bool operator==(const CategoryID &other) const
    {
        return category == other.category && categoryNumber == other.categoryNumber;
    }

    bool operator!=(const CategoryID &other) const
    {
        return !(*this == other);
    }

    bool operator<(const CategoryID &other) const
    {
        if(category == other.category)
            return categoryNumber < other.categoryNumber;

        return category < other.category;
    }

    TagType category;
    unsigned categoryNumber;
};

/**
 * Defines options for a tag type.  Used by FileRenamerTagOptions as its
 * data type.
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 */
class TagRenamerOptions
{
public:
    enum EmptyActions { ForceEmptyInclude, IgnoreEmptyTag, UseReplacementValue };

    TagRenamerOptions();

    /**
     * Construct the options by loading from KConfig.
     *
     * @param category The category to load the options for.
     */
    TagRenamerOptions(const CategoryID &category);
    TagRenamerOptions(const TagRenamerOptions &other);

    QString prefix() const { return m_prefix; }
    QString suffix() const { return m_suffix; }
    QString emptyText() const { return m_emptyText; }
    EmptyActions emptyAction() const { return m_emptyAction; }
    unsigned trackWidth() const { return m_trackWidth; }
    bool disabled() const { return m_disabled; }
    TagType category() const { return m_category; }

    void setPrefix(const QString &prefix) { m_prefix = prefix; }
    void setSuffix(const QString &suffix) { m_suffix = suffix; }
    void setEmptyText(const QString &emptyText) { m_emptyText = emptyText; }
    void setEmptyAction(EmptyActions action) { m_emptyAction = action; }
    void setTrackWidth(unsigned width) { m_trackWidth = width; }
    void setDisabled(bool disabled) { m_disabled = disabled; }
    void setCategory(TagType category) { m_category = category; }

    /**
     * Maps \p type to a textual representation of its name.  E.g. Track => "Track"
     *
     * @param type the category to retrieve a text representation of.
     * @param translate if true, the string is translated (if possible).
     * @return text representation of category.
     */
    static QString tagTypeText(TagType category, bool translate = true);

    QString tagTypeText(bool translate = true) const
    {
        return tagTypeText(category(), translate);
    }

    /**
     * Function that tries to match a string back to its category.  Uses
     * the case-sensitive form of the string.  If it fails it will return
     * Unknown.
     *
     * @param translated If true, @p text is translated, if false, it is the untranslated
     *                   version.
     */
    static TagType tagFromCategoryText(const QString &text, bool translate = true);

    /**
     * This saves the options to the global KConfig object.
     *
     * @param categoryNum The zero-based count of the number of this type of
     *           category.  For example, this would be 1 for the
     *           second category of this type.  The stored category
     *           number is not used in order to allow you to save with
     *           a different one (for compaction purposes perhaps).
     */
    void saveConfig(unsigned categoryNum) const;

private:

    // Member variables

    QString m_prefix;
    QString m_suffix;

    /// Defines the action to take when the tag is empty.
    EmptyActions m_emptyAction;

    /// If m_emptyAction is UseReplacementValue, this holds the text of the value
    /// to use.
    QString m_emptyText;

    /// Used only for the Track type.  Defines the minimum track width when
    /// expanding the track token.
    unsigned m_trackWidth;

    /// This is true if this tag is always disabled when expanding file names.
    bool m_disabled;

    TagType m_category;
};

#endif /* JUK_TAGRENAMEROPTIONS_H */

// vim: set et ts=4 sw=4:
