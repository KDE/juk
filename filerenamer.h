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

class PlaylistItem;
class QString;

class FileRenamer
{
	public:
		FileRenamer();
		FileRenamer(const PlaylistItem *item);

		void rename(const PlaylistItem *item);

	private:
		void moveFile(const QString &src, const QString &dest);
};

#endif // FILERENAMER_H
// vim:ts=4:sw=4:noet
