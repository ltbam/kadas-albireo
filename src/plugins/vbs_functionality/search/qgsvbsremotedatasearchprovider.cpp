/***************************************************************************
 *  qgsvbsremotedatasearchprovider.cpp                                     *
 *  -------------------                                                    *
 *  begin                : Jul 09, 2015                                    *
 *  copyright            : (C) 2015 by Sandro Mani / Sourcepole AG         *
 *  email                : smani@sourcepole.ch                             *
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgsvbsremotedatasearchprovider.h"
#include "qgsnetworkaccessmanager.h"
#include "qgscoordinatetransform.h"
#include "qgslogger.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <qjson/parser.h>


const int QgsVBSRemoteDataSearchProvider::sSearchTimeout = 2000;
const int QgsVBSRemoteDataSearchProvider::sResultCountLimit = 100;
const QByteArray QgsVBSRemoteDataSearchProvider::sGeoAdminUrl = "https://api3.geo.admin.ch/rest/services/api/SearchServer";
const QByteArray QgsVBSRemoteDataSearchProvider::sGeoAdminReferrer = "http://localhost";


QgsVBSRemoteDataSearchProvider::QgsVBSRemoteDataSearchProvider( QgisInterface *iface )
    : QgsVBSSearchProvider( iface )
{
  mNetReply = 0;

  mPatBox = QRegExp( "^BOX\\s*\\(\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*,\\s*(\\d+\\.?\\d*)\\s*(\\d+\\.?\\d*)\\s*\\)$" );

  mTimeoutTimer.setSingleShot( true );
  connect( &mTimeoutTimer, SIGNAL( timeout() ), this, SLOT( replyFinished() ) );
}

void QgsVBSRemoteDataSearchProvider::startSearch( const QString &searchtext , const SearchRegion &searchRegion )
{
  QStringList remoteLayers;
#warning TODO

  QUrl url( sGeoAdminUrl );
  url.addQueryItem( "type", "featuresearch" );
  url.addQueryItem( "searchText", searchtext );
  url.addQueryItem( "limit", QString::number( sResultCountLimit ) );
  url.addQueryItem( "features", remoteLayers.join( "," ) );
  if ( !searchRegion.rect.isEmpty() )
  {
    QgsRectangle bbox = QgsCoordinateTransform( searchRegion.crs, QgsCoordinateReferenceSystem( "EPSG:21781" ) ).transform( searchRegion.rect );
    url.addQueryItem( "bbox", QString( "%1,%2,%3,%4" ).arg( bbox.xMinimum() ).arg( bbox.yMinimum() ).arg( bbox.xMaximum() ).arg( bbox.yMaximum() ) );
  }

  QNetworkRequest req( url );
  req.setRawHeader( "Referer", sGeoAdminReferrer );
  mNetReply = QgsNetworkAccessManager::instance()->get( req );
  connect( mNetReply, SIGNAL( finished() ), this, SLOT( replyFinished() ) );
  mTimeoutTimer.start( sSearchTimeout );
}

void QgsVBSRemoteDataSearchProvider::cancelSearch()
{
  if ( mNetReply )
  {
    mTimeoutTimer.stop();
    disconnect( mNetReply, SIGNAL( finished() ), this, SLOT( replyFinished() ) );
    mNetReply->close();
    mNetReply->deleteLater();
    mNetReply = 0;
  }
}

void QgsVBSRemoteDataSearchProvider::replyFinished()
{
  if ( !mNetReply )
    return;

  if ( mNetReply->error() != QNetworkReply::NoError || !mTimeoutTimer.isActive() )
  {
    mNetReply->deleteLater();
    mNetReply = 0;
    emit searchFinished();
    return;
  }

  QByteArray replyText = mNetReply->readAll();
  QJson::Parser parser;
  QVariant result = parser.parse( replyText );
  if ( result.isNull() )
  {
    QgsDebugMsg( QString( "Error at line %1: %2" ).arg( parser.errorLine() ).arg( parser.errorString() ) );
  }
  foreach ( const QVariant& item, result.toMap()["results"].toList() )
  {
    QVariantMap itemMap = item.toMap();
    QVariantMap itemAttrsMap = itemMap["attrs"].toMap();

    if ( !mPatBox.exactMatch( itemAttrsMap["geom_st_box2d"].toString() ) )
    {
      QgsDebugMsg( "Box RegEx did not match " + itemAttrsMap["geom_st_box2d"].toString() );
      continue;
    }

    SearchResult searchResult;
    searchResult.bbox = QgsRectangle( mPatBox.cap( 1 ).toDouble(), mPatBox.cap( 2 ).toDouble(),
                                      mPatBox.cap( 3 ).toDouble(), mPatBox.cap( 4 ).toDouble() );
    // When bbox is empty, fallback to pos + zoomScale is used
    searchResult.pos = QgsPoint( itemAttrsMap["lon"].toDouble(), itemAttrsMap["lat"].toDouble() );
    searchResult.pos = QgsCoordinateTransform( QgsCoordinateReferenceSystem( "EPSG:4326" ), QgsCoordinateReferenceSystem( "EPSG:21781" ) ).transform( searchResult.pos );
    searchResult.zoomScale = 1000;

    searchResult.category = tr( "Feature" );
    searchResult.text = itemAttrsMap["label"].toString();
    searchResult.text.replace( QRegExp( "<[^>]+>" ), "" ); // Remove HTML tags
    searchResult.crs = QgsCoordinateReferenceSystem( "EPSG:21781" );
    emit searchResultFound( searchResult );
  }
  mNetReply->deleteLater();
  mNetReply = 0;
  emit searchFinished();
}