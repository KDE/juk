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

FileRenamer::FileRenamer()
{
}

FileRenamer::FileRenamer(const PlaylistItem *item)
{
    rename(item);
}

QString FileRenamer::filenameScheme() const
{
    return KConfigGroup(kapp->config(), "FileRenamer").readEntry("FilenameScheme");
}

QString FileRenamer::getToken(const QString &name, const QString &value) const
{
    const KConfigGroup grp(kapp->config(), "FileRenamer");

    const bool needContent = grp.readBoolEntry("Need" + name + "Content", true);
    if(value.isEmpty() && needContent)
        return QString();

    QString token = grp.readEntry(name + "Token");
    token.replace("%s", value);
    return token;
}

QString FileRenamer::titleToken(const QString &value) const
{
    return getToken("Title", value);
}

QString FileRenamer::artistToken(const QString &value) const
{
    return getToken("Artist", value);
}

QString FileRenamer::albumToken(const QString &value) const
{
    return getToken("Album", value);
}

QString FileRenamer::trackToken(const QString &value) const
{
    return getToken("Track", value);
}

QString FileRenamer::commentToken(const QString &value) const
{
    return getToken("Comment", value);
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
    QString newFilename = filenameScheme();

    QMap<QChar, QString> substitutions;
    substitutions[ 't' ] = titleToken(tag.track());
    substitutions[ 'a' ] = artistToken(tag.artist());
    substitutions[ 'A' ] = albumToken(tag.album());
    substitutions[ 'T' ] = trackToken(tag.trackNumberString());
    substitutions[ 'c' ] = commentToken(tag.comment());

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
