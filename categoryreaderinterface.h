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
     * width from this function.
     *
     * @param category to retrieve the value of.
     * @return textual representation of that category's value.
     */
    virtual QString categoryValue(TagType type) const = 0;

    /**
     * Returns the user-specified prefix string for \p category.
     *
     * @param category the category to retrieve the value for.
     * @return user-specified prefix string for \p category.
     */
    virtual QString prefix(TagType category) const = 0;

    /**
     * Returns the user-specified suffix string for \p category.
     *
     * @param category the category to retrieve the value for.
     * @return user-specified suffix string for \p category.
     */
    virtual QString suffix(TagType category) const = 0;

    /**
     * Returns the user-specified empty action for \p category.
     *
     * @param category the category to retrieve the value for.
     * @return user-specified empty action for \p category.
     */
    virtual TagRenamerOptions::EmptyActions emptyAction(TagType category) const = 0;

    /**
     * Returns the user-specified empty text for \p category.  This text might
     * be used to replace an empty value.
     *
     * @param category the category to retrieve the value for.
     * @return the user-specified empty text for \p category.
     */
    virtual QString emptyText(TagType category) const = 0;

    virtual QValueList<TagType> categoryOrder() const = 0;

    // You probably shouldn't reimplement this
    virtual QString value(TagType category) const;

    virtual QString separator() const = 0;

    virtual QString musicFolder() const = 0;

    virtual int trackWidth() const = 0;

    virtual bool hasFolderSeparator(int index) const = 0;

    virtual bool isDisabled(TagType category) const = 0;

    // You probably shouldn't reimplement this
    virtual bool isRequired(TagType category) const;

    // You probably shouldn't reimplement this
    virtual bool isEmpty(TagType category) const;
};

#endif /* JUK_CATEGORYREADERINTERFACE_H */

// vim: set et sw=4 ts=4:
