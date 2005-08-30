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

#ifndef JUK_CATEGORYREADERINTERFACE_H
#define JUK_CATEGORYREADERINTERFACE_H

#include "tagrenameroptions.h"

enum TagType;
class QString;

template<class T> class QValueList;

/**
 * This class is used to map categories into values.  You should implement the
 * functionality in a subclass.
 *
 * @author Michael Pyne <michael.pyne@kdemail.net>
 */
class CategoryReaderInterface
{
public:
    virtual ~CategoryReaderInterface() { }

    /**
     * Returns the textual representation of \p type, without any processing done
     * on it.  For example, track values shouldn't be expanded out to the minimum
     * width from this function.  No CategoryID is needed since the value is constant
     * for a category.
     *
     * @param type, The category to retrieve the value of.
     * @return textual representation of that category's value.
     */
    virtual QString categoryValue(TagType type) const = 0;

    /**
     * Returns the user-specified prefix string for \p category.
     *
     * @param category the category to retrieve the value for.
     * @return user-specified prefix string for \p category.
     */
    virtual QString prefix(const CategoryID &category) const = 0;

    /**
     * Returns the user-specified suffix string for \p category.
     *
     * @param category the category to retrieve the value for.
     * @return user-specified suffix string for \p category.
     */
    virtual QString suffix(const CategoryID &category) const = 0;

    /**
     * Returns the user-specified empty action for \p category.
     *
     * @param category the category to retrieve the value for.
     * @return user-specified empty action for \p category.
     */
    virtual TagRenamerOptions::EmptyActions emptyAction(const CategoryID &category) const = 0;

    /**
     * Returns the user-specified empty text for \p category.  This text might
     * be used to replace an empty value.
     *
     * @param category the category to retrieve the value for.
     * @return the user-specified empty text for \p category.
     */
    virtual QString emptyText(const CategoryID &category) const = 0;

    /**
     * @return the categories in the order the user has chosen.  Categories may
     * be repeated (which is why CategoryID has the categoryNumber value to
     * disambiguate duplicates).
     */
    virtual QValueList<CategoryID> categoryOrder() const = 0;

    /**
     * @return track width for the Track item identified by categoryNum.
     */
    virtual int trackWidth(unsigned categoryNum) const = 0;

    // You probably shouldn't reimplement this
    virtual QString value(const CategoryID &category) const;

    virtual QString separator() const = 0;

    virtual QString musicFolder() const = 0;

    /**
     * @param index the zero-based index of the item just before the
     *        separator in question.
     * @return true if a folder separator should be placed between the tags
     * at index and index + 1.
     */
    virtual bool hasFolderSeparator(unsigned index) const = 0;

    virtual bool isDisabled(const CategoryID &category) const = 0;

    // You probably shouldn't reimplement this
    virtual bool isRequired(const CategoryID &category) const;

    // You probably shouldn't reimplement this
    virtual bool isEmpty(TagType category) const;

    // You probably shouldn't reimplement this
    virtual QString fixupTrack(const QString &track, unsigned categoryNum) const;
};

#endif /* JUK_CATEGORYREADERINTERFACE_H */

// vim: set et sw=4 ts=4:
