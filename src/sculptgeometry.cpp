#include "sculptgeometry.h"
#include "figures.h"
#include "woodland.h"
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <algorithm>
#include <map>

SculptGeometry::SculptGeometry(QQuick3DObject *parent) : QQuick3DGeometry(parent) { rebuild(); }
void SculptGeometry::setKind(int value) { if (m_kind != value) { m_kind=value; rebuild(); emit changed(); } }
void SculptGeometry::setDesign(int value) { if (m_design != value) { m_design=value; rebuild(); emit changed(); } }
void SculptGeometry::setPart(int value) { if (m_part != value) { m_part=value; rebuild(); emit changed(); } }
void SculptGeometry::setTone(int value) { if (m_tone != value) { m_tone=value; rebuild(); emit changed(); } }
void SculptGeometry::rebuild() {
  // Sculpting takes a moment, so every part is made once and shared by all the pieces that wear it.
  struct Sculpture { QByteArray vertices, indices; QVector3D minimum, maximum; };
  static std::map<int,Sculpture> sculptures;
  const bool woodland=m_design==2;
  const int key=((m_design*6+m_kind-1)*6+m_part)*2+(woodland && m_tone ? 1 : 0);
  auto upload=[&](const Sculpture &shape) {
    clear();
    setVertexData(shape.vertices);
    setStride(sizeof(SculptVertex));
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,0,QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic,sizeof(QVector3D),QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::TexCoord0Semantic,2*sizeof(QVector3D),QQuick3DGeometry::Attribute::F32Type);
    // Moss & Myth is painted in its vertices; the cast sets keep the shade of their recesses there.
    addAttribute(QQuick3DGeometry::Attribute::ColorSemantic,2*sizeof(QVector3D)+sizeof(QVector2D),QQuick3DGeometry::Attribute::F32Type);
    if(!shape.indices.isEmpty()) {
      setIndexData(shape.indices);
      addAttribute(QQuick3DGeometry::Attribute::IndexSemantic,0,QQuick3DGeometry::Attribute::U32Type);
    }
    setBounds(shape.minimum,shape.maximum);
    update();
  };
  if(m_design<0 || m_design>3 || m_kind<1 || m_kind>6 || m_part<0 || m_part>5) { clear(); update(); return; }
  const auto found=sculptures.find(key);
  if(found!=sculptures.end()) { upload(found->second); return; }
  // Moss & Myth figures are whole painted carvings; the other sets are cast part by part.
  SculptFigure figure;
  if(!woodland) figure=Figures::part(m_design,m_kind,m_part);
  else if(m_part==0) figure=Woodland::figure(m_kind,m_tone);
  // A part a piece does not have is a single point, which draws nothing.
  if(figure.vertices.empty()) {
    figure.vertices.assign(3,SculptVertex{QVector3D(0,0,0),QVector3D(0,1,0),QVector2D(0,0),QVector4D(1,1,1,1)});
    figure.indices.clear();
  }
  QVector3D minimum=figure.vertices.front().position,maximum=minimum;
  for (const auto &vertex : figure.vertices) for(int axis=0;axis<3;++axis) {
    minimum[axis]=std::min(minimum[axis],vertex.position[axis]);
    maximum[axis]=std::max(maximum[axis],vertex.position[axis]);
  }
  Sculpture shape;
  shape.vertices=QByteArray(reinterpret_cast<const char *>(figure.vertices.data()),qsizetype(figure.vertices.size()*sizeof(SculptVertex)));
  shape.indices=QByteArray(reinterpret_cast<const char *>(figure.indices.data()),qsizetype(figure.indices.size()*sizeof(quint32)));
  shape.minimum=minimum; shape.maximum=maximum;
  upload(sculptures.emplace(key,std::move(shape)).first->second);
}
