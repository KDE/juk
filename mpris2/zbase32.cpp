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


#include <QtDebug>
#include <QtCore/QLatin1String>
#include <QtCore/QString>
#include <QtCore/QByteArray>
#include <QtCore/QFile>

#include <kdemacros.h>

// The 0-based position of an entry in this table indicates its value.
static const char zBase32EncodingTable [] = "ybndrfg8ejkmcpqxot1uwisza345h769";

// 8 5-bit entries for 40-bits total
static const int ZBASE32_PARTIAL_BUFFER_SIZE = 8;

static int locateCharacterInTable(const QChar ch)
{
    for (int i = 0; i < 32; ++i) {
        if (zBase32EncodingTable[i] == ch) {
            return i;
        }
    }

    return -1;
}

// @p len is the number of **destination** items to utilize!
static void zBase32DecodeBufferIntoBuffer(
        unsigned char *destination,
        const unsigned char *source,
        int destinationLength)
{
    // Each item in source buffer is already masked to only take 5 bits.
    // We need to pick the appropriate bits of each item and place in
    // the right spot.

    // 00000000-11111111-22222222-33333333-44444444 destination
    // 01234567-01234567-01234567-01234567-01234567
    // 00000111-11222223-33334444-45555566-66677777 source

    switch (destinationLength) {
        case 5: destination[4] = ((source[6] & 0x07) << 5) | source[7];
                // fallthrough
        case 4: destination[3] = ((source[4] & 0x01) << 7) | (source[5] << 2) | ((source[6]) >> 3);
                // fallthrough
        case 3: destination[2] = ((source[3] & 0x0F) << 4) | (source[4] >> 1);
                // fallthrough
        case 2: destination[1] = ((source[1] & 0x03) << 6) | (source[2] << 1) | (source[3] >> 4);
                // fallthrough
        case 1: destination[0] = (source[0] << 3) | (source[1] >> 2);
            break;
        default:
            return;
    }
}

// Returns number of decoded bytes that are left from the given remainder
// length of source characters. This means the remainder length should be
// between 1 and 7 inclusive.
static int zBase32BytesAffectedFromSource(int sourceRemainderLength)
{
    switch (sourceRemainderLength) {
        case 7: return 5;
        case 6: return 4;
        case 5: return 4;
        case 4: return 3;
        case 3: return 2;
        case 2: return 2;
        case 1: return 1;
        case 0: return 0;
        default: return -1;
    }
}

QByteArray zBase32DecodeData(const QByteArray &str)
{
    QByteArray result;
    unsigned char buffer[ZBASE32_PARTIAL_BUFFER_SIZE] = { 0 };
    int strSize = str.size();
    int runLength = (strSize / ZBASE32_PARTIAL_BUFFER_SIZE) * ZBASE32_PARTIAL_BUFFER_SIZE;
    int remainder = strSize - runLength;

    result.resize(strSize / ZBASE32_PARTIAL_BUFFER_SIZE * 5 + zBase32BytesAffectedFromSource(remainder));
    int destinationPointer = 0;

    for (int i = 0; i < runLength; ++i) {
        int pos = locateCharacterInTable(str[i]);
        if (KDE_ISUNLIKELY(pos < 0)) {
            return QByteArray();
        }

        if (zBase32EncodingTable[pos] != str[i]) {
            return QByteArray();
        }

        buffer[i % ZBASE32_PARTIAL_BUFFER_SIZE] = pos;

        if (((i + 1) % ZBASE32_PARTIAL_BUFFER_SIZE) == 0) {
            // End of row, compress
            unsigned char *encodedData = reinterpret_cast<unsigned char *>(result.data()) + destinationPointer;

            zBase32DecodeBufferIntoBuffer(encodedData, buffer, 5);
            destinationPointer += 5;

            for (int j = 0; j < ZBASE32_PARTIAL_BUFFER_SIZE; j++) {
                buffer[j] = 0;
            }
        }
    }

    // Complete weird remainder. buffer should have already been cleared
    // above.
    for (int i = 0; i < remainder; ++i) {
        int pos = locateCharacterInTable(str[i + runLength]);
        if (KDE_ISUNLIKELY(pos < 0)) {
            return QByteArray();
        }

        buffer[i] = pos;
    }

    zBase32DecodeBufferIntoBuffer(
            reinterpret_cast<unsigned char *>(result.data()) + destinationPointer,
            buffer,
            zBase32BytesAffectedFromSource(remainder));

    return result;
}

// Returns the number of destination entries used to encode the given bytes
// of the source.
static int zBase32EncodeBufferIntoBuffer(
        unsigned char *destination,
        const unsigned char *source,
        int sourceLength)
{
    // Each item in destination buffer must be masked to only take 5 bits.
    // We need to pick the appropriate bits of each item and place in
    // the right spot.

    // 00000000-11111111-22222222-33333333-44444444 source
    // 01234567-01234567-01234567-01234567-01234567
    // 00000111-11222223-33334444-45555566-66677777 destination

    for (int i = 0; i < ZBASE32_PARTIAL_BUFFER_SIZE; ++i) {
        destination[i] = 0;
    }

    int entriesUsed = 0;

    switch (sourceLength) {
        case 5:
            entriesUsed = 8;
            destination[7] = source[4] & 0x1f;
                // fallthrough
        case 4:
            entriesUsed = entriesUsed ? entriesUsed : 7;
            destination[6] = ((source[4] & 0xe0) >> 5) | ((source[3] & 0x03) << 3);
            destination[5] = ((source[3] & 0x7c) >> 2);
                // fallthrough
        case 3:
            entriesUsed = entriesUsed ? entriesUsed : 5;
            destination[4] = ((source[2] & 0x0f) << 1) | ((source[3] & 0x80) >> 7);
                // fallthrough
        case 2:
            entriesUsed = entriesUsed ? entriesUsed : 4;
            destination[3] = ((source[1] & 0x01) << 4) | ((source[2] & 0xf0) >> 4);
            destination[2] = ((source[1] & 0x3e) >> 1);
                // fallthrough
        case 1:
            entriesUsed = entriesUsed ? entriesUsed : 2;
            destination[1] = ((source[0] & 0x07) << 2) | ((source[1] & 0xc0) >> 6);
            destination[0] = ((source[0] & 0xf8) >> 3);
            break;
        default:
            return -1;
    }

    // We still have binary values, encode them to legible items from the table
    for (int i = 0; i < entriesUsed; ++i) {
        destination[i] = zBase32EncodingTable[destination[i]];
    }

    return entriesUsed;
}

QByteArray zBase32EncodeData(const QByteArray &data)
{
    QByteArray result;
    result.reserve((data.size() * 8 / 5) + 1);

    int runLength = data.size() / 5 * 5;
    int remainder = data.size() - runLength;

    unsigned char buffer[ZBASE32_PARTIAL_BUFFER_SIZE] = { 0 };

    for (int i = 0; i < runLength; i += 5) {
        zBase32EncodeBufferIntoBuffer(buffer, reinterpret_cast<unsigned const char *>(data.constData()) + i, 5);
        result.append(reinterpret_cast<const char *>(buffer), 8);
    }

    int entriesUsed = zBase32EncodeBufferIntoBuffer(
        buffer,
        reinterpret_cast<const unsigned char *>(data.constData()) + runLength,
        remainder);

    result.append(reinterpret_cast<const char *>(buffer), entriesUsed);
    result.resize((runLength / 5 * 8) + entriesUsed);

    return result;
}
