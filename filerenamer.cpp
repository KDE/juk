/*
 * filerenamer.cpp - (c) 2003 Frerich Raabe <raabe@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "filerenamer.h"
#include "playlistitem.h"

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kmacroexpander.h>

#include <qdir.h>

FileRenamer::Config::Config(KConfigBase *cfg)
    : m_grp(cfg, "FileRenamer")
{
}

QString FileRenamer::Config::filenameScheme() const
{
    return m_grp.readEntry("FilenameScheme");
}

void FileRenamer::Config::setFilenameScheme(const QString &scheme)
{
    m_grp.writeEntry("FilenameScheme", scheme);
}

QString FileRenamer::Config::getToken(TokenType type) const
{
    return m_grp.readEntry(tokenToString(type) + "Token");
}

void FileRenamer::Config::setToken(TokenType type, const QString &value)
{
    m_grp.writeEntry(tokenToString(type) + "Token", value);
}

bool FileRenamer::Config::tokenNeedsValue(TokenType type) const
{
    return m_grp.readBoolEntry("Need" + tokenToString(type) + "Value");
}

void FileRenamer::Config::setTokenNeedsValue(TokenType type, bool needsValue)
{
    m_grp.writeEntry("Need" + tokenToString(type) + "Value", needsValue);
}

QString FileRenamer::tokenToString(TokenType type)
{
    switch(type) {
        case Title: return "Title";
        case Artist: return "Artist";
        case Album: return "Album";
        case Track: return "Track";
        case Comment: return "Comment";
    }
    return QString::null;
}

FileRenamer::FileRenamer()
    : m_cfg(kapp->config())
{
}

FileRenamer::FileRenamer(const PlaylistItem *item)
    : m_cfg(kapp->config())
{
    rename(item);
}

QString FileRenamer::expandToken(TokenType type, const QString &value) const
{
    const bool needValue = m_cfg.tokenNeedsValue(type);
    if(needValue && value.isEmpty())
        return QString();

    QString token = m_cfg.getToken(type);
    token.replace("%s", value);
    return token;
}


void FileRenamer::rename(const PlaylistItem *item)
{
    if(item == 0 || item->tag() == 0)
        return;

    QString newFilename = rename(item->absFilePath(), *item->tag());
    moveFile(item->absFilePath(), newFilename);
}

QString FileRenamer::rename(const QString &filename, const Tag &tag) const
{
    QString newFilename = m_cfg.filenameScheme();

    QMap<QChar, QString> substitutions;
    substitutions[ 't' ] = expandToken(Title, tag.track());
    substitutions[ 'a' ] = expandToken(Artist, tag.artist());
    substitutions[ 'A' ] = expandToken(Album, tag.album());
    substitutions[ 'T' ] = expandToken(Track, tag.trackNumberString());
    substitutions[ 'c' ] = expandToken(Comment, tag.comment());

    newFilename = KMacroExpander::expandMacros(newFilename, substitutions);
    newFilename = newFilename.stripWhiteSpace();

    if(QFileInfo(newFilename).isRelative())
        newFilename = filename.left( filename.findRev( "/" ) )
            + "/" + newFilename;
    newFilename += "." + QFileInfo(filename).extension();

    return newFilename;
}

void FileRenamer::moveFile(const QString &src, const QString &dest)
{
    kdDebug(65432) << "Moving file " << src << " to " << dest << endl;

    if(src == dest)
        return;

    QString dest_ = dest.mid(1); // strip the leading "/"
    if(dest_.find("/") > 0) {
        const QStringList components = QStringList::split("/", dest_.left( dest.findRev("/")));
        QStringList::ConstIterator it = components.begin();
        QStringList::ConstIterator end = components.end();
        QString processedComponents;
        for(; it != end; ++it) {
            processedComponents += "/" + *it;
            kdDebug(65432) << "Checking path " << processedComponents << endl;
            QDir dir(processedComponents);
            if (!dir.exists())
                kdDebug(65432) << "Need to create " << processedComponents << endl;
        }
    }

    return;

    QFile srcFile(src);
    if(!srcFile.open(IO_ReadOnly)) {
        kdWarning() << "Could not open" << src << " for reading." << endl;
        return;
    }

    QFile destFile(dest);
    if(!destFile.open(IO_WriteOnly)) {
        kdWarning() << "Could not open " << dest << " for writing." << endl;
        return;
    }

    destFile.writeBlock(srcFile.readAll());

    if(!srcFile.remove())
        kdWarning() << "Could not delete source file " << src << endl;
}
// vim:ts=4:sw=4:noet
