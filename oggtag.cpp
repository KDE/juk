/***************************************************************************
                          oggtag.cpp  -  description
                             -------------------
    begin                : Sat Oct 5 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : wheeler@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kdebug.h>

#include <qdatetime.h>
#include <qregexp.h>
#include <qvariant.h>

#include "oggtag.h"
#include "genrelistlist.h"

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

OggTag::OggTag(const QString &file) : Tag(file)
{
    m_fileInfo.setFile(file);
    m_metaInfo = KFileMetaInfo(file);
    m_commentGroup = KFileMetaInfoGroup(m_metaInfo.group("Comment"));
}

OggTag::~OggTag()
{

}

void OggTag::save()
{
    m_metaInfo.applyChanges();
}

bool OggTag::hasTag() const
{
    if(m_metaInfo.isValid() && !m_metaInfo.isEmpty())
	return true;
    else
	return false;
}

QString OggTag::track() const
{
    QString s = readCommentString("Title");

    if(s.stripWhiteSpace().isEmpty())
	s = m_fileInfo.baseName();

    return s;
}

QString OggTag::artist() const
{
    return readCommentString("Artist");
}

QString OggTag::album() const
{
    return readCommentString("Album");
}

Genre OggTag::genre() const
{
    QString genreName = readCommentString("Genre");
    int index = GenreListList::ID3v1List().findIndex(genreName);
    return Genre(genreName, index);
}

int OggTag::trackNumber() const
{
    return readCommentInt("Tracknumber");
}

QString OggTag::trackNumberString() const
{
    QString s = readCommentString("Tracknumber");
    return s.replace(QRegExp("^0+"), QString::null);
}

int OggTag::year() const
{
    QDateTime d = QDateTime::fromString(readCommentString("Date"), Qt::ISODate);

    if(d.isValid())
	return d.date().year();
    else
	return 0;
}

QString OggTag::yearString() const
{
    QDateTime d = QDate::fromString(readCommentString("Date"), Qt::ISODate);
    if(d.isValid())
	return QString::number(d.date().year());
    else
	return QString::null;
}

QString OggTag::comment() const
{
    return readCommentString("Description");
}

void OggTag::setTrack(const QString &value)
{
    writeCommentItem("Title", value);
}

void OggTag::setArtist(const QString &value)
{
    writeCommentItem("Artist", value);
}

void OggTag::setAlbum(const QString &value)
{
    writeCommentItem("Album", value);
}

void OggTag::setGenre(const Genre &value)
{
    writeCommentItem("Genre", value);
}

void OggTag::setTrackNumber(int value)
{
    writeCommentItem("Tracknumber", value);
}

void OggTag::setYear(int value)
{
    QDate d = QDate::fromString(readCommentString("Date"), Qt::ISODate);
    if(d.setYMD(value, d.month(), d.day())) {
	QDateTime dt = d;
	writeCommentItem("Date", dt.toString(Qt::ISODate));
    }
}

void OggTag::setComment(const QString &value)
{
    writeCommentItem("Description", value);
}

QString OggTag::bitrateString() const
{
    return readBitrate(m_metaInfo);
}

QString OggTag::lengthString() const
{
    return readLength(m_metaInfo);
}

int OggTag::seconds() const
{
    return readSeconds(m_metaInfo);
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

QString OggTag::readCommentString(const QString &key) const
{
    if(m_metaInfo.isValid() && !m_metaInfo.isEmpty() &&
       m_commentGroup.isValid() && !m_commentGroup.isEmpty() &&
       m_commentGroup.contains(key))
	// I'm throwing in the stripWhiteSpace() here, because the IOSlave/KFMI
	// stuff seems to be padding fields arbitrarily. 
	return m_commentGroup.item(key).string().stripWhiteSpace();
    else
	return QString::null;
}

int OggTag::readCommentInt(const QString &key) const
{
    if(m_metaInfo.isValid() && !m_metaInfo.isEmpty() &&
       m_commentGroup.isValid() && !m_commentGroup.isEmpty() &&
       m_commentGroup.contains(key)) {
	bool ok;
	int value = m_commentGroup.item(key).value().toInt(&ok);
	if(ok)
	    return value;
	else
	    return -1;
    }
    else
	return -1;
}

void OggTag::writeCommentItem(const QString &key, const QString &value)
{
    if(m_metaInfo.isValid() && m_commentGroup.isValid()) {
	QVariant v(value);
	if(!m_commentGroup.contains(key))
	    m_commentGroup.addItem(key);
	m_commentGroup.item(key).setValue(v);
    }
}

void OggTag::writeCommentItem(const QString &key, int value)
{
    if(m_metaInfo.isValid() && m_commentGroup.isValid()) {
	QVariant v(QString::number(value));
	if(!m_commentGroup.contains(key))
	    m_commentGroup.addItem(key);
	m_commentGroup.item(key).setValue(v);
    }
}
