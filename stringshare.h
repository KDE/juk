/***************************************************************************
    begin                : Sat Oct 25 2003
    copyright            : (C) 2003 by Maksim Orlovich
    email                : maksim.orlovich@kdemail.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef STRING_SHARE_H
#define STRING_SHARE_H

#include <qstring.h>

/**
 This class attempts to normalize repeated occurances of strings to use
 the same shared object, if possible, by using a small hash
*/
class StringShare
{
    struct Data;
public:
    static QString  tryShare(const QString& in);
    static QCString tryShare(const QCString& in);

private:
    static Data* data();
    static Data* s_data;
};

#endif
