// Konstruktor - An interactive LDraw modeler for KDE
// Copyright (c)2006-2011 Park "segfault" J. K. <mastermind@planetmono.org>

#ifndef _SUBMODELMODEL_H_
#define _SUBMODELMODEL_H_

#include <string>

#include <QAbstractItemModel>
#include <QPair>
#include <QVector>

#include "refobject.h"

namespace ldraw
{
	class model;
}

namespace Konstruktor
{

class Document;

class SubmodelModel : public QAbstractItemModel
{
 public:
  SubmodelModel(Document *document, QObject *parent = 0L);
  virtual ~SubmodelModel();
  
  Document* getDocument();
  
  QPair<std::string, ldraw::model *> modelIndexOf(const QModelIndex &index);
  QModelIndex index(const ldraw::model *m);
  
  void resetItems();
  
  // implementation
  
  int columnCount(const QModelIndex &) const { return 1; }
  int rowCount(const QModelIndex &parent = QModelIndex()) const;
  
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  
  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
  QModelIndex parent(const QModelIndex &) const { return QModelIndex(); }
  
  Qt::ItemFlags flags(const QModelIndex &index) const;
  
  QStringList mimeTypes() const;
  QMimeData* mimeData(const QModelIndexList &indexes) const;
  
  private:
  Document *document_;
  QVector<QPair<std::string, ldraw::model *> > submodelList_;
  QList<RefObject> refobjects_;
  int active_;
};

}

#endif
