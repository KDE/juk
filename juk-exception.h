/**
 * Copyright (C) 2008 Michael Pyne <mpyne@kde.org>
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
