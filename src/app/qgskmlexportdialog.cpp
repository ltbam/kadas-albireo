#include "qgskmlexportdialog.h"
#include "qgsmaplayerregistry.h"
#include "qgsmaplayer.h"
#include <QFileDialog>

QgsKMLExportDialog::QgsKMLExportDialog( const QStringList layerIds, QWidget * parent, Qt::WindowFlags f ): QDialog( parent, f ), mLayerIds( layerIds )
{
  setupUi( this );
  insertAvailableLayers();
}

QgsKMLExportDialog::QgsKMLExportDialog(): QDialog()
{
}

QgsKMLExportDialog::~QgsKMLExportDialog()
{
}

QString QgsKMLExportDialog::saveFile() const
{
  return mFileLineEdit->text();
}

QList<QgsMapLayer*> QgsKMLExportDialog::selectedLayers() const
{
  QList<QgsMapLayer*> layerList;

  int nItems = mLayerListWidget->count();
  for ( int i = 0; i < nItems; ++i )
  {
    QListWidgetItem* item = mLayerListWidget->item( i );
    if ( item && item->checkState() == Qt::Checked )
    {
      QString id = item->data( Qt::UserRole ).toString();
      QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( id );
      if ( layer )
      {
        layerList.append( layer );
      }
    }
  }
  return layerList;
}

void QgsKMLExportDialog::insertAvailableLayers()
{
  mLayerListWidget->clear();
  QStringList::const_iterator it = mLayerIds.constBegin();
  for ( ; it != mLayerIds.constEnd(); ++it )
  {
    QgsMapLayer* layer = QgsMapLayerRegistry::instance()->mapLayer( *it );
    if ( !layer )
    {
      continue;
    }

    QListWidgetItem* item = new QListWidgetItem( layer->name() );
    item->setCheckState( Qt::Checked );
    item->setData( Qt::UserRole, *it );
    mLayerListWidget->addItem( item );
  }
}

void QgsKMLExportDialog::on_mFileSelectionButton_clicked()
{
  QString fileName = QFileDialog::getSaveFileName( 0, tr( "Save KML file" ) );
  if ( !fileName.isEmpty() )
  {
    mFileLineEdit->setText( fileName );
  }
}