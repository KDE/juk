/**
 * Copyright (C) 2004 Michael Pyne <mpyne@kde.org>
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

#include "filerenameroptions.h"

#include <klocale.h>
#include <knuminput.h>

#include "juk_debug.h"

FileRenamerTagOptions::FileRenamerTagOptions(QWidget *parent,
                                             const TagRenamerOptions &options)
  : QWidget(parent)
  , Ui::FileRenamerTagOptionsBase()
  , m_options(options)
{
    setupUi(this);

    layout()->setMargin(0);

    if(m_options.category() != Track)
        m_trackGroup->hide();

    QString tagText = m_options.tagTypeText();

    setWindowTitle(i18nc("%1 will be a music tag category like Artist or Album", "%1 Options",tagText));
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
                                   unsigned categoryNumber)
  : QDialog(parent)
  , m_options(options)
  , m_categoryNumber(categoryNumber)
{
    setModal(true);
    setWindowTitle(i18n("File Renamer"));

    loadConfig();

    m_widget = new FileRenamerTagOptions(this, m_options);
    m_widget->setMinimumSize(400, 200);

    connect(m_widget->dlgButtonBox, &QDialogButtonBox::accepted,
            this,                   &QDialog::accept);
    connect(m_widget->dlgButtonBox, &QDialogButtonBox::rejected,
            this,                   &QDialog::reject);

    auto boxLayout = new QVBoxLayout(this);
    boxLayout->addWidget(m_widget);
}

void TagOptionsDialog::accept()
{
    m_options = m_widget->options();

    saveConfig();
    QDialog::accept();
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

// vim: set et sw=4 tw=0 sta:
