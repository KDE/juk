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

#include <qstring.h>

#include "filerenameroptions.h"
#include "categoryreaderinterface.h"

QString CategoryReaderInterface::value(TagType category) const
{
    QString value = categoryValue(category).stripWhiteSpace();

    if(value.isEmpty() && emptyAction(category) == TagRenamerOptions::UseReplacementValue)
        value = emptyText(category);

    return prefix(category) + value + suffix(category);
}

bool CategoryReaderInterface::isRequired(TagType category) const
{
    return emptyAction(category) != TagRenamerOptions::IgnoreEmptyTag;
}

bool CategoryReaderInterface::isEmpty(TagType category) const
{
    return categoryValue(category).isEmpty();
}

// vim: set et sw=4 ts=4:
