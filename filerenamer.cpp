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
#if KDE_IS_VERSION(3,1,90)
#  include <kmacroexpander.h>
#endif

#include <qdir.h>

FileRenamer::FileRenamer()
{
}

FileRenamer::FileRenamer(const PlaylistItem *item)
{
    rename(item);
}

void FileRenamer::rename(const PlaylistItem *item)
{
    if(item == 0 || item->tag() == 0)
        return;

    QString newFilename;
    QString titleToken, artistToken, albumToken, trackToken, commentToken;
    KConfig *cfg = kapp->config();
    {
        KConfigGroupSaver saver(cfg, "FileRenamer");
        newFilename = cfg->readEntry("FilenameScheme", item->absFilePath());

        if(!item->tag()->track().isNull()) {
            titleToken = cfg->readEntry("Title token");
            titleToken.replace("%s", item->tag()->track());
        }

        if(!item->tag()->artist().isNull()) {
            artistToken = cfg->readEntry("Artist token");
            artistToken.replace("%s", item->tag()->artist());
        }

        if(!item->tag()->album().isNull()) {
            albumToken = cfg->readEntry("Album token");
            albumToken.replace("%s", item->tag()->album());
        }

        if(item->tag()->trackNumber() > 0) {
            trackToken = cfg->readEntry("Track token");
            trackToken.replace("%s", item->tag()->trackNumberString());
        }

        if(!item->tag()->comment().isNull()) {
            commentToken = cfg->readEntry("Comment token");
            trackToken.replace("%s", item->tag()->comment());
        }
    }

    QMap<QChar, QString> substitutions;
    substitutions[ 't' ] = titleToken;
    substitutions[ 'a' ] = artistToken;
    substitutions[ 'A' ] = albumToken;
    substitutions[ 'T' ] = trackToken;
    substitutions[ 'c' ] = commentToken;

#if KDE_IS_VERSION(3,1,90)
    newFilename = KMacroExpander::expandMacros(newFilename, substitutions);
#else
    QMap<QChar, QString>::ConstIterator it = substitutions.begin();
    QMap<QChar, QString>::ConstIterator end = substitutions.end();
    for(; it != end; ++it)
        newFilename.replace("%" + QString(it.key()), it.data());
#endif

    newFilename = newFilename.stripWhiteSpace();

    if(QFileInfo(newFilename).isRelative())
        newFilename = item->absFilePath().left( item->absFilePath().findRev( "/" ) )
                      + "/" + newFilename;

    newFilename += "." + QFileInfo(item->absFilePath()).extension();

    moveFile(item->absFilePath(), newFilename);
}

void FileRenamer::moveFile(const QString &src, const QString &dest)
{
    kdDebug() << "Moving file " << src << " to " << dest << endl;

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
            kdDebug() << "Checking path " << processedComponents << endl;
            QDir dir(processedComponents);
            if (!dir.exists())
                kdDebug() << "Need to create " << processedComponents << endl;
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
