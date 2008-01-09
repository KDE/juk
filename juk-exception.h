/***************************************************************************
    begin                : Tue Jan 08 2008
    copyright            : (C) 2008 by Michael Pyne
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

#ifndef JUK_EXCEPTION_H
#define JUK_EXCEPTION_H

#include <exception>

/**
 * This exception is thrown when playlist reading code believes that it has
 * encountered a binary incompatible version of QDataStream.  Added due to a
 * coding error which resulted in not setting a specific encoding for
 * QDataStreams.
 */
class BICStreamException : public std::exception
{
    virtual const char *what() const throw()
    {
        return "Read jibberish from a QDataStream, probably using an older protocol.";
    }
};

#endif /* JUK_EXCEPTION_H */

// vim: set et sw=4 tw=0 sta:
