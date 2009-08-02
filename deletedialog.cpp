/***************************************************************************
    begin                : Tue Aug 31 21:59:58 EST 2004
    copyright            : (C) 2004, 2008 by Michael Pyne
    email                : michael.pyne@kdemail.net
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "deletedialog.h"
#include "ui_deletedialogbase.h"

#include <KApplication>
#include <KStandardGuiItem>
#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>
#include <kconfiggroup.h>

#include <QStringList>
#include <QCheckBox>

//////////////////////////////////////////////////////////////////////////////
// DeleteWidget implementation
//////////////////////////////////////////////////////////////////////////////

DeleteWidget::DeleteWidget(QWidget *parent)
    : QWidget(parent), m_ui(new Ui::DeleteDialogBase)
{
    m_ui->setupUi(this);

    setObjectName("delete_dialog_widget");

    KConfigGroup messageGroup(KGlobal::config()->group("FileRemover"));

    bool deleteInstead = messageGroup.readEntry("deleteInsteadOfTrash", false);
    slotShouldDelete(deleteInstead);
    m_ui->ddShouldDelete->setChecked(deleteInstead);

    // Forward on this signal.
    connect(m_ui->ddShouldDelete, SIGNAL(toggled(bool)), SIGNAL(signalShouldDelete(bool)));
}

void DeleteWidget::setFiles(const QStringList &files)
{
    m_ui->ddFileList->clear();
    m_ui->ddFileList->insertItems(0, files);
    m_ui->ddNumFiles->setText(i18np("<b>1</b> file selected.", "<b>%1</b> files selected.", files.count()));
}

bool DeleteWidget::shouldDelete() const
{
    return m_ui->ddShouldDelete->isChecked();
}

void DeleteWidget::slotShouldDelete(bool shouldDelete)
{
    if(shouldDelete) {
        m_ui->ddDeleteText->setText(i18n("<qt>These items will be <b>permanently "
            "deleted</b> from your hard disk.</qt>"));
        m_ui->ddWarningIcon->setPixmap(KIconLoader::global()->loadIcon("dialog-warning",
            KIconLoader::Desktop, KIconLoader::SizeLarge));
    }
    else {
        m_ui->ddDeleteText->setText(i18n("<qt>These items will be moved to the Trash Bin.</qt>"));
        m_ui->ddWarningIcon->setPixmap(KIconLoader::global()->loadIcon("user-trash-full",
            KIconLoader::Desktop, KIconLoader::SizeLarge));
    }
}

//////////////////////////////////////////////////////////////////////////////
// DeleteDialog implementation
//////////////////////////////////////////////////////////////////////////////

DeleteDialog::DeleteDialog(QWidget *parent) :
    KDialog(parent, Qt::WStyle_DialogBorder),
    m_trashGuiItem(i18n("&Send to Trash"), "user-trash-full")
{
    setObjectName("delete_dialog");
    setModal(true);
    setCaption(i18n("About to delete selected files"));
    setButtons(Ok | Cancel);
    setDefaultButton(Cancel);
    showButtonSeparator(true);

    m_widget = new DeleteWidget(this);
    setMainWidget(m_widget);

    m_widget->setMinimumSize(400, 300);

    // Trying to adjust for Qt bug with rich text where the layout is ignored
    // (something about not being able to get height-for-width on X11?)
    setMinimumSize(410, 326);
    adjustSize();

    slotShouldDelete(shouldDelete());

    connect(m_widget, SIGNAL(signalShouldDelete(bool)), SLOT(slotShouldDelete(bool)));
}

bool DeleteDialog::confirmDeleteList(const QStringList &condemnedFiles)
{
    m_widget->setFiles(condemnedFiles);

    return exec() == QDialog::Accepted;
}

void DeleteDialog::setFiles(const QStringList &files)
{
    m_widget->setFiles(files);
}

void DeleteDialog::accept()
{
    KConfigGroup messageGroup(KGlobal::config()->group("FileRemover"));

    // Save user's preference

    messageGroup.writeEntry("deleteInsteadOfTrash", shouldDelete());
    messageGroup.sync();

    KDialog::accept();
}

void DeleteDialog::slotShouldDelete(bool shouldDelete)
{
    setButtonGuiItem(Ok, shouldDelete ? KStandardGuiItem::del() : m_trashGuiItem);
}

#include "deletedialog.moc"

// vim: set et sw=4 tw=0 sta:
