/***************************************************************************
    begin                : Thu Oct 28 2004
    copyright            : (C) 2004 by Michael Pyne
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

#include "filerenameroptions.h"

#include <klocale.h>
#include <kdebug.h>
#include <knuminput.h>


FileRenamerTagOptions::FileRenamerTagOptions(QWidget *parent,
                                             const TagRenamerOptions &options) :
    QWidget(parent),
    Ui::FileRenamerTagOptionsBase(),
    m_options(options)
{
    setupUi(this);

    layout()->setMargin(0);

    if(m_options.category() != Track)
        m_trackGroup->hide();

    QString tagText = m_options.tagTypeText();

    setWindowTitle(i18n("%1 Options",tagText));
    m_tagFormatGroup->setTitle(i18n("%1 Format",tagText));
    m_emptyTagGroup->setTitle(i18n("When the Track's %1 is Empty",tagText));
    m_description->setText(i18n("When using the file renamer your files will be renamed to the values that you have in your track's %1 tag, plus any additional text that you specify below.",tagText));
    m_tagLabel->setText(tagText);

    m_prefixText->setText(options.prefix());
    m_suffixText->setText(options.suffix());
    if(options.emptyAction() == TagRenamerOptions::ForceEmptyInclude)
        m_includeEmptyButton->setChecked(true);
    else if(options.emptyAction() == TagRenamerOptions::UseReplacementValue)
        m_useValueButton->setChecked(true);
    m_emptyTagValue->setText(options.emptyText());
    m_trackWidth->setValue(options.trackWidth());

    slotBracketsChanged();
    slotEmptyActionChanged();
    slotTrackWidthChanged();
}

void FileRenamerTagOptions::slotBracketsChanged()
{
    QString tag = m_options.tagTypeText();

    m_options.setPrefix(m_prefixText->text());
    m_options.setSuffix(m_suffixText->text());

    m_substitution->setText(m_options.prefix() + tag + m_options.suffix());
}

void FileRenamerTagOptions::slotTrackWidthChanged()
{
    int width = m_trackWidth->value();

    m_options.setTrackWidth(width);

    QString singleDigitText = m_singleDigit->text();
    singleDigitText.remove(" ->");
    QString doubleDigitText = m_doubleDigit->text();
    doubleDigitText.remove(" ->");

    if(singleDigitText.length() < width) {
        QString p;
        p.fill('0', width - singleDigitText.length());
        singleDigitText.prepend(p);
    }

    if(doubleDigitText.length() < width) {
        QString p;
        p.fill('0', width - doubleDigitText.length());
        doubleDigitText.prepend(p);
    }

    m_singleDigitExample->setText(singleDigitText);
    m_doubleDigitExample->setText(doubleDigitText);
}

void FileRenamerTagOptions::slotEmptyActionChanged()
{
    m_options.setEmptyText(m_emptyTagValue->text());

    m_options.setEmptyAction(TagRenamerOptions::IgnoreEmptyTag);

    if(m_useValueButton->isChecked())
        m_options.setEmptyAction(TagRenamerOptions::UseReplacementValue);
    else if(m_includeEmptyButton->isChecked())
        m_options.setEmptyAction(TagRenamerOptions::ForceEmptyInclude);
}

TagOptionsDialog::TagOptionsDialog(QWidget *parent,
                                   const TagRenamerOptions &options,
                                   unsigned categoryNumber) :
    KDialog(parent, 0),
    m_options(options),
    m_categoryNumber(categoryNumber)
{
    setModal(true);
    setCaption(i18n("File Renamer"));
    setButtons(Ok|Cancel);

    loadConfig();

    m_widget = new FileRenamerTagOptions(this, m_options);
    m_widget->setMinimumSize(400, 200);

    setMainWidget(m_widget);
}

void TagOptionsDialog::accept()
{
    m_options = m_widget->options();

    saveConfig();
    KDialog::accept();
}

void TagOptionsDialog::loadConfig()
{
    // Our m_options may not have been loaded from KConfig, force that to
    // happen.

    CategoryID category(m_options.category(), m_categoryNumber);
    m_options = TagRenamerOptions(category);
}

void TagOptionsDialog::saveConfig()
{
    m_options.saveConfig(m_categoryNumber);
}

#include "filerenameroptions.moc"

// vim: set et sw=4 tw=0 sta:
