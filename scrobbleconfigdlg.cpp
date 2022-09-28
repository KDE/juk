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
#include "juk_debug.h"

#include <KLineEdit>
#include <KPasswordLineEdit>
#include <KLocalizedString>
#include <KMessageBox>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KWallet>

#include <QFormLayout>
#include <QLabel>
#include <QPushButton>

ScrobbleConfigDlg::ScrobbleConfigDlg(QWidget* parent)
  : QDialog(parent)
  , m_wallet(Scrobbler::openKWallet())
{
    setWindowTitle(i18n("Configure scrobbling..."));

    m_passwordEdit = new KPasswordLineEdit(this);
    m_usernameEdit = new KLineEdit(this);
    m_testButton = new QPushButton(i18n("Test login..."), this);
    m_testFeedbackLabel = new QLabel("");

    auto vboxLayout = new QVBoxLayout(this);

    QWidget *mainWidget = new QWidget();
    QFormLayout *layout = new QFormLayout(mainWidget);
    QLabel *infoLabel = new QLabel(i18n("Please enter your <a href=\"https://last.fm/\">last.fm</a> login credentials:"));
    infoLabel->setOpenExternalLinks(true);
    infoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    layout->addRow(infoLabel);
    layout->addRow(new QLabel(i18n("Username:")), m_usernameEdit);
    layout->addRow(new QLabel(i18n("Password:")), m_passwordEdit);
    layout->addRow(m_testButton);
    layout->addRow(m_testFeedbackLabel);

    auto dlgButtonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    vboxLayout->addWidget(mainWidget);
    vboxLayout->addStretch();
    vboxLayout->addWidget(dlgButtonBox);

    connect(m_passwordEdit, &KPasswordLineEdit::passwordChanged, this, &ScrobbleConfigDlg::valuesChanged);
    connect(m_usernameEdit, SIGNAL(textEdited(QString)), this, SLOT(valuesChanged()));
    connect(m_testButton, SIGNAL(clicked(bool)), this, SLOT(testLogin()));

    connect(dlgButtonBox, &QDialogButtonBox::accepted, this, &ScrobbleConfigDlg::save);
    connect(dlgButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    m_saveButton = dlgButtonBox->button(QDialogButtonBox::Save);

    // Loading credentials using either KWallet or KConfigGroup.
    if (m_wallet) {
        QMap<QString, QString> scrobblingCredentials;
        m_wallet->readMap("Scrobbling", scrobblingCredentials);

        if (scrobblingCredentials.contains("Username") && scrobblingCredentials.contains("Password")) {
            m_usernameEdit->setText(scrobblingCredentials.value("Username"));
            m_passwordEdit->setPassword(scrobblingCredentials.value("Password"));
        }
    } else {
        // Warning message, KWallet is safer than KConfig.
        KMessageBox::information(this, i18n("KWallet is unavailable, your Last.fm credentials will be stored without encryption."), i18n("KWallet is unavailable"));

        KConfigGroup config(KSharedConfig::openConfig(), "Scrobbling");
        m_usernameEdit->setText(config.readEntry("Username", ""));
        m_passwordEdit->setPassword(config.readEntry("Password", ""));
    }

    if (m_passwordEdit->password().isEmpty() || m_usernameEdit->text().isEmpty()) {
        m_saveButton->setEnabled(false);
        m_testButton->setEnabled(false);
    }
}

void ScrobbleConfigDlg::valuesChanged()
{
    m_testButton->setEnabled(
            !m_usernameEdit->text().isEmpty() &&
            !m_passwordEdit->password().isEmpty());
    m_saveButton->setEnabled(false);
}

void ScrobbleConfigDlg::save()
{
    QDialog::accept();

    if (m_wallet) {
        QMap<QString, QString> scrobblingCredentials;
        scrobblingCredentials.insert("Username", m_usernameEdit->text());
        scrobblingCredentials.insert("Password", m_passwordEdit->password());

        if (!m_wallet->writeMap("Scrobbling", scrobblingCredentials)) {
            qCCritical(JUK_LOG) << "Couldn't save Last.fm credentials using KWallet.";
        }

    } else {
        KConfigGroup config(KSharedConfig::openConfig(), "Scrobbling");
        config.writeEntry("Username", m_usernameEdit->text());
        config.writeEntry("Password", m_passwordEdit->password());
    }
}

void ScrobbleConfigDlg::testLogin()
{
    m_testFeedbackLabel->setText(i18n("Validating login..."));
    Scrobbler *scrobbler = new Scrobbler(this);
    connect(scrobbler, SIGNAL(validAuth()), this, SLOT(validLogin()));
    connect(scrobbler, SIGNAL(invalidAuth()), this, SLOT(invalidLogin()));
    setEnabled(false);
    scrobbler->getAuthToken(m_usernameEdit->text(), m_passwordEdit->password());
}

void ScrobbleConfigDlg::invalidLogin()
{
    m_testFeedbackLabel->setText(i18n("Login invalid."));
    setEnabled(true);
    sender()->deleteLater();
    m_saveButton->setEnabled(false);
}

void ScrobbleConfigDlg::validLogin()
{
    m_testFeedbackLabel->setText(i18n("Login valid."));
    setEnabled(true);
    sender()->deleteLater();
    m_saveButton->setEnabled(true);
}
