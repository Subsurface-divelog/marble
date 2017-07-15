//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2011      Daniel Marth <danielmarth@gmx.at>
// Copyright 2012        Bernhard Beschow <bbeschow@cs.tu-berlin.de>
//

#ifndef QT_MOBILITY_POSITION_PROVIDER_PLUGIN_H
#define QT_MOBILITY_POSITION_PROVIDER_PLUGIN_H

#include "PositionProviderPlugin.h"

namespace Marble
{

class QtMobilityPositionProviderPluginPrivate;

class QtMobilityPositionProviderPlugin: public PositionProviderPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA( IID "org.kde.edu.marble.QtMobilityPositionProviderPlugin" )
    Q_INTERFACES( Marble::PositionProviderPluginInterface )

public:
    QtMobilityPositionProviderPlugin();
    virtual ~QtMobilityPositionProviderPlugin();

    // Implementing PluginInterface
    virtual QString name() const;
    virtual QString nameId() const;
    virtual QString guiString() const;
    virtual QString version() const;
    virtual QString description() const;
    virtual QString copyrightYears() const;
    virtual QList<PluginAuthor> pluginAuthors() const;
    virtual QIcon icon() const;
    virtual void initialize();
    virtual bool isInitialized() const;
    virtual qreal speed() const;
    virtual qreal direction() const;
    virtual QDateTime timestamp() const;

    // Implementing PositionProviderPlugin
    virtual PositionProviderPlugin * newInstance() const;

    // Implementing PositionProviderPluginInterface
    virtual PositionProviderStatus status() const;
    virtual GeoDataCoordinates position() const;
    virtual GeoDataAccuracy accuracy() const;

private Q_SLOTS:
    /** Regular (each second) position and status update */
    void update();

private:
    QtMobilityPositionProviderPluginPrivate* const d;

};

}

#endif // QT_POSITION_PROVIDER_PLUGIN_H
