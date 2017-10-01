/**
 * Copyright (C) 2003 Stefan Asserhall <stefan.asserhall@telia.com>
 * Copyright (C) 2007, 2008 Michael Pyne <mpyne@kde.org>
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

#include "keydialog.h"
#include "actioncollection.h"

#include <KConfig>
#include <KShortcutsEditor>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KGlobalAccel>
#include <KLocalizedString>

#include <QAction>
#include <QKeySequence>
#include <QList>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTimer>
#include <QDialogButtonBox>
#include <QButtonGroup>
#include <QRadioButton>
#include <QPushButton>
#include <QGroupBox>
#include <QString>

// Table of shortcut keys for each action, key group and three or four button modifier

struct KeyDialog::KeyInfo {
    QString action;
    QKeySequence shortcut[3];
};

const KeyDialog::KeyInfo KeyDialog::keyInfo[] = {
    { "playPause",
      { QKeySequence(),
        QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_P),
        QKeySequence(Qt::Key_MediaPlay) } },
    { "stop",
      { QKeySequence(),
        QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_S),
        QKeySequence(Qt::Key_MediaStop) } },
    { "back",
      { QKeySequence(),
        QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_Left),
        QKeySequence(Qt::Key_MediaPrevious) } },
    { "forward",
      { QKeySequence(),
        QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_Right),
        QKeySequence(Qt::Key_MediaNext) } },
    { "forwardAlbum",
      { QKeySequence(),
        QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_Up),
        QKeySequence(Qt::CTRL+Qt::Key_MediaNext) } },
    { "seekBack",
      { QKeySequence(),
        QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::ALT+Qt::Key_Left),
        QKeySequence(Qt::SHIFT+Qt::Key_MediaPrevious) } },
    { "seekForward",
      { QKeySequence(),
        QKeySequence(Qt::CTRL+Qt::SHIFT+Qt::ALT+Qt::Key_Right),
        QKeySequence(Qt::SHIFT+Qt::Key_MediaNext) } },
    { "volumeUp",
      { QKeySequence(),
        QKeySequence(Qt::CTRL+Qt::ALT+Qt::SHIFT+Qt::Key_Up),
        QKeySequence(Qt::Key_VolumeUp) } },
    { "volumeDown",
      { QKeySequence(),
        QKeySequence(Qt::CTRL+Qt::ALT+Qt::SHIFT+Qt::Key_Down),
        QKeySequence(Qt::Key_VolumeDown) } },
    { "mute",
      { QKeySequence(),
        QKeySequence(Qt::CTRL+Qt::ALT+Qt::Key_M),
        QKeySequence(Qt::Key_VolumeMute) } },
    { "showHide",
      { QKeySequence(),
        QKeySequence(),
        QKeySequence() } }
};

const uint KeyDialog::keyInfoCount = sizeof(KeyDialog::keyInfo) / sizeof(KeyDialog::keyInfo[0]);

KeyDialog::KeyDialog(KActionCollection *actionCollection, QWidget *parent)
  : QDialog(parent)
  , m_actionCollection(actionCollection)
{
    setWindowTitle(i18n("Configure Shortcuts"));

    // Read key group from configuration

    KConfigGroup config(KSharedConfig::openConfig(), "Shortcuts");
    int selectedButton = config.readEntry("GlobalKeys", int(StandardKeys));

    // Create widgets for key chooser

    auto vboxLayout = new QVBoxLayout(this);

    m_pKeyChooser = new KShortcutsEditor(actionCollection, this);
    vboxLayout->addWidget(m_pKeyChooser);

    // Create buttons to select key group

    QGroupBox *buttonBox = new QGroupBox(i18n("Global Shortcuts"), this);
    vboxLayout->addWidget(buttonBox);

    m_group = new QButtonGroup(buttonBox);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonBox);

    QRadioButton *radioButton = new QRadioButton(i18n("&No keys"), buttonBox);
    m_group->addButton(radioButton, NoKeys);
    buttonLayout->addWidget(radioButton);

    radioButton = new QRadioButton(i18n("&Standard keys"), buttonBox);
    m_group->addButton(radioButton, StandardKeys);
    buttonLayout->addWidget(radioButton);

    radioButton = new QRadioButton(i18n("&Multimedia keys"), buttonBox);
    m_group->addButton(radioButton, MultimediaKeys);
    buttonLayout->addWidget(radioButton);

    connect(m_group, SIGNAL(buttonClicked(int)), SLOT(slotKeys(int)));
    buttonBox->setWhatsThis(
        i18n("Here you can select the keys used as global shortcuts to control the player"));

    m_group->button(selectedButton)->setChecked(true);

    auto dlgButtonBox = new QDialogButtonBox(
            QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::RestoreDefaults,
            this);
    vboxLayout->addWidget(dlgButtonBox);

    connect(dlgButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(dlgButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(dlgButtonBox->button(QDialogButtonBox::RestoreDefaults), &QAbstractButton::clicked,
            this,                                                    &KeyDialog::slotDefault);

    resize(400, 500); // TODO: Make it bigger!
}

int KeyDialog::configure()
{
    // Show the dialog and save configuration if accepted

    int retcode = exec();
    if(retcode == Accepted) {
        KConfigGroup config(KSharedConfig::openConfig(), "Shortcuts");

        config.writeEntry("GlobalKeys", m_group->checkedId());
        KSharedConfig::openConfig()->sync();

        m_pKeyChooser->save();
    }

    return retcode;
}

void KeyDialog::slotKeys(int group)
{
    // Set modifier keys according to key group and modifier keys

    auto globalAccel = KGlobalAccel::self();

    for(uint i = 0; i < keyInfoCount; i++) {
        QAction *a = ActionCollection::action<QAction>(keyInfo[i].action);

        if(a) {
            if (group == 0) {
                globalAccel->removeAllShortcuts(a);
            }
            else {
                QKeySequence shortcut(keyInfo[i].shortcut[group]);
                QList<QKeySequence> shortcutList{shortcut};
                globalAccel->setShortcut(a, shortcutList, KGlobalAccel::NoAutoloading);
            }
        }
    }

    // Update the key chooser widget.
    // TODO: When widget is fixed to note the update remove this bit so that
    // we don't drop user changes for no reason.

    m_pKeyChooser->clearCollections();
    m_pKeyChooser->addCollection(m_actionCollection);
}

void KeyDialog::slotDefault()
{
    // Select default keys - standard key group

    m_group->button(StandardKeys)->setChecked(true);
    slotKeys(StandardKeys);
    m_pKeyChooser->allDefault();
}

void KeyDialog::setupActionShortcut(const QString &actionName)
{
    // Find and insert a standard key
    QKeySequence shortcut = QKeySequence();

    // Find out what type is selected so we know what keys to setup.
    KConfigGroup config(KSharedConfig::openConfig(), "Shortcuts");
    int selectedKeys = config.readEntry("GlobalKeys", int(StandardKeys));

    for(uint i = 0; i < keyInfoCount; i++) {
        if(keyInfo[i].action == actionName) {
            shortcut = keyInfo[i].shortcut[selectedKeys];
            break;
        }
    }

    if(shortcut.isEmpty())
        return; // We have no shortcut to set.

    QAction *a = ActionCollection::action<QAction>(actionName);
    if(a) {
        KGlobalAccel::setGlobalShortcut(a, shortcut);
    }
}

// vim: set et sw=4 tw=0 sta:
