//
// This file is part of the Marble Desktop Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2006-2007 Torsten Rahn <tackat@kde.org>"
// Copyright 2007      Inge Wallin  <ingwa@kde.org>"
// Copyright 2008      Carlos Licea <carlos.licea@kdemail.net>
//

#include "TextureColorizer.h"

#include <cmath>

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QPainter>

#include "global.h"
#include "GeoSceneDocument.h"
#include "GeoSceneSettings.h"
#include "ViewParams.h"
#include "ViewportParams.h"
#include "AbstractProjection.h"
#include "MathHelper.h"

uint TextureColorizer::texturepalette[16][512];

TextureColorizer::TextureColorizer( const QString& seafile, 
                                    const QString& landfile )
{
   generatePalette(seafile, landfile);
}

QString TextureColorizer::seafile() const
{
    return m_seafile;
}

QString TextureColorizer::landfile() const
{
    return m_landfile;
}

// This function takes two images, both in viewParams:
//  - The coast image, which has a number of colors where each color
//    represents a sort of terrain (ex: land/sea)
//  - The canvas image, which has a gray scale image, often
//    representing a height field.
//
// It then uses the values of the pixels in the coast image to select
// a color map.  The value of the pixel in the canvas image is used as
// an index into the selected color map and the resulting color is
// written back to the canvas image.  This way we can have different
// color schemes for land and water.
//
// In addition to this, a simple form of bump mapping is performed to
// increase the illusion of height differences (see the variable
// showRelief).
// 

void TextureColorizer::colorize(ViewParams *viewParams)
{
    QImage        *origimg  = viewParams->canvasImage();
    const QImage  *coastimg = viewParams->coastImage();
    const qint64   radius   = viewParams->radius();

    const int  imgheight = origimg->height();
    const int  imgwidth  = origimg->width();
    const int  imgrx     = imgwidth / 2;
    const int  imgry     = imgheight / 2;
    // This variable is not used anywhere..
    const int  imgradius = imgrx * imgrx + imgry * imgry;

    const uint landoffscreen = qRgb(255,0,0);
    // const uint seaoffscreen = qRgb(0,0,0);
    const uint lakeoffscreen = qRgb(0,0,0);
    // const uint glaciercolor = qRgb(200,200,200);

    int     bump = 8;
    GpFifo  emboss;
    emboss.buffer = 0;

    bool showRelief;
    viewParams->mapTheme()->settings()->propertyValue( "relief", showRelief );

    if ( radius * radius > imgradius
         || viewParams->projection() == Equirectangular
         || viewParams->projection() == Mercator )
    {
        int yTop = 0;
        int yBottom = imgheight;

        if( viewParams->projection() == Equirectangular
            || viewParams->projection() == Mercator )
        {
            // Calculate translation of center point
            double  centerLon;
            double  centerLat;
            viewParams->centerCoordinates( centerLon, centerLat );

            const float rad2Pixel = (double)( 2 * radius ) / M_PI;
            if ( viewParams->projection() == Equirectangular ) {
                int yCenterOffset = (int)( centerLat * rad2Pixel );
                yTop = ( imgry - radius + yCenterOffset < 0)? 0 : imgry - radius + yCenterOffset;
                yBottom = ( imgry + yCenterOffset + radius > imgheight )? imgheight : imgry + yCenterOffset + radius;
            }
            else if ( viewParams->projection() == Mercator ) {
                int yCenterOffset = (int)( asinh( tan( centerLat ) ) * rad2Pixel  );
                yTop = ( imgry - 2 * radius + yCenterOffset < 0 ) ? 0 : imgry - 2 * radius + yCenterOffset;
                yBottom = ( imgry + 2 * radius + yCenterOffset > imgheight )? imgheight : imgry + 2 * radius + yCenterOffset;
            }
        }

        for (int y = yTop; y < yBottom; ++y) {

            QRgb  *writeData         = (QRgb*)( origimg->scanLine( y ) );
            const QRgb  *coastData   = (QRgb*)( coastimg->scanLine( y ) );

            uchar *readDataStart     = origimg->scanLine( y );
            const uchar *readDataEnd = readDataStart + imgwidth*4;

            emboss.buffer = 0;
		
            for ( uchar* readData = readDataStart; 
		  readData < readDataEnd; 
		  readData += 4, ++writeData, ++coastData )
	    {

                // Cheap Emboss / Bumpmapping
                const uchar&  grey = *readData; // qBlue(*data);

                if ( showRelief ) {
                    emboss.gpuint.x4 = grey;
                    emboss.buffer = emboss.buffer >> 8;
                    bump = ( emboss.gpuint.x1 + 8 - grey );
                    if ( bump  < 0 )  bump = 0;
                    if ( bump  > 15 ) bump = 15;
                }

                int alpha = qRed( *coastData );
                if ( alpha == 255 || alpha == 0 ) {
                    if ( *coastData == landoffscreen )
                        *writeData = texturepalette[bump][grey + 0x100]; 
                    else {
                        if (*coastData == lakeoffscreen)
                            *writeData = texturepalette[bump][0x055];
                        else {
                            *writeData = texturepalette[bump][grey];
                        }
                    }
                }
                else {
                    double c = 1.0 / 255.0;

                    if ( qRed( *coastData ) != 0 && qGreen( *coastData ) == 0) {

                        QRgb landcolor  = (QRgb)(texturepalette[bump][grey + 0x100]);
                        QRgb watercolor = (QRgb)(texturepalette[bump][grey]);

                        *writeData = qRgb( 
                            (int) ( c * ( alpha * qRed( landcolor )
                            + ( 255 - alpha ) * qRed( watercolor ) ) ),
                            (int) ( c * ( alpha * qGreen( landcolor )
                            + ( 255 - alpha ) * qGreen( watercolor ) ) ),
                            (int) ( c * ( alpha * qBlue( landcolor )
                            + ( 255 - alpha ) * qBlue( watercolor ) ) )
                        );
                    }
                    else {

                        if ( qGreen( *coastData ) != 0 ) {

                            QRgb landcolor  = (QRgb)(texturepalette[bump][grey + 0x100]);
                            QRgb glaciercolor = (QRgb)(texturepalette[bump][grey]);
    
                            *writeData = qRgb( 
                                (int) ( c * ( alpha * qRed( glaciercolor )
                                + ( 255 - alpha ) * qRed( landcolor ) ) ),
                                (int) ( c * ( alpha * qGreen( glaciercolor )
                                + ( 255 - alpha ) * qGreen( landcolor ) ) ),
                                (int) ( c * ( alpha * qBlue( glaciercolor )
                                + ( 255 - alpha ) * qBlue( landcolor ) ) )
                            ); 
                        }
                    }
                }
            }
        }
    }
    else {
        int yTop    = ( imgry-radius < 0 ) ? 0 : imgry-radius;
        const int yBottom = ( yTop == 0 ) ? imgheight : imgry + radius;

        for ( int y = yTop; y < yBottom; ++y ) {
            const int  dy = imgry - y;
            int  rx = (int)sqrt( (double)( radius * radius - dy * dy ) );
            int  xLeft  = 0; 
            int  xRight = imgwidth;

            if ( imgrx-rx > 0 ) {
                xLeft  = imgrx - rx; 
                xRight = imgrx + rx;
            }

            QRgb  *writeData         = (QRgb*)( origimg->scanLine( y ) )  + xLeft;
            const QRgb *coastData    = (QRgb*)( coastimg->scanLine( y ) ) + xLeft;

            uchar *readDataStart     = origimg->scanLine( y ) + xLeft * 4;
            const uchar *readDataEnd = origimg->scanLine( y ) + xRight * 4;

 
            for ( uchar* readData = readDataStart;
		  readData < readDataEnd;
		  readData += 4, ++writeData, ++coastData )
	    {
                // Cheap Embosss / Bumpmapping

                const uchar& grey = *readData; // qBlue(*data);

                if ( showRelief == true ) {
                    emboss.buffer = emboss.buffer >> 8;
                    emboss.gpuint.x4 = grey;    
                    bump = ( emboss.gpuint.x1 + 16 - grey ) >> 1;

                    if ( bump > 15 ) bump = 15;
                    if ( bump < 0 )  bump = 0;
                }

                int alpha = qRed( *coastData );
                if ( alpha == 255 || alpha == 0 ) {
                    if ( *coastData == landoffscreen )
                        *writeData = texturepalette[bump][grey + 0x100]; 
                    else {
                        if (*coastData == lakeoffscreen)
                            *writeData = texturepalette[bump][0x055];
                        else {
                            *writeData = texturepalette[bump][grey];
                        }
                    }
                }
                else {
                    double c = 1.0 / 255.0;

                    if ( qRed( *coastData ) != 0 && qGreen( *coastData ) == 0) {

                        QRgb landcolor  = (QRgb)(texturepalette[bump][grey + 0x100]);
                        QRgb watercolor = (QRgb)(texturepalette[bump][grey]);

                        *writeData = qRgb( 
                            (int) ( c * ( alpha * qRed( landcolor )
                            + ( 255 - alpha ) * qRed( watercolor ) ) ),
                            (int) ( c * ( alpha * qGreen( landcolor )
                            + ( 255 - alpha ) * qGreen( watercolor ) ) ),
                            (int) ( c * ( alpha * qBlue( landcolor )
                            + ( 255 - alpha ) * qBlue( watercolor ) ) )
                        );
                    }
                    else {

                        if ( qGreen( *coastData ) != 0 ) {

                            QRgb landcolor  = (QRgb)(texturepalette[bump][grey + 0x100]);
                            QRgb glaciercolor = (QRgb)(texturepalette[bump][grey]);
    
                            *writeData = qRgb( 
                                (int) ( c * ( alpha * qRed( glaciercolor )
                                + ( 255 - alpha ) * qRed( landcolor ) ) ),
                                (int) ( c * ( alpha * qGreen( glaciercolor )
                                + ( 255 - alpha ) * qGreen( landcolor ) ) ),
                                (int) ( c * ( alpha * qBlue( glaciercolor )
                                + ( 255 - alpha ) * qBlue( landcolor ) ) )
                            ); 
                        }
                    }
                }
            }
        }
    }
}

void TextureColorizer::generatePalette(const QString& seafile,
				       const QString& landfile)
{
    for(int i = 0; i < 16; i++) {
        for(int j = 0; j < 512; j++) {
            texturepalette[i][j] = 0;
        }
    }
    QImage   gradientImage ( 256, 10, QImage::Format_RGB32 );
    QPainter  gradientPainter;
    gradientPainter.begin( &gradientImage );
    gradientPainter.setPen( Qt::NoPen );

    QImage    shadingImage ( 256, 10, QImage::Format_RGB32 );
    QPainter  shadingPainter;
    shadingPainter.begin( &shadingImage );
    shadingPainter.setPen( Qt::NoPen );

    int offset = 0;

    QStringList  filelist;
    filelist << seafile << landfile;

    foreach ( const QString &filename, filelist ) {

        QLinearGradient  gradient( 0, 0, 256, 0 );

        QFile  file( filename );
        file.open( QIODevice::ReadOnly );
        QTextStream  stream( &file );  // read the data from the file
        QString      evalstrg;

        while ( !stream.atEnd() ) {
            stream >> evalstrg;
            if ( !evalstrg.isEmpty() && evalstrg.contains( "=" ) ) {
                QString  colorValue = evalstrg.section( "=", 0, 0 );
                QString  colorPosition = evalstrg.section( "=", 1, 1 );
                gradient.setColorAt( colorPosition.toDouble(),
				     QColor( colorValue ) );
            }
        }
        gradientPainter.setBrush( gradient );
        gradientPainter.drawRect( 0, 0, 256, 3 );	

        for ( int j = 0; j < 16; ++j ) {
  
            int  shadeIndex = 120 + j;

            for ( int i = 0; i < 256; ++i ) {

                    QRgb  shadeColor = gradientImage.pixel( i, 1 );
                    QLinearGradient  shadeGradient( 0, 0, 256, 0 );
                    shadeGradient.setColorAt(0.15, QColor(Qt::white));
                    shadeGradient.setColorAt(0.496, shadeColor);
                    shadeGradient.setColorAt(0.504, shadeColor);
                    shadeGradient.setColorAt(0.75, QColor(Qt::black));
                    shadingPainter.setBrush( shadeGradient );
                    shadingPainter.drawRect( 0, 0, 256, 3 );  
                    QRgb  paletteColor = shadingImage.pixel( shadeIndex, 1 );

                    // populate texturepalette[][]
                    texturepalette[j][offset + i] = (uint)paletteColor;
            }
        }
        offset += 256;
    }
    shadingPainter.end();  // Need to explicitely tell painter lifetime to avoid crash
    gradientPainter.end(); // on some systems. 

    m_seafile = seafile;
    m_landfile = landfile;
}
