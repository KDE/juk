/***************************************************************************
                          filelistitemdata.cpp  -  description
                             -------------------
    begin                : Fri Mar 22 2002
    copyright            : (C) 2002 by Scott Wheeler
    email                : scott@slackorama.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "filelistitemdata.h"

////////////////////////////////////////////////////////////////////////////////
// public methods
////////////////////////////////////////////////////////////////////////////////

FileListItemData::FileListItemData(QFileInfo &file) : QFileInfo(file)
{
    referenceCount = 1;

    // initialize pointers to null
    cache = 0;
    tag = 0;
    audioData = 0;
}

FileListItemData::~FileListItemData()
{
    delete(cache);
    delete(tag);
    delete(audioData);
}

FileListItemData *FileListItemData::newUser()
{
    referenceCount++;
    return(this);
}

void FileListItemData::deleteUser()
{
    if(--referenceCount == 0)
        delete(this);
}

Tag *FileListItemData::getTag()
{
    if(!tag)
        tag = new Tag(filePath());
    return(tag);
}

AudioData *FileListItemData::getAudioData()
{
    if(!audioData) {
        audioData = new AudioData(filePath());
    }
    return(audioData);
}

void FileListItemData::setFile(QString file)
{
    delete(tag);
    tag = 0;

    QFileInfo::setFile(file);
}
