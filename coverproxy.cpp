/**
 * Copyright (C) 2009 Michael Pyne <mpyne@kde.org>
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

#include "coverproxy.h"
#include "covermanager.h"

#include <kdebug.h>
#include <kio/job.h>

CoverProxy::CoverProxy(QObject *parent) :
    QObject(parent)
{
}

void CoverProxy::handleResult(KJob *job)
{
    if(job->error()) {
        kError() << "Cover download job failed with the following error:" << job->errorString();
        CoverManager::jobComplete(job, false);
    }
    else {
        CoverManager::jobComplete(job, true);
    }
}

#include "coverproxy.moc"
