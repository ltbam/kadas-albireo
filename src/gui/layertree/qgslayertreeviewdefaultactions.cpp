/***************************************************************************
  qgslayertreeviewdefaultactions.cpp
  --------------------------------------
  Date                 : May 2014
  Copyright            : (C) 2014 by Martin Dobias
  Email                : wonder dot sk at gmail dot com
 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "qgslayertreeviewdefaultactions.h"

#include "qgsapplication.h"
#include "qgslayertree.h"
#include "qgslayertreemodel.h"
#include "qgslayertreeview.h"
#include "qgsmapcanvas.h"
#include "qgsmaplayerregistry.h"
#include "qgsproject.h"
#include "qgsrasterlayer.h"
#include "qgsvectorlayer.h"

#include <QAction>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QWidgetAction>

QgsLayerTreeViewDefaultActions::QgsLayerTreeViewDefaultActions( QgsLayerTreeView* view )
    : QObject( view )
    , mView( view )
{
}

QAction* QgsLayerTreeViewDefaultActions::actionAddGroup( QObject* parent )
{
  QAction* a = new QAction( QgsApplication::getThemeIcon( "/mActionFolder.png" ), tr( "&Add Group" ), parent );
  connect( a, SIGNAL( triggered() ), this, SLOT( addGroup() ) );
  return a;
}

QAction* QgsLayerTreeViewDefaultActions::actionRemoveGroupOrLayer( QObject* parent )
{
  QAction* a = new QAction( QgsApplication::getThemeIcon( "/mActionRemoveLayer.svg" ), tr( "&Remove" ), parent );
  connect( a, SIGNAL( triggered() ), this, SLOT( removeGroupOrLayer() ) );
  return a;
}

QAction* QgsLayerTreeViewDefaultActions::actionShowInOverview( QObject* parent )
{
  QgsLayerTreeNode* node = mView->currentNode();
  if ( !node )
    return 0;

  QAction* a = new QAction( tr( "&Show in overview" ), parent );
  connect( a, SIGNAL( triggered() ), this, SLOT( showInOverview() ) );
  a->setCheckable( true );
  a->setChecked( node->customProperty( "overview", 0 ).toInt() );
  return a;
}

QAction* QgsLayerTreeViewDefaultActions::actionRenameGroupOrLayer( QObject* parent )
{
  QAction* a = new QAction( QgsApplication::getThemeIcon( "/mActionRename.png" ), tr( "Re&name" ), parent );
  connect( a, SIGNAL( triggered() ), this, SLOT( renameGroupOrLayer() ) );
  return a;
}

QAction* QgsLayerTreeViewDefaultActions::actionShowFeatureCount( QObject* parent )
{
  QgsLayerTreeNode* node = mView->currentNode();
  if ( !node )
    return 0;

  QAction* a = new QAction( tr( "Show Feature Count" ), parent );
  connect( a, SIGNAL( triggered() ), this, SLOT( showFeatureCount() ) );
  a->setCheckable( true );
  a->setChecked( node->customProperty( "showFeatureCount", 0 ).toInt() );
  return a;
}

QAction* QgsLayerTreeViewDefaultActions::actionZoomToLayer( QgsMapCanvas* canvas, QObject* parent )
{
  QAction* a = new QAction( QgsApplication::getThemeIcon( "/mActionZoomToLayer.svg" ),
                            tr( "&Zoom to Layer" ), parent );
  a->setData( QVariant::fromValue( reinterpret_cast<void*>( canvas ) ) );
  connect( a, SIGNAL( triggered() ), this, SLOT( zoomToLayer() ) );
  return a;
}

QAction* QgsLayerTreeViewDefaultActions::actionZoomToGroup( QgsMapCanvas* canvas, QObject* parent )
{
  QAction* a = new QAction( QgsApplication::getThemeIcon( "/mActionZoomToLayer.svg" ),
                            tr( "&Zoom to Group" ), parent );
  a->setData( QVariant::fromValue( reinterpret_cast<void*>( canvas ) ) );
  connect( a, SIGNAL( triggered() ), this, SLOT( zoomToGroup() ) );
  return a;
}

QAction* QgsLayerTreeViewDefaultActions::actionTransparency( QgsMapCanvas* canvas, QObject* parent )
{
  QgsMapLayer* layer = mView->currentLayer();
  if ( !layer )
    return 0;

  int curValue = 0;
  if ( layer->type() == QgsMapLayer::VectorLayer || layer->type() == QgsMapLayer::RedliningLayer )
  {
    curValue = static_cast<QgsVectorLayer*>( layer )->layerTransparency();
  }
  else if ( layer->type() == QgsMapLayer::RasterLayer )
  {
    curValue = 100 - static_cast<QgsRasterLayer*>( layer )->renderer()->opacity() * 100;
  }

  QWidget* transpWidget = new QWidget();
  QHBoxLayout* transpLayout = new QHBoxLayout( transpWidget );

  QLabel* transpLabel = new QLabel( tr( "Transparency:" ) );
  transpLabel->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Fixed );
  transpLayout->addWidget( transpLabel );

  QSlider* transpSlider = new QSlider( Qt::Horizontal );
  transpSlider->setRange( 0, 100 );
  transpSlider->setValue( curValue );
  transpSlider->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
  transpSlider->setProperty( "mapcanvas", QVariant::fromValue( reinterpret_cast<void*>( canvas ) ) );
  transpSlider->setTracking( false );
  connect( transpSlider, SIGNAL( valueChanged( int ) ), this, SLOT( setLayerTransparency() ) );
  transpLayout->addWidget( transpSlider );

  QWidgetAction* transpAction = new QWidgetAction( parent );
  transpAction->setDefaultWidget( transpWidget );
  return transpAction;
}

QAction* QgsLayerTreeViewDefaultActions::actionUseAsHightMap( QObject *parent )
{
  QgsMapLayer* layer = mView->currentLayer();
  if ( !layer )
    return 0;
  QAction* heightmapAction = new QAction( tr( "Use as heightmap" ), parent );
  heightmapAction->setCheckable( true );
  QString currentHeightmap = QgsProject::instance()->readEntry( "Heightmap", "layer" );
  heightmapAction->setChecked( currentHeightmap == layer->id() );
  heightmapAction->setProperty( "layerid", layer->id() );
  connect( heightmapAction, SIGNAL( toggled( bool ) ), this, SLOT( setHeightMapLayer( bool ) ) );
  return heightmapAction;
}

QAction* QgsLayerTreeViewDefaultActions::actionShowLayerInfo( QObject *parent )
{
  QgsMapLayer* layer = mView->currentLayer();
  if ( !layer )
    return 0;
  QAction* layerInfoAction = new QAction( QgsApplication::getThemeIcon( "/mActionInfo.svg" ), tr( "Show layer info" ), parent );
  layerInfoAction->setProperty( "layerid", layer->id() );
  connect( layerInfoAction, SIGNAL( triggered() ), this, SLOT( showLayerInfo( ) ) );
  return layerInfoAction;
}

QAction* QgsLayerTreeViewDefaultActions::actionMakeTopLevel( QObject* parent )
{
  QAction* a = new QAction( tr( "&Move to Top-level" ), parent );
  connect( a, SIGNAL( triggered() ), this, SLOT( makeTopLevel() ) );
  return a;
}

QAction* QgsLayerTreeViewDefaultActions::actionGroupSelected( QObject* parent )
{
  QAction* a = new QAction( tr( "&Group Selected" ), parent );
  connect( a, SIGNAL( triggered() ), this, SLOT( groupSelected() ) );
  return a;
}

QAction* QgsLayerTreeViewDefaultActions::actionMutuallyExclusiveGroup( QObject* parent )
{
  QgsLayerTreeNode* node = mView->currentNode();
  if ( !node || !QgsLayerTree::isGroup( node ) )
    return 0;

  QAction* a = new QAction( tr( "Mutually Exclusive Group" ), parent );
  a->setCheckable( true );
  a->setChecked( QgsLayerTree::toGroup( node )->isMutuallyExclusive() );
  connect( a, SIGNAL( triggered() ), this, SLOT( mutuallyExclusiveGroup() ) );
  return a;
}

void QgsLayerTreeViewDefaultActions::addGroup()
{
  QgsLayerTreeGroup* group = mView->currentGroupNode();
  if ( !group )
    group = mView->layerTreeModel()->rootGroup();

  QgsLayerTreeGroup* newGroup = group->addGroup( uniqueGroupName( group ) );
  mView->edit( mView->layerTreeModel()->node2index( newGroup ) );
}

void QgsLayerTreeViewDefaultActions::removeGroupOrLayer()
{
  foreach ( QgsLayerTreeNode* node, mView->selectedNodes( true ) )
  {
    // could be more efficient if working directly with ranges instead of individual nodes
    qobject_cast<QgsLayerTreeGroup*>( node->parent() )->removeChildNode( node );
  }
}

void QgsLayerTreeViewDefaultActions::renameGroupOrLayer()
{
  mView->edit( mView->currentIndex() );
}

void QgsLayerTreeViewDefaultActions::showInOverview()
{
  QgsLayerTreeNode* node = mView->currentNode();
  if ( !node )
    return;

  node->setCustomProperty( "overview", node->customProperty( "overview", 0 ).toInt() ? 0 : 1 );
}

void QgsLayerTreeViewDefaultActions::showFeatureCount()
{
  QgsLayerTreeNode* node = mView->currentNode();
  if ( !QgsLayerTree::isLayer( node ) )
    return;


  node->setCustomProperty( "showFeatureCount", node->customProperty( "showFeatureCount", 0 ).toInt() ? 0 : 1 );
}


void QgsLayerTreeViewDefaultActions::zoomToLayer( QgsMapCanvas* canvas )
{
  QgsMapLayer* layer = mView->currentLayer();
  if ( !layer )
    return;

  QList<QgsMapLayer*> layers;
  layers << layer;
  zoomToLayers( canvas, layers );
}

void QgsLayerTreeViewDefaultActions::zoomToGroup( QgsMapCanvas* canvas )
{
  QgsLayerTreeGroup* groupNode = mView->currentGroupNode();
  if ( !groupNode )
    return;

  QList<QgsMapLayer*> layers;
  foreach ( QString layerId, groupNode->findLayerIds() )
    layers << QgsMapLayerRegistry::instance()->mapLayer( layerId );

  zoomToLayers( canvas, layers );
}

void QgsLayerTreeViewDefaultActions::zoomToLayer()
{
  QAction* s = qobject_cast<QAction*>( sender() );
  QgsMapCanvas* canvas = reinterpret_cast<QgsMapCanvas*>( s->data().value<void*>() );
  zoomToLayer( canvas );
}

void QgsLayerTreeViewDefaultActions::zoomToGroup()
{
  QAction* s = qobject_cast<QAction*>( sender() );
  QgsMapCanvas* canvas = reinterpret_cast<QgsMapCanvas*>( s->data().value<void*>() );
  zoomToGroup( canvas );
}

void QgsLayerTreeViewDefaultActions::setLayerTransparency()
{
  QSlider* slider = qobject_cast<QSlider*>( QObject::sender() );
  int value = slider->value();
  QgsMapCanvas* canvas = reinterpret_cast<QgsMapCanvas*>( slider->property( "mapcanvas" ).value<void*>() );
  QgsMapLayer* layer = mView->currentLayer();
  if ( !layer )
    return;

  if ( layer->type() == QgsMapLayer::VectorLayer || layer->type() == QgsMapLayer::RedliningLayer )
  {
    static_cast<QgsVectorLayer*>( layer )->setLayerTransparency( value );
    mView->refreshLayerSymbology( layer->id() );
    layer->triggerRepaint();
  }
  else if ( layer->type() == QgsMapLayer::RasterLayer )
  {
    static_cast<QgsRasterLayer*>( layer )->renderer()->setOpacity( 1. - value / 100. );
    mView->refreshLayerSymbology( layer->id() );
    layer->triggerRepaint();
  }
}

void QgsLayerTreeViewDefaultActions::setHeightMapLayer( bool active )
{
  QgsProject::instance()->writeEntry( "Heightmap", "layer", active ? QObject::sender()->property( "layerid" ).toString() : "" );
}

void QgsLayerTreeViewDefaultActions::showLayerInfo()
{
  QString layerId = QObject::sender()->property( "layerid" ).toString();
  QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( layerId );
  if ( layer && !layer->infoUrl().isEmpty() )
    QDesktopServices::openUrl( layer->infoUrl() );
}

void QgsLayerTreeViewDefaultActions::zoomToLayers( QgsMapCanvas* canvas, const QList<QgsMapLayer*>& layers )
{
  QgsRectangle extent;
  extent.setMinimal();

  for ( int i = 0; i < layers.size(); ++i )
  {
    QgsMapLayer* layer = layers.at( i );
    QgsRectangle layerExtent = layer->extent();

    QgsVectorLayer *vLayer = qobject_cast<QgsVectorLayer*>( layer );
    if ( vLayer )
    {
      if ( vLayer->geometryType() == QGis::NoGeometry )
        continue;

      if ( layerExtent.isEmpty() )
      {
        vLayer->updateExtents();
        layerExtent = vLayer->extent();
      }
    }

    if ( layerExtent.isNull() )
      continue;

    //transform extent if otf-projection is on
    if ( canvas->hasCrsTransformEnabled() )
      layerExtent = canvas->mapSettings().layerExtentToOutputExtent( layer, layerExtent );

    extent.combineExtentWith( &layerExtent );
  }

  if ( extent.isNull() )
    return;

  // Increase bounding box with 5%, so that layer is a bit inside the borders
  extent.scale( 1.05 );

  //zoom to bounding box
  canvas->setExtent( extent );
  canvas->refresh();
}


QString QgsLayerTreeViewDefaultActions::uniqueGroupName( QgsLayerTreeGroup* parentGroup )
{
  QString prefix = parentGroup == mView->layerTreeModel()->rootGroup() ? "group" : "sub-group";
  QString newName = prefix + "1";
  for ( int i = 2; parentGroup->findGroup( newName ); ++i )
    newName = prefix + QString::number( i );
  return newName;
}


void QgsLayerTreeViewDefaultActions::makeTopLevel()
{
  QgsLayerTreeNode* node = mView->currentNode();
  if ( !node )
    return;

  QgsLayerTreeGroup* rootGroup = mView->layerTreeModel()->rootGroup();
  QgsLayerTreeGroup* parentGroup = qobject_cast<QgsLayerTreeGroup*>( node->parent() );
  if ( !parentGroup || parentGroup == rootGroup )
    return;

  QgsLayerTreeNode* clonedNode = node->clone();
  rootGroup->addChildNode( clonedNode );
  parentGroup->removeChildNode( node );
}


void QgsLayerTreeViewDefaultActions::groupSelected()
{
  QList<QgsLayerTreeNode*> nodes = mView->selectedNodes( true );
  if ( nodes.count() < 2 || ! QgsLayerTree::isGroup( nodes[0]->parent() ) )
    return;

  QgsLayerTreeGroup* parentGroup = QgsLayerTree::toGroup( nodes[0]->parent() );
  int insertIdx = parentGroup->children().indexOf( nodes[0] );

  QgsLayerTreeGroup* newGroup = new QgsLayerTreeGroup( uniqueGroupName( parentGroup ) );
  foreach ( QgsLayerTreeNode* node, nodes )
    newGroup->addChildNode( node->clone() );

  parentGroup->insertChildNode( insertIdx, newGroup );

  foreach ( QgsLayerTreeNode* node, nodes )
  {
    QgsLayerTreeGroup* group = qobject_cast<QgsLayerTreeGroup*>( node->parent() );
    if ( group )
      group->removeChildNode( node );
  }

  mView->setCurrentIndex( mView->layerTreeModel()->node2index( newGroup ) );
}

void QgsLayerTreeViewDefaultActions::mutuallyExclusiveGroup()
{
  QgsLayerTreeNode* node = mView->currentNode();
  if ( !node || !QgsLayerTree::isGroup( node ) )
    return;

  QgsLayerTree::toGroup( node )->setIsMutuallyExclusive( !QgsLayerTree::toGroup( node )->isMutuallyExclusive() );
}
