#pragma once
#include <QQuick3DGeometry>

class SculptGeometry : public QQuick3DGeometry {
  Q_OBJECT
  Q_PROPERTY(int kind READ kind WRITE setKind NOTIFY changed)
  Q_PROPERTY(int design READ design WRITE setDesign NOTIFY changed)
  Q_PROPERTY(int part READ part WRITE setPart NOTIFY changed)
public:
  explicit SculptGeometry(QQuick3DObject *parent = nullptr);
  int kind() const { return m_kind; }
  int design() const { return m_design; }
  int part() const { return m_part; }
  void setKind(int value);
  void setDesign(int value);
  void setPart(int value);
signals:
  void changed();
private:
  void rebuild();
  int m_kind = 1, m_design = 0, m_part = 0;
};
