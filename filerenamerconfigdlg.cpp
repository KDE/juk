/*
 * filerenamerconfigdlg.cpp - (c) 2003 Frerich Raabe <raabe@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "filerenamer.h"
#include "filerenamerconfigdlg.h"
#include "filerenamerconfigdlgwidget.h"
#include "tag.h"

#include <kapplication.h>
#include <kconfig.h>
#include <klineedit.h>
#include <klocale.h>
#include <kurlrequester.h>

#include <qcheckbox.h>
#include <qlabel.h>

FileRenamerConfigDlg::FileRenamerConfigDlg(QWidget *parent, const char *name)
    : KDialogBase(parent, name, true, i18n("File Renamer Configuration"),
                  Ok | Cancel, Ok, true)
{
    m_child = new FileRenamerConfigDlgWidget(this, "child");
    m_child->urlReqCurrentFilename->setMode(KFile::File | KFile::ExistingOnly);
    connect(m_child->leFilenameScheme, SIGNAL(textChanged(const QString &)),
            this, SLOT(stateChanged(const QString &)));
    connect(m_child->leTitleToken, SIGNAL(textChanged(const QString &)),
            this, SLOT(stateChanged(const QString &)));
    connect(m_child->cbNeedTitle, SIGNAL(clicked()), SLOT(stateChanged()));
    connect(m_child->leArtistToken, SIGNAL(textChanged(const QString &)),
            this, SLOT(stateChanged(const QString &)));
    connect(m_child->cbNeedArtist, SIGNAL(clicked()), SLOT(stateChanged()));
    connect(m_child->leAlbumToken, SIGNAL(textChanged(const QString &)),
            this, SLOT(stateChanged(const QString &)));
    connect(m_child->cbNeedAlbum, SIGNAL(clicked()), SLOT(stateChanged()));
    connect(m_child->leTrackToken, SIGNAL(textChanged(const QString &)),
            this, SLOT(stateChanged(const QString &)));
    connect(m_child->cbNeedTrack, SIGNAL(clicked()), SLOT(stateChanged()));
    connect(m_child->leCommentToken, SIGNAL(textChanged(const QString &)),
            this, SLOT(stateChanged(const QString &)));
    connect(m_child->cbNeedComment, SIGNAL(clicked()), SLOT(stateChanged()));
    connect(m_child->urlReqCurrentFilename, SIGNAL(textChanged(const QString &)),
            this, SLOT(currentFilenameChanged(const QString &)));
    setMainWidget(m_child);

    loadSettings();

    resize( 400, 300 );
}

void FileRenamerConfigDlg::accept()
{
    saveSettings();
    KDialogBase::accept();
}

void FileRenamerConfigDlg::currentFilenameChanged(const QString &curFilename)
{
    Tag *tag = Tag::createTag(curFilename, true);
    if(tag == 0) {
        m_child->lNewFilename->setText(i18n("Could not read tag from file"));
        return;
    }

    FileRenamer renamer;
    m_child->lNewFilename->setText(renamer.rename(curFilename, *tag));

    delete tag;
}

void FileRenamerConfigDlg::loadSettings()
{
    const FileRenamer::Config cfg(kapp->config());
    m_child->leFilenameScheme->setText(cfg.filenameScheme());
    m_child->leTitleToken->setText(cfg.getToken(FileRenamer::Title));
    m_child->cbNeedTitle->setChecked(cfg.tokenNeedsValue(FileRenamer::Title));
    m_child->leArtistToken->setText(cfg.getToken(FileRenamer::Artist));
    m_child->cbNeedArtist->setChecked(cfg.tokenNeedsValue(FileRenamer::Artist));
    m_child->leAlbumToken->setText(cfg.getToken(FileRenamer::Album));
    m_child->cbNeedAlbum->setChecked(cfg.tokenNeedsValue(FileRenamer::Album));
    m_child->leTrackToken->setText(cfg.getToken(FileRenamer::Track));
    m_child->cbNeedTrack->setChecked(cfg.tokenNeedsValue(FileRenamer::Track));
    m_child->leCommentToken->setText(cfg.getToken(FileRenamer::Comment));
    m_child->cbNeedComment->setChecked(cfg.tokenNeedsValue(FileRenamer::Comment));
}

void FileRenamerConfigDlg::saveSettings()
{
    FileRenamer::Config cfg(kapp->config());
    cfg.setFilenameScheme(m_child->leFilenameScheme->text());
    cfg.setToken(FileRenamer::Title, m_child->leTitleToken->text());
    cfg.setTokenNeedsValue(FileRenamer::Title, m_child->cbNeedTitle->isChecked());
    cfg.setToken(FileRenamer::Artist, m_child->leArtistToken->text());
    cfg.setTokenNeedsValue(FileRenamer::Artist, m_child->cbNeedArtist->isChecked());
    cfg.setToken(FileRenamer::Album, m_child->leAlbumToken->text());
    cfg.setTokenNeedsValue(FileRenamer::Album, m_child->cbNeedAlbum->isChecked());
    cfg.setToken(FileRenamer::Track, m_child->leTrackToken->text());
    cfg.setTokenNeedsValue(FileRenamer::Track, m_child->cbNeedTrack->isChecked());
    cfg.setToken(FileRenamer::Comment, m_child->leCommentToken->text());
    cfg.setTokenNeedsValue(FileRenamer::Comment, m_child->cbNeedComment->isChecked());
    kapp->config()->sync();
}

void FileRenamerConfigDlg::stateChanged(const QString &)
{
    if(m_child->urlReqCurrentFilename->url().isEmpty())
        return;
    saveSettings();
    currentFilenameChanged(m_child->urlReqCurrentFilename->url());
}

#include "filerenamerconfigdlg.moc"
// vim:ts=4:sw=4:et
