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

#include <kdebug.h>
#include <kiconloader.h>
#include <kactionclasses.h>
#include <kconfig.h>
#include <klocale.h>
#include <kfiledialog.h>

#include <qsize.h>
#include <qsizepolicy.h>
#include <qpixmap.h>
#include <qbutton.h>
#include <qimage.h>
#include <qvaluelist.h>
#include <qsplitter.h>

#include "nowplaying.h"
#include "playermanager.h"
#include "filehandle.h"
#include "actioncollection.h"
#include "coverinfo.h"

using namespace ActionCollection;

////////////////////////////////////////////////////////////////////////////////
// public members
////////////////////////////////////////////////////////////////////////////////

NowPlaying::NowPlaying(QWidget *parent, const char *name) :
    QWidget(parent, name),
    m_button(0)
{
    setupActions();
    setupLayout();
    readConfig();

    slotClear();
}

NowPlaying::~NowPlaying()
{
    saveConfig();
}

////////////////////////////////////////////////////////////////////////////////
// public slots
////////////////////////////////////////////////////////////////////////////////

void NowPlaying::slotRefresh()
{
    if(!PlayerManager::instance()->playing())
        return;

    m_button->hide();

    FileHandle playingFile = PlayerManager::instance()->playingFile();

    if(playingFile.coverInfo()->hasCover()){
        QImage image = playingFile.coverInfo()->largeCoverPixmap().convertToImage();
        QPixmap cover = image.smoothScale(size().width(), size().width());
        setPaletteBackgroundPixmap(cover);
    }
    else {
        m_button->show();
        m_button->setText(i18n("Add Cover Image"));
        setPaletteBackgroundPixmap(QPixmap());
    }
}

void NowPlaying::slotClear()
{
    m_button->show();
    m_button->setPixmap(DesktopIcon("juk", 72));
    setPaletteBackgroundPixmap(QPixmap());
}

void NowPlaying::mousePressEvent(QMouseEvent *)
{
    if(PlayerManager::instance()->playing())
        slotButtonPress();
}

void NowPlaying::resizeEvent(QResizeEvent *ev)
{
    // if the width hasn't changed, just return.

    if(!ev || ev->oldSize().width() == size().width())
        return;

    QSplitter *parentSplitter = static_cast<QSplitter *>(parent());

    setMaximumHeight(parentSplitter->size().height());
    setMinimumHeight(0);

    resize(QSize(width(), width()));
    int newTop = parentSplitter->size().height() - size().height() - parentSplitter->handleWidth();
    int newBottom = size().height();
    QValueList<int> sizeList;
    sizeList.append(newTop);
    sizeList.append(newBottom);
    parentSplitter->setSizes(sizeList);

    FileHandle playingFile=PlayerManager::instance()->playingFile();

    if((PlayerManager::instance()->playing() || PlayerManager::instance()->paused()) &&
       playingFile.coverInfo()->hasCover())
    {
        QImage image = playingFile.coverInfo()->largeCoverPixmap().convertToImage();
        setPaletteBackgroundPixmap(image.smoothScale(size().width(), size().width()));
    }
    setMaximumHeight(width());
    setMinimumHeight(width());
}

///////////////////////////////////////////////////////////////////////////////
// private slots
////////////////////////////////////////////////////////////////////////////////

void NowPlaying::slotButtonPress()
{
    if(!PlayerManager::instance()->playing()) {
        action<KAction>("help_about_app")->activate();
        return;
    }

    FileHandle playingFile = PlayerManager::instance()->playingFile();

    if(playingFile.coverInfo()->hasCover())
        playingFile.coverInfo()->popupLargeCover();
    else {
        KURL file = KFileDialog::getImageOpenURL(":homedir", this, i18n("Select cover image file - JuK"));
        QImage image(file.directory() + "/" + file.fileName());
        image.save(playingFile.coverInfo()->coverLocation(true), "PNG");
        slotRefresh();
    }
}

////////////////////////////////////////////////////////////////////////////////
// private members
////////////////////////////////////////////////////////////////////////////////

void NowPlaying::readConfig()
{
    KConfigGroup config(KGlobal::config(), "NowPlaying");
    bool show = config.readBoolEntry("Show", false);
    action<KToggleAction>("showNowPlaying")->setChecked(show);
    setShown(show);

}

void NowPlaying::saveConfig()
{
    KConfigGroup config(KGlobal::config(), "NowPlaying");
    config.writeEntry("Show", action<KToggleAction>("showNowPlaying")->isChecked());
}

void NowPlaying::setupActions()
{
    KToggleAction *show =
        new KToggleAction(i18n("Show Now Playing"), "player_play", 0, actions(), "showNowPlaying");
    show->setCheckedState(i18n("Hide Now Playing"));
    connect(show, SIGNAL(toggled(bool)), this, SLOT(setShown(bool)));
}

void NowPlaying::setupLayout()
{
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    QHBoxLayout *hLayout = new QHBoxLayout(vLayout);
    m_button = new QPushButton(this);
    m_button->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    hLayout->addWidget(m_button);
    connect(m_button, SIGNAL(clicked()), this, SLOT(slotButtonPress()));
}

#include "nowplaying.moc"

