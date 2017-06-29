//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2012   Mohammed Nafees   <nafees.technocool@gmail.com>
// Copyright 2012   Dennis Nienhüser  <nienhueser@kde.org>
// Copyright 2012   Illya Kovalevskyy <illya.kovalevskyy@gmail.com>
// Copyright 2015   Imran Tatriev     <itatriev@gmail.com>
//

#include "PopupLayer.h"

#include "GeoDataCoordinates.h"
#include "GeoPainter.h"
#include "MarbleWidget.h"
#ifndef SUBSURFACE
#include "PopupItem.h"
#endif
#include "ViewportParams.h"
#include "RenderPlugin.h"
#include "RenderState.h"

#include <QSizeF>

namespace Marble
{

class Q_DECL_HIDDEN PopupLayer::Private
{
public:
    Private( MarbleWidget *marbleWidget, PopupLayer *q );

    /**
     * @brief Sets size of the popup item, based on the requested size and viewport size
     * @param viewport required to compute the maximum dimensions
     */
    void setAppropriateSize( const ViewportParams *viewport );

    static QString filterEmptyShortDescription(const QString &description);
    void setupDialogSatellite( const GeoDataPlacemark *index );
    void setupDialogCity( const GeoDataPlacemark *index );
    void setupDialogNation( const GeoDataPlacemark *index );
    void setupDialogGeoPlaces( const GeoDataPlacemark *index );
    void setupDialogSkyPlaces( const GeoDataPlacemark *index );

#ifndef SUBSURFACE
    PopupItem *const m_popupItem;
#endif
    MarbleWidget *const m_widget;
    QSizeF m_requestedSize;
    bool m_hasCrosshairsPlugin;
    bool m_crosshairsVisible;
};

PopupLayer::Private::Private( MarbleWidget *marbleWidget, PopupLayer *q ) :
#ifndef SUBSURFACE
    m_popupItem( new PopupItem( q ) ),
    m_hasCrosshairsPlugin( false ),
    m_crosshairsVisible( true )
#endif
    m_widget( marbleWidget)
{
}

PopupLayer::PopupLayer( MarbleWidget *marbleWidget, QObject *parent ) :
    QObject( parent ),
    d( new Private( marbleWidget, this ) )
{
#ifndef SUBSURFACE
    for (const RenderPlugin *renderPlugin: d->m_widget->renderPlugins()) {
        if (renderPlugin->nameId() == QLatin1String("crosshairs")) {
            d->m_hasCrosshairsPlugin = true;
            break;
        }
    }

    connect( d->m_popupItem, SIGNAL(repaintNeeded()), this, SIGNAL(repaintNeeded()) );
    connect( d->m_popupItem, SIGNAL(hide()), this, SLOT(hidePopupItem()) );
#endif
}

PopupLayer::~PopupLayer()
{
    delete d;
}

QStringList PopupLayer::renderPosition() const
{
    return QStringList(QStringLiteral("ALWAYS_ON_TOP"));
}

bool PopupLayer::render( GeoPainter *painter, ViewportParams *viewport,
                                const QString&, GeoSceneLayer* )
{
    if ( visible() ) {
        d->setAppropriateSize( viewport );
#ifndef SUBSURFACE
        d->m_popupItem->paintEvent( painter, viewport );
#endif
    }

    return true;
}

bool PopupLayer::eventFilter( QObject *object, QEvent *e )
{
#ifndef SUBSURFACE
    return visible() && d->m_popupItem->eventFilter( object, e );
#else
    return false;
#endif
}

qreal PopupLayer::zValue() const
{
    return 4711.23;
}

RenderState PopupLayer::renderState() const
{
    return RenderState(QStringLiteral("Popup Window"));
}

bool PopupLayer::visible() const
{
#ifndef SUBSURFACE
    return d->m_popupItem->visible();
#else
    return false;
#endif
}

void PopupLayer::setVisible( bool visible )
{
#ifndef SUBSURFACE
    d->m_popupItem->setVisible( visible );
    if ( !visible ) {
        disconnect( d->m_popupItem, SIGNAL(repaintNeeded()), this, SIGNAL(repaintNeeded()) );
        d->m_popupItem->clearHistory();
        emit repaintNeeded();
    }
    else {
        connect( d->m_popupItem, SIGNAL(repaintNeeded()), this, SIGNAL(repaintNeeded()) );
    }
#endif
}

void PopupLayer::popup()
{
#ifndef SUBSURFACE
    GeoDataCoordinates coords = d->m_popupItem->coordinate();
    ViewportParams viewport( d->m_widget->viewport()->projection(),
                             coords.longitude(), coords.latitude(), d->m_widget->viewport()->radius(),
                             d->m_widget->viewport()->size() );
    qreal sx, sy, lon, lat;
    viewport.screenCoordinates( coords, sx, sy );
    sx = viewport.radius() < viewport.width() ? 0.5 * (viewport.width() + viewport.radius()) : 0.75 * viewport.width();
    viewport.geoCoordinates( sx, sy, lon, lat, GeoDataCoordinates::Radian );
    coords.setLatitude( lat );
    coords.setLongitude( lon );
    d->m_widget->centerOn( coords, true );

    if( d->m_hasCrosshairsPlugin ) {
        d->m_crosshairsVisible = d->m_widget->showCrosshairs();

        if( d->m_crosshairsVisible ) {
            d->m_widget->setShowCrosshairs( false );
        }
    }

    setVisible( true );
#endif
}

void PopupLayer::setCoordinates( const GeoDataCoordinates &coordinates , Qt::Alignment alignment )
{
#ifndef SUBSURFACE
    d->m_popupItem->setCoordinate( coordinates );
    d->m_popupItem->setAlignment( alignment );
#endif
}

void PopupLayer::setUrl( const QUrl &url )
{
#ifndef SUBSURFACE
    d->m_popupItem->setUrl( url );
#endif
}

void PopupLayer::setContent( const QString &html, const QUrl &baseUrl )
{
#ifndef SUBSURFACE
    d->m_popupItem->setContent( html, baseUrl );
#endif
}

void PopupLayer::setBackgroundColor(const QColor &color)
{
#ifndef SUBSURFACE
    if(color.isValid()) {
        d->m_popupItem->setBackgroundColor(color);
    }
#endif
}

void PopupLayer::setTextColor(const QColor &color)
{
#ifndef SUBSURFACE
    if(color.isValid()) {
        d->m_popupItem->setTextColor(color);
    }
#endif
}

void PopupLayer::setSize( const QSizeF &size )
{
    d->m_requestedSize = size;
}

void PopupLayer::Private::setAppropriateSize( const ViewportParams *viewport )
{
#ifndef SUBSURFACE
    qreal margin = 15.0;

    QSizeF maximumSize;
    maximumSize.setWidth( viewport->width() - margin );
    maximumSize.setHeight( viewport->height() - margin );

    QSizeF minimumSize( 100.0, 100.0 );

    m_popupItem->setSize( m_requestedSize.boundedTo( maximumSize ).expandedTo( minimumSize ) );
#endif
}

void PopupLayer::hidePopupItem()
{
    if( d->m_hasCrosshairsPlugin && d->m_crosshairsVisible ) {
        d->m_widget->setShowCrosshairs( d->m_crosshairsVisible );
    }

    setVisible( false );
}

}

#include "moc_PopupLayer.cpp"
