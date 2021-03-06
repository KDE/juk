/**
 * Copyright (C) 2021 Michael Pyne <mpyne@kde.org>
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
 * this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "iconsupport.h"

#include <QIcon>

namespace IconSupport {

QIcon operator ""_icon(const char *latin1Name, std::size_t len)
{
    QString iconName = QString::fromLatin1(latin1Name, len);
    return QIcon::fromTheme(iconName);
}

};

