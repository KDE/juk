/**
 * Copyright (C) 2012 Martin Sandsmark <martin.sandsmark@kde.org>
 * Copyright (C) 2014 Arnold Dumas <contact@arnolddumas.fr>
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

#include <KDebug>
#include <KLineEdit>
#include <QPushButton>
#include <KLocalizedString>
#include <KMessageBox>
#include <kglobal.h>

#include <QLayout>
#include <QLabel>
#include <QFormLayout>



ScrobbleConfigDlg::ScrobbleConfigDlg(QWidget* parent, Qt::WindowFlags f)
    : KDialog(parent, f)
    , m_wallet(0)
{
    setWindowTitle(i18n("Configure scrobbling..."));
    
    setButtons(Apply | Cancel);
    
    m_passwordEdit = new KLineEdit(this);
    m_passwordEdit->setPasswordMode(true);
    m_usernameEdit = new KLineEdit(this);
    m_testButton = new QPushButton(i18n("Test login..."), this);
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

    // Loading credentials using either KWallet or KConfigGroup.
    m_wallet = Scrobbler::openKWallet();

    if (m_wallet) {

        QMap<QString, QString> scrobblingCredentials;
        m_wallet->readMap("Scrobbling", scrobblingCredentials);

        if (scrobblingCredentials.contains("Username") && scrobblingCredentials.contains("Password")) {
            m_usernameEdit->setText(scrobblingCredentials.value("Username"));
            m_passwordEdit->setText(scrobblingCredentials.value("Password"));
        }

    } else {

        // Warning message, KWallet is safer than KConfig.
        KMessageBox::information(this, i18n("KWallet is unavailable, your Last.fm credentials will be stored without encryption."), i18n("KWallet is unavailable"));

        KConfigGroup config(KGlobal::config(), "Scrobbling");
        m_usernameEdit->setText(config.readEntry("Username", ""));
        m_passwordEdit->setText(config.readEntry("Password", ""));
    }

    if (m_passwordEdit->text().isEmpty() || m_usernameEdit->text().isEmpty()) {
        button(Apply)->setEnabled(false);
        m_testButton->setEnabled(false);
    }
}

ScrobbleConfigDlg::~ScrobbleConfigDlg()
{
    delete m_wallet;
}

void ScrobbleConfigDlg::valuesChanged()
{
    if (m_usernameEdit->text().isEmpty() || m_passwordEdit->text().isEmpty())
        m_testButton->setEnabled(false);

    else
        m_testButton->setEnabled(true);

    button(Apply)->setEnabled(false);
}

void ScrobbleConfigDlg::save()
{
    QDialog::accept();

    if (m_wallet) {

        QMap<QString, QString> scrobblingCredentials;
        scrobblingCredentials.insert("Username", m_usernameEdit->text());
        scrobblingCredentials.insert("Password", m_passwordEdit->text());

        if (!m_wallet->writeMap("Scrobbling", scrobblingCredentials)) {

            kError() << "Couldn't save Last.fm credentials using KWallet.";
        }

    } else {

        KConfigGroup config(KGlobal::config(), "Scrobbling");
        config.writeEntry("Username", m_usernameEdit->text());
        config.writeEntry("Password", m_passwordEdit->text());
    }
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
