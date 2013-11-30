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

#include <kconfig.h>
#include <klocale.h>
#include <kshortcutseditor.h>
#include <kglobal.h>
#include <kaction.h>
#include <kvbox.h>
#include <kconfiggroup.h>
#include <KShortcut>

#include <QHBoxLayout>
#include <QTimer>
#include <QButtonGroup>
#include <QRadioButton>
#include <QGroupBox>
#include <QString>

// Table of shortcut keys for each action, key group and three or four button modifier

struct KeyDialog::KeyInfo {
    QString action;
    KShortcut shortcut[3];
};

const KeyDialog::KeyInfo KeyDialog::keyInfo[] = {
    { "playPause",
      { KShortcut(),
        KShortcut(Qt::CTRL+Qt::ALT+Qt::Key_P),
        KShortcut(Qt::Key_MediaPlay) } },
    { "stop",
      { KShortcut(),
        KShortcut(Qt::CTRL+Qt::ALT+Qt::Key_S),
        KShortcut(Qt::Key_MediaStop) } },
    { "back",
      { KShortcut(),
        KShortcut(Qt::CTRL+Qt::ALT+Qt::Key_Left),
        KShortcut(Qt::Key_MediaPrevious) } },
    { "forward",
      { KShortcut(),
        KShortcut(Qt::CTRL+Qt::ALT+Qt::Key_Right),
        KShortcut(Qt::Key_MediaNext) } },
    { "forwardAlbum",
      { KShortcut(),
        KShortcut(Qt::CTRL+Qt::ALT+Qt::Key_Up),
        KShortcut(Qt::CTRL+Qt::Key_MediaNext) } },
    { "seekBack",
      { KShortcut(),
        KShortcut(Qt::CTRL+Qt::SHIFT+Qt::ALT+Qt::Key_Left),
        KShortcut(Qt::SHIFT+Qt::Key_MediaPrevious) } },
    { "seekForward",
      { KShortcut(),
        KShortcut(Qt::CTRL+Qt::SHIFT+Qt::ALT+Qt::Key_Right),
        KShortcut(Qt::SHIFT+Qt::Key_MediaNext) } },
    { "volumeUp",
      { KShortcut(),
        KShortcut(Qt::CTRL+Qt::ALT+Qt::SHIFT+Qt::Key_Up),
        KShortcut(Qt::Key_VolumeUp) } },
    { "volumeDown",
      { KShortcut(),
        KShortcut(Qt::CTRL+Qt::ALT+Qt::SHIFT+Qt::Key_Down),
        KShortcut(Qt::Key_VolumeDown) } },
    { "mute",
      { KShortcut(),
        KShortcut(Qt::CTRL+Qt::ALT+Qt::Key_M),
        KShortcut(Qt::Key_VolumeMute) } },
    { "showHide",
      { KShortcut(),
        KShortcut(),
        KShortcut() } }
};

const uint KeyDialog::keyInfoCount = sizeof(KeyDialog::keyInfo) / sizeof(KeyDialog::keyInfo[0]);

KeyDialog::KeyDialog(KActionCollection *actionCollection, QWidget *parent)
    : KDialog(parent), m_actionCollection(actionCollection)
{
    setCaption(i18n("Configure Shortcuts"));
    setButtons(Default | Ok | Cancel);

    // Read key group from configuration

    KConfigGroup config(KGlobal::config(), "Shortcuts");
    int selectedButton = config.readEntry("GlobalKeys", int(StandardKeys));

    // Create widgets for key chooser

    KVBox *vbox = new KVBox(this);
    vbox->setSpacing(KDialog::spacingHint());

    m_pKeyChooser = new KShortcutsEditor(actionCollection, vbox);

    // Create buttons to select key group

    QGroupBox *buttonBox = new QGroupBox(i18n("Global Shortcuts"), vbox);
    m_group = new QButtonGroup(buttonBox);
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonBox->setLayout(buttonLayout);

    QRadioButton *radioButton = new QRadioButton(i18n("&No keys"), buttonBox);
    m_group->addButton(radioButton, NoKeys);
    buttonLayout->addWidget(radioButton);

    radioButton = new QRadioButton(i18n("&Standard keys"), buttonBox);
    m_group->addButton(radioButton, StandardKeys);
    buttonLayout->addWidget(radioButton);

    radioButton = new QRadioButton(i18n("&Multimedia keys"), buttonBox);
    m_group->addButton(radioButton, MultimediaKeys);
    buttonLayout->addWidget(radioButton);

    connect(m_group, SIGNAL(buttonClicked(int)), this, SLOT(slotKeys(int)));
    buttonBox->setWhatsThis(
	i18n("Here you can select the keys used as global shortcuts to control the player"));

    m_group->button(selectedButton)->setChecked(true);
    connect(this, SIGNAL(defaultClicked()), this, SLOT(slotDefault()));

    setMainWidget(vbox);
    resize(400, 500); // Make it bigger!
}

KeyDialog::~KeyDialog()
{
}

int KeyDialog::configure()
{
    // Show the dialog and save configuration if accepted

    int retcode = exec();
    if(retcode == Accepted) {
        KConfigGroup config(KGlobal::config(), "Shortcuts");

        config.writeEntry("GlobalKeys", m_group->checkedId());
        KGlobal::config()->sync();

        m_pKeyChooser->save();
    }

    return retcode;
}

void KeyDialog::slotKeys(int group)
{
    // Set modifier keys according to key group and modifier keys

    for(uint i = 0; i < keyInfoCount; i++) {
        KAction *a = ActionCollection::action<KAction>(keyInfo[i].action);

        if(a) {
            KShortcut shortcut(keyInfo[i].shortcut[group]);

            a->setGlobalShortcut(shortcut, KAction::ActiveShortcut, KAction::NoAutoloading);
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

int KeyDialog::configure(KActionCollection *actionCollection, QWidget *parent)
{
    // Create and show dialog - update connections if accepted

    return KeyDialog(actionCollection, parent).configure();
}

void KeyDialog::setupActionShortcut(const QString &actionName)
{
    // Find and insert a standard key
    KShortcut shortcut = KShortcut();

    // Find out what type is selected so we know what keys to setup.
    KConfigGroup config(KGlobal::config(), "Shortcuts");
    int selectedKeys = config.readEntry("GlobalKeys", int(StandardKeys));

    for(uint i = 0; i < keyInfoCount; i++) {
        if(keyInfo[i].action == actionName) {
            shortcut = keyInfo[i].shortcut[selectedKeys];
            break;
        }
    }

    if(shortcut.isEmpty())
        return; // We have no shortcut to set.

    KAction *a = ActionCollection::action<KAction>(actionName);
    if(a)
        a->setGlobalShortcut(shortcut);
}

#include "keydialog.moc"

// vim: set et sw=4 tw=0 sta:
