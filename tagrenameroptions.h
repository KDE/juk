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

#include <qstring.h>

// Insert all new tag types before NumTypes, that way NumTypes will always be
// the count of valid tag types.
enum TagType {
    StartTag, Title = StartTag, Artist, Album,
    Track, Genre, Year, NumTypes, Unknown
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
    TagRenamerOptions(TagType category);
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

    TagRenamerOptions &operator=(const TagRenamerOptions &other);

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
     * This saves the options to the global KConfig object.
     */
    void saveConfig() const;

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
