/**
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

#include "scrobbleconfigdlg.h"
#include "scrobbler.h"
#include <KLineEdit>
#include <KPushButton>
#include <KLocalizedString>
#include <QLayout>
#include <QLabel>
#include <QFormLayout>



ScrobbleConfigDlg::ScrobbleConfigDlg(QWidget* parent, Qt::WindowFlags f)
    : KDialog(parent, f)
{
    setWindowTitle(i18n("Configure scrobbling..."));
    
    setButtons(Apply | Cancel);
    
    m_passwordEdit = new KLineEdit(this);
    m_passwordEdit->setPasswordMode(true);
    m_usernameEdit = new KLineEdit(this);
    m_testButton = new KPushButton(i18n("Test login..."), this);
    m_testFeedbackLabel = new QLabel("");
    
    QWidget *mainWidget = new QWidget();
    QFormLayout *layout = new QFormLayout();
    mainWidget->setLayout(layout);
    QLabel *infoLabel = new QLabel(i18n("Please enter your <a href=\"http://last.fm/\">last.fm</a> login credentials:"));
    infoLabel->setOpenExternalLinks(true);
    infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout->addRow(infoLabel);
    layout->addRow(new QLabel(i18n("Username:")), m_usernameEdit);
    layout->addRow(new QLabel(i18n("Password:")), m_passwordEdit);
    layout->addRow(m_testButton);
    layout->addRow(m_testFeedbackLabel);
    
    connect(m_passwordEdit, SIGNAL(textEdited(QString)), this, SLOT(valuesChanged()));
    connect(m_usernameEdit, SIGNAL(textEdited(QString)), this, SLOT(valuesChanged()));
    connect(m_testButton, SIGNAL(clicked(bool)), this, SLOT(testLogin()));
    connect(this, SIGNAL(applyClicked()), this, SLOT(save()));
    
    setMainWidget(mainWidget);
    
    KConfigGroup config(KGlobal::config(), "Scrobbling");
    m_usernameEdit->setText(config.readEntry("Username", ""));//TODO: use kwallet
    m_passwordEdit->setText(config.readEntry("Password", ""));//TODO: use kwallet
    
    if (m_passwordEdit->text().isEmpty() || m_usernameEdit->text().isEmpty())
        button(Apply)->setEnabled(false);
}

void ScrobbleConfigDlg::valuesChanged()
{
    button(Apply)->setEnabled(false);
}

void ScrobbleConfigDlg::save()
{
    QDialog::accept();
    
    KConfigGroup config(KGlobal::config(), "Scrobbling");
    config.writeEntry("Username", m_usernameEdit->text());
    config.writeEntry("Password", m_passwordEdit->text());
}

void ScrobbleConfigDlg::testLogin()
{
    m_testFeedbackLabel->setText(i18n("Validating login..."));
    Scrobbler *scrobbler = new Scrobbler(this);
    connect(scrobbler, SIGNAL(validAuth()), this, SLOT(validLogin()));
    connect(scrobbler, SIGNAL(invalidAuth()), this, SLOT(invalidLogin()));
    setEnabled(false);
    scrobbler->getAuthToken(m_usernameEdit->text(), m_passwordEdit->text());
}

void ScrobbleConfigDlg::invalidLogin()
{
    m_testFeedbackLabel->setText(i18n("Login invalid."));
    setEnabled(true);
    sender()->deleteLater();
    button(Apply)->setEnabled(false);
}

void ScrobbleConfigDlg::validLogin()
{
    m_testFeedbackLabel->setText(i18n("Login valid."));
    setEnabled(true);
    sender()->deleteLater();
    button(Apply)->setEnabled(true);
}
