/***************************************************************************
    begin                : Tue Mar 11 19:00:00 CET 2003
    copyright            : (C) 2003 by Stefan Asserhall
    email                : stefan.asserhall@telia.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "keydialog.h"

#include <kconfig.h>
#include <klocale.h>

#include <qradiobutton.h>
#include <q3vbox.h>



// Table of shortcut keys for each action, key group and three or four button modifier

const KeyDialog::KeyInfo KeyDialog::keyInfo[] = {
    { "PlayPause",
      { { KShortcut::null(),                        KShortcut::null() },
        { Qt::CTRL+Qt::ALT+Qt::Key_P,               Qt::META+Qt::ALT+Qt::Key_P },
        { Qt::Key_MediaPlay,                        Qt::Key_MediaPlay } } },
    { "Stop",
      { { KShortcut::null(),                        KShortcut::null() },
        { Qt::CTRL+Qt::ALT+Qt::Key_S,               Qt::META+Qt::ALT+Qt::Key_S },
        { Qt::Key_MediaStop,                        Qt::Key_MediaStop } } },
    { "Back",
      { { KShortcut::null(),                        KShortcut::null() },
        { Qt::CTRL+Qt::ALT+Qt::Key_Left,            Qt::META+Qt::ALT+Qt::Key_Left },
        { Qt::Key_MediaPrevious   ,                        Qt::Key_MediaPrevious } } },
    { "Forward",
      { { KShortcut::null(),                        KShortcut::null() },
        { Qt::CTRL+Qt::ALT+Qt::Key_Right,           Qt::META+Qt::ALT+Qt::Key_Right },
        { Qt::Key_MediaNext,                        Qt::Key_MediaNext } } },
    { "ForwardAlbum",
      { { KShortcut::null(),                        KShortcut::null() },
        { Qt::CTRL+Qt::ALT+Qt::Key_Up,              Qt::META+Qt::ALT+Qt::Key_Up },
        { Qt::CTRL+Qt::Key_MediaNext,               Qt::CTRL+Qt::Key_MediaNext } } },
    { "SeekBack",
      { { KShortcut::null(),                        KShortcut::null() },
        { Qt::CTRL+Qt::SHIFT+Qt::ALT+Qt::Key_Left,  Qt::META+Qt::SHIFT+Qt::ALT+Qt::Key_Left },
        { Qt::SHIFT+Qt::Key_MediaPrevious,              Qt::SHIFT+Qt::Key_MediaPrevious } } },
    { "SeekForward",
      { { KShortcut::null(),                        KShortcut::null() },
        { Qt::CTRL+Qt::SHIFT+Qt::ALT+Qt::Key_Right, Qt::META+Qt::SHIFT+Qt::ALT+Qt::Key_Right },
        { Qt::SHIFT+Qt::Key_MediaNext,              Qt::SHIFT+Qt::Key_MediaNext } } },
    { "VolumeUp",
      { { KShortcut::null(),                        KShortcut::null() },
        { Qt::CTRL+Qt::ALT+Qt::SHIFT+Qt::Key_Up,    Qt::META+Qt::ALT+Qt::SHIFT+Qt::Key_Up },
        { Qt::Key_VolumeUp,                         Qt::Key_VolumeUp } } },
    { "VolumeDown",
      { { KShortcut::null(),                        KShortcut::null() },
        { Qt::CTRL+Qt::ALT+Qt::SHIFT+Qt::Key_Down,  Qt::META+Qt::ALT+Qt::SHIFT+Qt::Key_Down },
        { Qt::Key_VolumeDown,                       Qt::Key_VolumeDown } } },
    { "Mute",
      { { KShortcut::null(),                        KShortcut::null() },
        { Qt::CTRL+Qt::ALT+Qt::Key_M,               Qt::META+Qt::ALT+Qt::Key_M },
        { Qt::Key_VolumeMute,                       Qt::Key_VolumeMute } } },
    { "ShowHide",
      { { KShortcut::null(),                        KShortcut::null() },
        { KShortcut::null(),                        KShortcut::null() },
        { KShortcut::null(),                        KShortcut::null() } } }
};

const uint KeyDialog::keyInfoCount = sizeof(KeyDialog::keyInfo) / sizeof(KeyDialog::keyInfo[0]);

KeyDialog::KeyDialog(KGlobalAccel *keys, KActionCollection *actionCollection,
                     QWidget *parent, const char *name)
    : KDialogBase(parent, name, true, i18n("Configure Shortcuts"), Default | Ok | Cancel, Ok)
{
    // Read key group from configuration

    KConfigGroup config(KGlobal::config(), "Shortcuts");
    int selectedButton = config.readEntry("GlobalKeys", int(StandardKeys));

    // Create widgets for key chooser - widget stack used to replace key chooser

    Q3VBox *vbox = new Q3VBox(this);
    vbox->setSpacing(KDialog::spacingHint());
    m_widgetStack = new Q3WidgetStack(vbox);

    vbox->setStretchFactor(m_widgetStack, 1);

    // Create buttons to select key group

    #warning Another place that probably needs some post-mortem analysis.

    m_group = new QButtonGroup(/* i18n("Global Shortcuts"), */ vbox);
    new QRadioButton(i18n("&No keys"), /* m_group */ vbox);
    new QRadioButton(i18n("&Standard keys"), /* m_group */ vbox);
    new QRadioButton(i18n("&Multimedia keys"), /* m_group */ vbox);
    connect(m_group, SIGNAL(clicked(int)), this, SLOT(slotKeys(int)));
    /*m_group*/ vbox->setWhatsThis(
	i18n("Here you can select the keys used as global shortcuts to control the player"));

    // Create the key chooser

    setMainWidget(vbox);
    newDialog(keys, actionCollection, selectedButton);
}

KeyDialog::~KeyDialog()
{

}

void KeyDialog::newDialog(KGlobalAccel *keys, KActionCollection *actionCollection,
                          int selectedButton)
{
    m_keys = keys;
    m_actionCollection = actionCollection;

    // Create key chooser and show it in the widget stack
    m_pKeyChooser = new KKeyChooser(keys, this);
    m_pKeyChooser->insert(actionCollection);
    m_widgetStack->addWidget(m_pKeyChooser);
    m_widgetStack->raiseWidget(m_pKeyChooser);

    #warning More broken code
    /* m_group->setButton(selectedButton); */

    connect(this, SIGNAL(defaultClicked()), this, SLOT(slotDefault()));
}

int KeyDialog::configure()
{
    // Show the dialog and save configuration if accepted

    int retcode = exec();
    if(retcode == Accepted) {

        KConfigGroup config(KGlobal::config(), "Shortcuts");
	#warning All of the stuff related to the group selection needs to be rewritten
        /* config.writeEntry("GlobalKeys", m_group->id(m_group->selected())); */
        KGlobal::config()->sync();

        m_pKeyChooser->save();
    }
    return retcode;
}

void KeyDialog::slotKeys(int group)
{
    #warning bleh
    bool fourModKeys = /* KGlobalAccel::useFourModifierKeys()*/ true;

    // Set modifier keys according to key group and modifier keys

    for(uint i = 0; i < keyInfoCount; i++)
        m_keys->setShortcut(keyInfo[i].action, keyInfo[i].shortcut[group][fourModKeys]);

    // Create a new key chooser to show the keys, and delete the old one

    QWidget *w = m_widgetStack->visibleWidget();
    newDialog(m_keys, m_actionCollection, group);
    m_widgetStack->removeWidget(w);
    delete w;
}

void KeyDialog::slotDefault()
{
    // Select default keys - standard key group

    #warning More of the same
    /* m_group->setButton(StandardKeys); */
    m_pKeyChooser->allDefault();
}

int KeyDialog::configure(KGlobalAccel *keys, KActionCollection *actionCollection,
                         QWidget *parent)
{
    // Create and show dialog - update connections if accepted

    int retcode = KeyDialog(keys, actionCollection, parent).configure();

    if(retcode == Accepted)
        keys->updateConnections();
    return retcode;
}

void KeyDialog::insert(KGlobalAccel *keys, const QString &action, const QString &label,
                       const QObject *objSlot, const char *methodSlot)
{
    KShortcut def3 = KShortcut::null();
    KShortcut def4 = KShortcut::null();

    // Find and insert a standard key

    for(uint i = 0; i < keyInfoCount; i++) {
        if(keyInfo[i].action == action) {
            def3 = keyInfo[i].shortcut[StandardKeys][0];
            def4 = keyInfo[i].shortcut[StandardKeys][1];
            break;
        }
    }
    keys->insert(action, label, QString::null, def3, objSlot, methodSlot);
}

#include "keydialog.moc"

// vim: set et sw=4 tw=0 sta:
