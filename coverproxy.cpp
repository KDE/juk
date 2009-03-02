/***************************************************************************
    begin                : Sun Mar 01 2009
    copyright            : (C) 2009 by Michael Pyne
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
