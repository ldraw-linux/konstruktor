// Konstruktor - An interactive LDraw modeler for KDE
// Copyright (c)2006-2011 Park "segfault" J. K. <mastermind@planetmono.org>

#ifndef _DOCUMENT_H_
#define _DOCUMENT_H_

#include <map>
#include <set>
#include <string>
#include <utility>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QString>

#include <libldr/math.h>
#include <libldr/model.h>
#include <renderer/mouse_rotation.h>

#include "viewport.h"

class QUndoStack;

namespace Konstruktor
{

extern const ldraw::matrix isometricProjectionMatrix;

class SubmodelModel;

class Document : public QObject
{
  Q_OBJECT;
  
 public:
  Document(const QString &name, const QString &desc, const QString &author, QObject *parent = 0L);
  Document(const QString &path, QObject *parent = 0L);
  ~Document();
  
  void sendSignals();
  
  QByteArray save(bool forceMultipart = true) const;
  
  ldraw::model_multipart* contents() { return modelBase_; }
  ldraw::model* getActiveModel() { return activeModel_; }
  QString path() const { return path_; }
  bool setActiveModel(ldraw::model *m);
  bool setActiveModel(const std::string &key);
  void setPath(const QString &path);
  
  bool canSave() const { return canSave_; }
  
  void setSaveable(bool s);
  
  SubmodelModel* model() { return model_; }
  
  // Manipulation
  ldraw::model* newSubmodel(const std::string &name, const std::string &desc, const std::string &author);
  void renameSubmodel(ldraw::model *model, const std::string &newname, const std::string &newdesc);
  void deleteSubmodel(ldraw::model *model);
  
  void updatePixmap();
  bool updatePixmap(ldraw::model *model);
  
  QUndoStack* activeUndoStack();
  QList<QUndoStack *> undoStacks();
  
  // Viewport-related stuff
  Viewport& getViewport(int i) { return viewport_[i]; }
  ldraw_renderer::mouse_rotation& getMouseRotation() { return rotation_;}
  const ldraw::vector& getCenter() const { return center_; }
  const ldraw::vector& getLength() const { return length_; }
  
  void recalibrateScreenDimension();
  void resetPerspective();
  
 signals:
  void undoStackAdded(QUndoStack *editor);
  void undoStackChanged(QUndoStack *editor);
  
 private:
  QString path_;
  ldraw::model_multipart *modelBase_;
  
  ldraw::model *activeModel_;
  QUndoStack *activeUndoStack_;
  
  Viewport viewport_[7];
  ldraw_renderer::mouse_rotation rotation_;
  ldraw::vector center_, length_;
  
  bool canSave_;
  
  SubmodelModel *model_;
  
  //friend class PixmapUpdater;
};

}

#endif
