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

class PlaylistItem;
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
		FileRenamer(const PlaylistItem *item);

		void rename(const PlaylistItem *item);
		QString rename(const QString &filename, const Tag &tag) const;

	private:
		QString expandToken(TokenType type, const QString &value) const;
		void moveFile(const QString &src, const QString &dest);

		Config m_cfg;
};

#endif // FILERENAMER_H
// vim:ts=4:sw=4:noet
