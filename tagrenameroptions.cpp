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

#include "tagrenameroptions.h"

#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <kconfigbase.h>
#include <kconfiggroup.h>

#include "juk_debug.h"

TagRenamerOptions::TagRenamerOptions() :
    m_emptyAction(IgnoreEmptyTag),
    m_trackWidth(0),
    m_disabled(true),
    m_category(TagUnknown)
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

TagRenamerOptions::TagRenamerOptions(const CategoryID &category)
    : m_category(category.category)
{
    // Set some defaults

    bool disabled;
    unsigned categoryNum = category.categoryNumber;

    switch(category.category) {
    case Title:
    case Artist:
    case Genre:
    case Year:
    case Album:
    case Track:
        disabled = false;
        break;
    default:
        disabled = true;
    }

    // Make sure we don't use translated strings for the config file keys.

    QString typeKey = tagTypeText(category.category, false);
    KConfigGroup config(KSharedConfig::openConfig(), "FileRenamer");

    if(categoryNum > 0)
        typeKey.append(QString::number(categoryNum));

    setSuffix(config.readEntry(QString("%1Suffix").arg(typeKey), QString()));
    setPrefix(config.readEntry(QString("%1Prefix").arg(typeKey), QString()));

    // Default the emptyAction to ignoring the empty tag.

    const QString emptyAction = config.readEntry(QString("%1EmptyAction").arg(typeKey), QString()).toLower();
    setEmptyAction(IgnoreEmptyTag);

    if(emptyAction == "forceemptyinclude")
        setEmptyAction(ForceEmptyInclude);
    else if(emptyAction == "usereplacementvalue")
        setEmptyAction(UseReplacementValue);

    setEmptyText(config.readEntry(QString("%1EmptyText").arg(typeKey), QString()));
    setTrackWidth(config.readEntry(QString("%1TrackWidth").arg(typeKey), 0));
    setDisabled(config.readEntry(QString("%1Disabled").arg(typeKey), disabled));
}

QString TagRenamerOptions::tagTypeText(TagType type, bool translate)
{
    const char *msg = 0, *context = 0;

    switch(type) {
        case Title:
            msg = I18N_NOOP2("song title", "Title");
            context = "song title";
        break;

        case Artist:
            msg = I18N_NOOP("Artist");
        break;

        case Album:
            msg = I18N_NOOP("Album");
        break;

        case Track:
            msg = I18N_NOOP2("cd track number", "Track");
            context = "cd track number";
        break;

        case Genre:
            msg = I18N_NOOP("Genre");
        break;

        case Year:
            msg = I18N_NOOP("Year");
        break;

        default:
            qCWarning(JUK_LOG) << "I don't know what category we're looking up, this is a problem.";
            qCWarning(JUK_LOG) << "The category ID is " << (unsigned) type;
            msg = I18N_NOOP2("unknown renamer category", "Unknown");
            context = "unknown renamer category";
    }

    if(translate)
        return context ? i18nc(context, msg) : i18n(msg);
    else
        return msg;
}

void TagRenamerOptions::saveConfig(unsigned categoryNum) const
{
    // Make sure we don't use translated strings for the config file keys.

    QString typeKey = tagTypeText(false);
    if(categoryNum > 0)
        typeKey.append(QString::number(categoryNum));

    KConfigGroup config(KSharedConfig::openConfig(), "FileRenamer");

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

TagType TagRenamerOptions::tagFromCategoryText(const QString &text)
{
    for(unsigned i = StartTag; i < NumTypes; ++i)
        if(tagTypeText(static_cast<TagType>(i), false) == text)
            return static_cast<TagType>(i);

    return TagUnknown;
}

// vim: set et sw=4 tw=0 sta:
