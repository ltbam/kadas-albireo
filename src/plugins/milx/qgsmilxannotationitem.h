/***************************************************************************
 *  qgsmilxannotationitem.h                                                *
 *  -----------------------                                                *
 *  begin                : Oct 01, 2015                                    *
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

#ifndef QGSMILXANNOTATIONITEM_H
#define QGSMILXANNOTATIONITEM_H

#include "qgsannotationitem.h"
#include "MilXClient.hpp"

class QgsCoordinateTransform;
class QgsMilXItem;

class QgsMilXAnnotationItem : public QgsAnnotationItem
{
    Q_OBJECT

    QGS_ANNOTATION_ITEM( QgsMilXAnnotationItem, "MilXAnnotationItem" )

  public:
    QgsMilXAnnotationItem( QgsMapCanvas* canvas );
    ~QgsMilXAnnotationItem();
    QgsMilXAnnotationItem* clone( QgsMapCanvas *canvas ) override { return new QgsMilXAnnotationItem( canvas, this ); }

    void fromMilxItem( QgsMilXItem* item );
    QgsMilXItem* toMilxItem();

    void setSymbolXml( const QString& symbolXml, const QString &symbolMilitaryName );
    void setMapPosition( const QgsPoint &pos, const QgsCoordinateReferenceSystem &crs = QgsCoordinateReferenceSystem() ) override;
    void appendPoint( const QPoint &newPoint );
    void movePoint( int index, const QPoint &newPos );
    void moveAttributePoint( int attr, const QPoint& newPos );
    QList<QPoint> screenPoints() const;
    QList< QPair<int, double> > screenAttributes() const;
    QList< QPair<int, QPoint> > screenAttributePoints() const;
    int absolutePointIdx( int regularIdx ) const;
    void setGraphic( MilXClient::NPointSymbolGraphic& result, bool updatePoints );
    void finalize();

    void paint( QPainter* painter ) override;
    void writeXML( QDomDocument& /*doc*/ ) const override {}
    void readXML( const QDomDocument& /*doc*/, const QDomElement& /*itemElem*/ ) override {}

    int moveActionForPosition( const QPointF& pos ) const override;
    void handleMoveAction( int moveAction, const QPointF &newPos, const QPointF &oldPos ) override;
    Qt::CursorShape cursorShapeForAction( int moveAction ) const override;

    void showContextMenu( const QPoint &screenPos );
    bool hitTest( const QPoint& screenPos ) const override;

    bool isMultiPoint() const { return !mAdditionalPoints.isEmpty() || !mAttributes.isEmpty(); }

  protected:
    QgsMilXAnnotationItem( QgsMapCanvas* canvas, QgsMilXAnnotationItem* source );

  private:
    QString mSymbolXml;
    QString mSymbolMilitaryName;
    QPixmap mGraphic;
    QList<QgsPoint> mAdditionalPoints;
    QPoint mRenderOffset;
    QList<int> mControlPoints;
    QList< QPair<int, double> > mAttributes;
    QList< QPair<int, QgsPoint> > mAttributePoints;
    bool mFinalized;

    void _showItemEditor() override;
    double metersToPixels() const;

  private slots:
    void updateSymbol( bool updatePoints = false );
};

#endif // QGSMILXANNOTATIONITEM_H
