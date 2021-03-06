// stressed - Stunts/4D [Sports] Driving resource editor
// Copyright (C) 2008-2013 Daniel Stien <daniel@stien.org>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#ifndef SHAPEVIEW_H
#define SHAPEVIEW_H

#include <QAbstractItemView>
#include <QMatrix4x4>

#include "shapemodel.h"

class QGLWidget;

class ShapeView : public QAbstractItemView
{
  Q_OBJECT

public:
  ShapeView(QWidget* parent = 0);

  void              setModel(QAbstractItemModel* model);
  QRect             visualRect(const QModelIndex& /*index*/) const                      { return viewport()->rect(); }
  void              scrollTo(const QModelIndex& /*index*/, ScrollHint /*hint*/ = EnsureVisible) { }
  QModelIndex       indexAt(const QPoint& /*point*/) const                              { return QModelIndex(); }

  void              setVertexSelectionModel(QItemSelectionModel* selection)             { m_vertexSelection = selection; }

public slots:
  void              reset();

signals:
  void              selectedPaintJobChangeRequested(int paintJob);

protected slots:
  void              setCurrentPaintJob(int paintJob);
  void              adjustCurrentPaintJobAfterMove(int oldPosition, int newPosition);
  void              toggleWireframe(bool enable);
  void              toggleShowCullData(bool enable);

protected:
  QModelIndex       moveCursor(CursorAction /*cursorAction*/, Qt::KeyboardModifiers /*modifiers*/) { return QModelIndex(); }

  void              updateGeometries();
  int               horizontalOffset() const                                            { return 0; }
  int               verticalOffset() const                                              { return 0; }

  bool              isIndexHidden(const QModelIndex& /*index*/) const                   { return 0; }

  void              setSelection(const QRect& /*rect*/, QItemSelectionModel::SelectionFlags /*command*/) { }
  QRegion           visualRegionForSelection(const QItemSelection& /*selection*/) const { return QRegion(viewport()->rect()); }

  void              showEvent(QShowEvent* event);
  void              paintEvent(QPaintEvent* event);
  void              mouseMoveEvent(QMouseEvent* event);
  void              mousePressEvent(QMouseEvent* event);

private:
  void              draw(bool pick);
  inline void       drawSphere(const VerticesFList* vertices);
  inline void       drawWheel(const VerticesFList* vertices, int& material, bool& pattern, const bool& selected, const bool& pick);
  inline void       drawHighlightedVertex(const VertexF& vertex);
  inline void       drawCullData(const Primitive& primitive);
  void              setMaterial(const int& material, bool& pattern, const bool& selected, const bool& pick);
  int               pick();

  static VertexF    centroid(const Primitive& primitive);
  static VertexF    centroid(const VertexF& v1, const VertexF& v2);
  static float      distance(const VertexF& v1, const VertexF& v2);

  QGLWidget*        m_glWidget;
  ShapeModel*       m_shapeModel;
  QPoint            m_lastMousePosition;
  QMatrix4x4        m_rotation;
  QMatrix4x4        m_translation;
  int               m_currentPaintJob;
  bool              m_wireframe;
  bool              m_showCullData;

  QItemSelectionModel* m_vertexSelection;

  static const quint8 PATTERNS[6][0x80];

  static const float  VERTEX_HIGHLIGHT_OFFSET;
  static const float  PI2;
  static const float  SPHERE_RADIUS_RATIO;
  static const float  WHEEL_TYRE_RATIO;
  static const int    CIRCLE_STEPS = 16;
};

#endif
