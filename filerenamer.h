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

#include <kconfigbase.h>
#include "playlistitem.h"

class Tag;

class FileRenamer
{
	public:
		enum TokenType {
			Title, Artist, Album, Track, Comment
		};

		class Config
		{
			public:
				Config(KConfigBase *cfg);

				QString filenameScheme() const;
				void setFilenameScheme(const QString &scheme);

				QString getToken(TokenType type) const;
				void setToken(TokenType type, const QString &value);

				bool tokenNeedsValue(TokenType type) const;
				void setTokenNeedsValue(TokenType type, bool needsValue);

			private:
				KConfigGroup m_grp;
		};

		static QString tokenToString(TokenType type);

		FileRenamer();
		FileRenamer(PlaylistItem *item);

		void rename(PlaylistItem *item);
		void rename(const PlaylistItemList &items);
		QString rename(const QString &filename, const Tag &tag) const;

	private:
		class ConfirmationDialog;

		QString expandToken(TokenType type, const QString &value) const;
		/**
		 * Attempts to rename the file from \a src to \a dest.  Returns true
		 * if the operation succeeded.
		 */
		bool moveFile(const QString &src, const QString &dest);

		Config m_cfg;
};

#endif // FILERENAMER_H
// vim:ts=4:sw=4:noet
