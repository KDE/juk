/*
 * filerenamer.h - (c) 2003 Frerich Raabe <raabe@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef FILERENAMER_H
#define FILERENAMER_H

#include <qstring.h>

class PlaylistItem;
class Tag;

class FileRenamer
{
	public:
		FileRenamer();
		FileRenamer(const PlaylistItem *item);

		QString filenameScheme() const;
		QString titleToken( const QString &value = QString::null ) const;
		QString artistToken( const QString &value = QString::null ) const;
		QString albumToken( const QString &value = QString::null ) const;
		QString trackToken( const QString &value = QString::null) const;
		QString commentToken( const QString &value = QString::null ) const;

		void rename(const PlaylistItem *item);
		QString rename(const QString &filename, const Tag &tag) const;

	private:
		QString getToken(const QString &name, const QString &value) const;
		void moveFile(const QString &src, const QString &dest);
};

#endif // FILERENAMER_H
// vim:ts=4:sw=4:noet
