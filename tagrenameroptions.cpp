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

#include <kdebug.h>
#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <kconfigbase.h>

#include "tagrenameroptions.h"

TagRenamerOptions::TagRenamerOptions() :
    m_emptyAction(IgnoreEmptyTag),
    m_trackWidth(0),
    m_disabled(true),
    m_category(Unknown)
{
}

TagRenamerOptions::TagRenamerOptions(const TagRenamerOptions &other) :
    m_prefix(other.m_prefix),
    m_suffix(other.m_suffix),
    m_emptyAction(other.m_emptyAction),
    m_emptyText(other.m_emptyText),
    m_trackWidth(other.m_trackWidth),
    m_disabled(other.m_disabled),
    m_category(other.m_category)
{
}

TagRenamerOptions::TagRenamerOptions(TagType category)
    : m_category(category)
{
    // Make sure we don't use translated strings for the config file keys.

    const QString typeKey = getTagTypeText(category, false);
    KConfigGroup config(KGlobal::config(), "FileRenamer");

    setSuffix(config.readEntry(QString("%1Suffix").arg(typeKey)));
    setPrefix(config.readEntry(QString("%1Prefix").arg(typeKey)));

    // Default the emptyAction to ignoring the empty tag.

    const QString emptyAction = config.readEntry(QString("%1EmptyAction").arg(typeKey)).lower();
    setEmptyAction(IgnoreEmptyTag);

    if(emptyAction == "forceemptyinclude")
        setEmptyAction(ForceEmptyInclude);
    else if(emptyAction == "usereplacementvalue")
        setEmptyAction(UseReplacementValue);

    setEmptyText(config.readEntry(QString("%1EmptyText").arg(typeKey)));
    setTrackWidth(config.readUnsignedNumEntry(QString("%1TrackWidth").arg(typeKey)));
    setDisabled(config.readBoolEntry(QString("%1Disabled").arg(typeKey)));
}

QString TagRenamerOptions::getTagTypeText(TagType type, bool translate)
{
    // These must be declared in the same order that they are defined in
    // the TagType enum in test.h.  We can dynamically translate these strings,
    // so make sure that I18N_NOOP() is used instead of i18n().

    const char *tags[] = {
        I18N_NOOP("Title"), I18N_NOOP("Artist"), I18N_NOOP("Album"),
        I18N_NOOP("Track"), I18N_NOOP("Genre"), I18N_NOOP("Year")
    };

    if(type < StartTag || type >= NumTypes) {
        kdWarning() << "I don't know what category we're looking up, this is a problem." << endl;
        kdWarning() << "The category ID is " << (unsigned) type << endl;
        return translate ? i18n("Unknown") : "Unknown";
    }

    return translate ? i18n(tags[type]) : tags[type];
}

void TagRenamerOptions::saveConfig() const
{
    // Make sure we don't use translated strings for the config file keys.

    const QString typeKey = getTagTypeText(false);
    KConfigGroup config(KGlobal::config(), "FileRenamer");

    config.writeEntry(QString("%1Suffix").arg(typeKey), suffix());
    config.writeEntry(QString("%1Prefix").arg(typeKey), prefix());

    QString emptyStr;

    switch(emptyAction()) {
    case ForceEmptyInclude:
        emptyStr = "ForceEmptyInclude";
    break;

    case IgnoreEmptyTag:
        emptyStr = "IgnoreEmptyTag";
    break;

    case UseReplacementValue:
        emptyStr = "UseReplacementValue";
    break;
    }

    config.writeEntry(QString("%1EmptyAction").arg(typeKey), emptyStr);
    config.writeEntry(QString("%1EmptyText").arg(typeKey), emptyText());
    config.writeEntry(QString("%1Disabled").arg(typeKey), disabled());

    if(category() == Track)
        config.writeEntry(QString("%1TrackWidth").arg(typeKey), trackWidth());

    config.sync();
}

TagRenamerOptions &TagRenamerOptions::operator=(const TagRenamerOptions &other)
{
    if(this == &other)
        return *this;

    setPrefix(other.prefix());
    setSuffix(other.suffix());
    setEmptyText(other.emptyText());
    setEmptyAction(other.emptyAction());
    setTrackWidth(other.trackWidth());
    setDisabled(other.disabled());
    setCategory(other.category());

    return *this;
}

// vim: set et ts=4 sw=4:
