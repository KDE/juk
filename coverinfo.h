/***************************************************************************
    copyright            : (C) 2004 Nathan Toone
    email                : nathan@toonetown.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef COVERINFO_H
#define COVERINFO_H

#include <qimage.h>
#include <qpixmap.h>
#include <qlabel.h>

#include "filehandle.h"

class CoverPopup : public QLabel
{
public:
    CoverPopup(QPixmap &pixmap):QLabel(0),m_popupWidget(0),m_pixmap(pixmap){}
     
    virtual bool eventFilter(QObject *object, QEvent *event);
    void popup(int x, int y);  

private:
    QWidget *m_popupWidget;
    QPixmap m_pixmap;
};

class CoverInfo
{
    friend class FileHandle;

public:
    enum CoverSize { FullSize, Thumbnail };

    CoverInfo(const FileHandle &file);

    bool hasCover();

    void clearCover();
    void setCover(const QImage &image = QImage());

    QPixmap pixmap(CoverSize size) const;
    void popupCover(int x = 10, int y = 10);

private:
    QString coverLocation(CoverSize size) const;

    FileHandle m_file;
    bool m_hasCover;
    bool m_haveCheckedForCover;
    CoverPopup *m_popup;
};
#endif

