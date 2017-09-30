/**
 * Copyright (C) 2004 Nathan Toone <nathan@toonetown.com>
 * Copyright (C) 2007 Michael Pyne <mpyne@kde.org>
 * Copyright (C) 2012 Martin Sandsmark <martin.sandsmark@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef JUK_WEBIMAGEFETCHER_H
#define JUK_WEBIMAGEFETCHER_H

#include <QObject>

class KJob;
class FileHandle;

class WebImageFetcher : public QObject
{
    Q_OBJECT

public:
    WebImageFetcher(QObject *parent);
    ~WebImageFetcher();

    void setFile(const FileHandle &file);

public slots:
    void abortSearch();
    void searchCover();

signals:
    void signalCoverChanged(int coverId);

private slots:
    void slotWebRequestFinished(KJob *job);
    void slotImageFetched(KJob *job);
    void slotCoverChosen();

private:
    class Private;
    Private *d;
};

#endif

// vim: set et sw=4 tw=0 sta:
