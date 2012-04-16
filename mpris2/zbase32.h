/***************************************************************************
    begin                : Sun Apr 16 14:23:12 EST 2012
    copyright            : (C) 2012 by Michael Pyne
    email                : mpyne@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef JUK_ZBASE32_H
#define JUK_ZBASE32_H

class QByteArray;

/**
 * Decodes a string encoded in the z-base-32 standard from
 * http://philzimmermann.com/docs/human-oriented-base-32-encoding.txt
 *
 * The implementation is "home-grown" due to lack of available simple
 * implementations.
 *
 * The encoding format itself doesn't know anything about the length, so it
 * decodes as far as it can, fills any remaining bits at the end with 0 to make
 * a full byte, and returns the resultant data.
 *
 * Note that @p str is assumed to be lowercase. If you allow user input for
 * this parameter you must ensure it is lowercased (e.g. with
 * QByteArray::toLower())
 *
 * @param str is the encoded string to decode. It is a QByteArray since by
 * definition the encoded string should easily conform to US-ASCII and/or
 * Latin1 standard and so the overhead of QString is unneeded.
 */
QByteArray zBase32DecodeData(const QByteArray &str);

/**
 * Encodes a bytestream in a string in the z-base-32 encoding from
 * http://philzimmermann.com/docs/human-oriented-base-32-encoding.txt
 *
 * The implementation is "home-grown" due to lack of available simple
 * implementations.
 *
 * The encoding format itself doesn't know anything about the length, so it
 * encodes all of the bytes, and fills any remaining bits at the end with 0 to
 * make a full byte, and returns the resultant string without padding.
 *
 * The returned string will be entirely in lowercase (and some digits).
 *
 * @param data, the byte stream to encode.
 */
QByteArray zBase32EncodeData(const QByteArray &data);

#endif
