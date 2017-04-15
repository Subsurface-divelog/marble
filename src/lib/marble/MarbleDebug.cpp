//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2009      Patrick Spendrin <ps_ml@gmx.de>
//

#include "MarbleDebug.h"

#include <QFile>
#include <QProcess>

namespace Marble
{
// bool MarbleDebug::m_enabled = true;
bool MarbleDebug::m_enabled = false;

QDebug mDebug()
{
    if ( MarbleDebug::isEnabled() ) {
        return QDebug( QtDebugMsg );
    }
    else {
        static QFile *nullDevice = new QFile(QProcess::nullDevice());
        if ( !nullDevice->isOpen() )
             nullDevice->open(QIODevice::WriteOnly);
         return QDebug( nullDevice );
    }
}

bool MarbleDebug::isEnabled()
{
    return MarbleDebug::m_enabled;
}

void MarbleDebug::setEnabled(bool enabled)
{
    MarbleDebug::m_enabled = enabled;
}

} // namespace Marble
