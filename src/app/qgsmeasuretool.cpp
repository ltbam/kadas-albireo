/***************************************************************************
    qgsmeasuretool.cpp  -  map tool for measuring distances and areas
    ---------------------
    begin                : April 2007
    copyright            : (C) 2007 by Martin Dobias
    email                : wonder.sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgscursors.h"
#include "qgsfeaturepicker.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmeasuredialog.h"
#include "qgsmeasuretool.h"
#include "qgsrubberband.h"
#include "qgssnappingutils.h"
#include "qgsvectorlayer.h"

#include <QMessageBox>
#include <QMouseEvent>
#include <QSettings>


QgsMeasureTool::QgsMeasureTool( QgsMapCanvas* canvas, bool measureArea )
    : QgsMapTool( canvas )
    , mCurrentPart( -1 )
    , mWrongProjectProjection( false )
    , mPickFeature( false )
{
  mMeasureArea = measureArea;

  mRubberBand = new QgsRubberBand( canvas, mMeasureArea ? QGis::Polygon : QGis::Line );
  mRubberBandPoints = new QgsRubberBand( canvas, QGis::Point );

  mDialog = new QgsMeasureDialog( this, Qt::WindowStaysOnTopHint );

  connect( canvas, SIGNAL( destinationCrsChanged() ), this, SLOT( updateSettings() ) );
  connect( canvas, SIGNAL( scaleChanged( double ) ), this, SLOT( updateLabelPositions() ) );
}

QgsMeasureTool::~QgsMeasureTool()
{
  qDeleteAll( mTextLabels );
  delete mRubberBand;
  delete mRubberBandPoints;
}

void QgsMeasureTool::activate()
{
  mDialog->show();
  QgsMapTool::activate();

  restart();

  // If we suspect that they have data that is projected, yet the
  // map CRS is set to a geographic one, warn them.
  if ( mCanvas->mapSettings().destinationCrs().geographicFlag() &&
       ( mCanvas->extent().height() > 360 ||
         mCanvas->extent().width() > 720 ) )
  {
    QMessageBox::warning( NULL, tr( "Incorrect measure results" ),
                          tr( "<p>This map is defined with a geographic coordinate system "
                              "(latitude/longitude) "
                              "but the map extents suggests that it is actually a projected "
                              "coordinate system (e.g., Mercator). "
                              "If so, the results from line or area measurements will be "
                              "incorrect.</p>"
                              "<p>To fix this, explicitly set an appropriate map coordinate "
                              "system using the <tt>Settings:Project Properties</tt> menu." ) );
    mWrongProjectProjection = true;
  }
}

void QgsMeasureTool::deactivate()
{
  restart();
  mDialog->close();
  QgsMapTool::deactivate();
}

void QgsMeasureTool::restart()
{
  qDeleteAll( mTextLabels );
  mTextLabels.clear();

  mCurrentPart = -1;

  mRubberBand->reset( mMeasureArea ? QGis::Polygon : QGis::Line );
  mRubberBandPoints->reset( QGis::Point );

  // re-read settings
  updateSettings();

  mWrongProjectProjection = false;
  mPickFeature = false;
  setCursor( QCursor( QPixmap(( const char ** ) cross_hair_cursor ), 8, 8 ) );
}

void QgsMeasureTool::updateLabels()
{
  for ( int i = 0, n = mTextLabels.size(); i < n; ++i )
  {
    updateLabel( i );
  }
}

void QgsMeasureTool::updateLabel( int idx )
{
  mTextLabels[idx]->setHtml( QString( "<div style=\"background: rgba(255, 255, 255, 159); padding: 5px; border-radius: 5px;\">%1</div>" ).arg( mDialog->getPartMeasurement( idx ) ) );
}

void QgsMeasureTool::updateSettings()
{
  QSettings settings;

  int myRed = settings.value( "/qgis/default_measure_color_red", 222 ).toInt();
  int myGreen = settings.value( "/qgis/default_measure_color_green", 155 ).toInt();
  int myBlue = settings.value( "/qgis/default_measure_color_blue", 67 ).toInt();
  mRubberBand->setBorderColor( QColor( myRed, myGreen, myBlue ) );
  mRubberBand->setFillColor( QColor( myRed, myGreen, myBlue, 127 ) );
  mRubberBand->setWidth( 3 );
  mRubberBandPoints->setIcon( QgsRubberBand::ICON_CIRCLE );
  mRubberBandPoints->setIconSize( 10 );
  mRubberBandPoints->setFillColor( Qt::white );
  mRubberBandPoints->setBorderColor( QColor( myRed, myGreen, myBlue ) );
  mRubberBandPoints->setWidth( 2 );
  mDialog->updateSettings();
}

void QgsMeasureTool::pickGeometry()
{
  if ( mCurrentPart >= 0 && mRubberBand->getPoints().size() > mCurrentPart )
  {
    mRubberBand->removeLastPart( true );
  }
  else if ( mCurrentPart == -1 )
  {
    mCurrentPart = 0;
  }
  mPickFeature = true;
  setCursor( QCursor( Qt::ArrowCursor ) );
}

void QgsMeasureTool::addGeometry( QgsGeometry *geometry, QgsVectorLayer* layer )
{
  if ( mCurrentPart == -1 )
    mCurrentPart = 0;
  mRubberBand->addGeometry( geometry, layer );
  mRubberBandPoints->addGeometry( geometry, layer );
  int idx = mCurrentPart;
  while ( mCurrentPart < mRubberBand->getPoints().size() )
  {
    addPart( toCanvasCoordinates( mRubberBand->partMidpoint( mCurrentPart ) ) );
    mDialog->updateMeasurements();
    ++mCurrentPart;
  }
  while ( idx < mCurrentPart )
  {
    updateLabel( idx++ );
  }
}

//////////////////////////

void QgsMeasureTool::canvasMoveEvent( QMouseEvent * e )
{
  if ( mCurrentPart >= 0 && mRubberBand->getPoints().size() > mCurrentPart )
  {
    QgsPoint point = snapPoint( e->pos() );

    mRubberBand->movePoint( point, mCurrentPart );
    mRubberBandPoints->movePoint( point, mCurrentPart );
    mDialog->updateMeasurements();
    mTextLabels.back()->setPos( toCanvasCoordinates( mRubberBand->partMidpoint( mCurrentPart ) ) );
    updateLabel( mCurrentPart );
  }
}

void QgsMeasureTool::canvasReleaseEvent( QMouseEvent * e )
{
  if ( mPickFeature )
  {
    QgsFeaturePicker::PickResult pickResult = QgsFeaturePicker::pick( mCanvas, toMapCoordinates( e->pos() ), mMeasureArea ? QGis::Polygon : QGis::Line );
    if ( pickResult.feature.isValid() )
    {
      addGeometry( pickResult.feature.geometry(), static_cast<QgsVectorLayer*>( pickResult.layer ) );
    }
    mPickFeature = false;
    setCursor( QCursor( QPixmap(( const char ** ) cross_hair_cursor ), 8, 8 ) );
  }
  else
  {
    QgsPoint point = snapPoint( e->pos() );

    if ( mCurrentPart < 0 )
    {
      mCurrentPart = 0;
      addPoint( point );
    }
    else
    {
      addPoint( point );

      if ( e->button() == Qt::RightButton )
      {
        ++mCurrentPart;
      }
    }
  }
}

void QgsMeasureTool::undo()
{
  if ( mRubberBand )
  {
    if ( mCurrentPart == 0 && mRubberBandPoints->partSize( mCurrentPart ) <= 1 )
    {
      //removing first point, so restart everything
      mDialog->restart();
    }
    else if ( mRubberBandPoints->partSize( mCurrentPart ) >= 1 )
    {
      //remove second last point from line band, and last point from points band
      mRubberBand->removePoint( -2, true, mCurrentPart );
      mRubberBandPoints->removePoint( -1, true, mCurrentPart );
      mDialog->removePoint();
      updateLabel( mCurrentPart );
    }
  }
}

void QgsMeasureTool::keyPressEvent( QKeyEvent* e )
{
  if (( e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete ) )
  {
    undo();

    // Override default shortcut management in MapCanvas
    e->ignore();
  }
}


void QgsMeasureTool::addPoint( const QgsPoint &point )
{
  if ( mRubberBand->getPoints().size() <= mCurrentPart )
  {
    addPart( toCanvasCoordinates( point ) );
  }

  // Append point that we will be moving.
  mRubberBand->addPoint( point, true, mCurrentPart );
  mRubberBandPoints->addPoint( point, true, mCurrentPart );
  mDialog->updateMeasurements();
}

void QgsMeasureTool::addPart( const QPoint &labelPos )
{
  mDialog->addPart();
  mTextLabels.append( new QGraphicsTextItem( "", 0, mCanvas->scene() ) );
  int red = QSettings().value( "/qgis/default_measure_color_red", 222 ).toInt();
  int green = QSettings().value( "/qgis/default_measure_color_green", 155 ).toInt();
  int blue = QSettings().value( "/qgis/default_measure_color_blue", 67 ).toInt();
  mTextLabels.back()->setDefaultTextColor( QColor( red, green, blue ) );
  QFont font = mTextLabels.back()->font();
  font.setBold( true );
  mTextLabels.back()->setFont( font );
  mTextLabels.back()->setPos( labelPos );
}

const QList< QList<QgsPoint> >& QgsMeasureTool::getPoints() const
{
  return mRubberBand->getPoints();
}

QgsPoint QgsMeasureTool::snapPoint( const QPoint& p )
{
  QgsPointLocator::Match m = mCanvas->snappingUtils()->snapToMap( p );
  return m.isValid() ? m.point() : mCanvas->getCoordinateTransform()->toMapCoordinates( p );
}

void QgsMeasureTool::updateLabelPositions()
{
  for ( int i = 0, n = mTextLabels.size(); i < n; ++i )
  {
    mTextLabels[i]->setPos( toCanvasCoordinates( mRubberBand->partMidpoint( i ) ) );
  }
}
