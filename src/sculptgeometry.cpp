#include "sculptgeometry.h"
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>
#include <QVector3D>
#include <vector>
#include <cmath>

namespace {
constexpr float pi = 3.14159265358979323846f;
struct Vertex { QVector3D position, normal; };
struct Mesh {
  std::vector<Vertex> vertices;
  void triangle(QVector3D a, QVector3D b, QVector3D c, QVector3D na, QVector3D nb, QVector3D nc) {
    vertices.push_back({a,na}); vertices.push_back({b,nb}); vertices.push_back({c,nc});
  }
  void lathe(const std::vector<QVector2D> &profile, const QMatrix4x4 &transform = {}) {
    const int sides = 48;
    auto point = [&](QVector2D p, float a) { return transform.map(QVector3D(p.x()*std::cos(a),p.y(),p.x()*std::sin(a))); };
    for (size_t j = 0; j + 1 < profile.size(); ++j) {
      auto a = profile[j], b = profile[j+1];
      float dr = b.x()-a.x(), dy = b.y()-a.y();
      for (int i = 0; i < sides; ++i) {
        float t = 2*pi*i/sides, u = 2*pi*(i+1)/sides;
        auto normal = [&](float angle) { return transform.mapVector(QVector3D(dy*std::cos(angle),-dr,dy*std::sin(angle))).normalized(); };
        auto n = normal(t), m = normal(u);
        triangle(point(a,t),point(b,t),point(b,u),n,n,m);
        triangle(point(a,t),point(b,u),point(a,u),n,m,m);
      }
    }
  }
  void sphere(QVector3D center, QVector3D scale) {
    std::vector<QVector2D> profile;
    for (int i = 0; i <= 24; ++i) {
      float t = pi*i/24;
      profile.push_back({std::sin(t),-std::cos(t)});
    }
    QMatrix4x4 matrix; matrix.translate(center); matrix.scale(scale); lathe(profile,matrix);
  }
  void ring(QVector3D center, float radius, float tube, QVector3D rotation = {}, float sweep = 360) {
    QMatrix4x4 transform; transform.translate(center); transform.rotate(rotation.x(),1,0,0); transform.rotate(rotation.y(),0,1,0); transform.rotate(rotation.z(),0,0,1);
    auto point = [&](float u, float v) { return transform.map(QVector3D((radius+tube*std::cos(v))*std::cos(u),tube*std::sin(v),(radius+tube*std::cos(v))*std::sin(u))); };
    auto norm = [&](float u, float v) { return transform.mapVector(QVector3D(std::cos(v)*std::cos(u),std::sin(v),std::cos(v)*std::sin(u))).normalized(); };
    for (int i = 0; i < 64; ++i) for (int j = 0; j < 10; ++j) {
      float a = sweep*pi/180*i/64, b = sweep*pi/180*(i+1)/64, c = 2*pi*j/10, d = 2*pi*(j+1)/10;
      triangle(point(a,c),point(b,c),point(b,d),norm(a,c),norm(b,c),norm(b,d));
      triangle(point(a,c),point(b,d),point(a,d),norm(a,c),norm(b,d),norm(a,d));
    }
  }
  void box(QVector3D center, QVector3D size, float rotate = 0) {
    QMatrix4x4 matrix; matrix.translate(center); matrix.rotate(rotate,0,1,0); matrix.scale(size);
    QVector3D v[] = {{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f},
                     {-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f}};
    int faces[][4] = {{0,3,2,1},{4,5,6,7},{0,4,7,3},{1,2,6,5},{3,7,6,2},{0,1,5,4}};
    for (auto &face : faces) {
      auto a=matrix.map(v[face[0]]), b=matrix.map(v[face[1]]), c=matrix.map(v[face[2]]), d=matrix.map(v[face[3]]);
      auto n = QVector3D::crossProduct(b-a,c-a).normalized();
      triangle(a,b,c,n,n,n); triangle(a,c,d,n,n,n);
    }
  }
  void branch(QVector3D a, QVector3D b, float radius) {
    auto direction = b-a;
    QMatrix4x4 matrix; matrix.translate(a); matrix.rotate(QQuaternion::rotationTo(QVector3D(0,1,0),direction.normalized()));
    lathe({{0,0},{radius,0},{radius*.7f,direction.length()},{0,direction.length()}},matrix);
  }
};
}
SculptGeometry::SculptGeometry(QQuick3DObject *parent) : QQuick3DGeometry(parent) { rebuild(); }
void SculptGeometry::setKind(int value) { if (m_kind != value) { m_kind=value; rebuild(); emit changed(); } }
void SculptGeometry::setDesign(int value) { if (m_design != value) { m_design=value; rebuild(); emit changed(); } }
void SculptGeometry::setPart(int value) { if (m_part != value) { m_part=value; rebuild(); emit changed(); } }
void SculptGeometry::rebuild() {
  Mesh mesh;
  const bool orbital = m_design==0, woodland=m_design==2, candy=m_design==3;
  const float height = m_kind==1 ? .75f : m_kind==4 ? 1.02f : m_kind==2 ? 1.12f : m_kind==3 ? 1.2f : m_kind==5 ? 1.38f : 1.5f;
  if (m_part==0) {
    mesh.lathe({{0,0},{.27f,0},{.32f,.035f},{.33f,.085f},{.31f,.13f},{.26f,.17f},{.24f,.19f},{0,.19f}});
    if (m_kind==2) {
      mesh.lathe({{0,.17f},{.23f,.17f},{.22f,.26f},{.18f,.36f},{0,.36f}});
      mesh.sphere({0,.55f,.04f},{.16f,.3f,.19f});
      mesh.sphere({0,.79f,0},{.17f,.27f,.19f});
      mesh.sphere({0,.91f,-.14f},{.17f,.16f,.26f});
      mesh.sphere({0,.84f,-.31f},{.14f,.12f,.13f});
      for (float x : {-.1f,.1f}) {
        QMatrix4x4 matrix; matrix.translate(x,1.01f,.015f);
        mesh.lathe({{0,0},{.072f,0},{.015f,woodland?.27f:.2f},{0,.22f}},matrix);
      }
    } else if (m_kind==4) {
      mesh.lathe({{0,.17f},{.25f,.17f},{.21f,.25f},{.2f,.69f},{.29f,.75f},{.29f,.85f},{.2f,.85f},{.2f,.74f},{0,.74f}});
      for (int i=0;i<6;++i) {
        float a=2*pi*i/6;
        mesh.box({.24f*std::cos(a),.91f,.24f*std::sin(a)},{.13f,.19f,.14f},-a*180/pi);
      }
    } else {
      const float neck = height*.6f;
      mesh.lathe({{0,.17f},{.24f,.17f},{.22f,.24f},{.17f,.3f},{.12f,neck-.13f},{.11f,neck},
                  {.2f,neck+.025f},{.21f,neck+.075f},{.16f,neck+.105f},{0,neck+.105f}});
      if (m_kind==1) {
        mesh.sphere({0,.6f,0},{.19f,.18f,.19f});
        if (woodland) mesh.lathe({{0,.65f},{.3f,.65f},{.26f,.73f},{.15f,.83f},{0,.86f}});
      } else if (m_kind==3) {
        if (woodland) {
          mesh.sphere({0,.95f,0},{.23f,.27f,.19f});
          for (float x : {-.17f,.17f}) mesh.branch({x,1.06f,0},{x*1.15f,1.25f,0},.075f);
        } else mesh.lathe({{0,.79f},{.14f,.79f},{.21f,.9f},{.19f,1.02f},{.08f,1.14f},{0,1.28f}});
      } else if (!orbital) {
        mesh.lathe({{0,height*.66f},{.15f,height*.66f},{.17f,height*.77f},{.24f,height*.86f},{.25f,height*.91f},{.18f,height*.91f},{0,height*.85f}});
        if (woodland) mesh.sphere({0,height*.82f,-.04f},{.15f,.18f,.14f});
      }
    }
  } else if (m_part==1) {
    mesh.ring({0,.135f,0},.287f,.018f);
    if (orbital) mesh.ring({0,.045f,0},.32f,.012f);
    if (m_kind==1) {
      if (orbital) mesh.ring({0,.61f,0},.182f,.023f,{15,0,0});
      else if (woodland) mesh.sphere({.07f,.79f,-.08f},{.035f,.02f,.035f});
      else mesh.ring({0,.44f,0},.15f,.018f);
    } else if (m_kind==2) {
      if (orbital) mesh.branch({0,.48f,.17f},{0,1.03f,.17f},.04f);
      else mesh.sphere({0,.8f,.17f},{.08f,.26f,.08f});
    } else if (m_kind==3) {
      if (orbital) mesh.ring({0,1.08f,0},.22f,.023f,{90,0,0});
      else if (woodland) for (float x : {-.09f,.09f}) mesh.sphere({x,1.0f,-.17f},{.09f,.1f,.04f});
      else mesh.ring({0,.85f,0},.17f,.022f);
    } else if (m_kind==4) {
      mesh.ring({0,.77f,0},.29f,.022f);
      if (orbital) for (int i=0;i<6;++i) { float a=2*pi*i/6; mesh.sphere({.265f*std::cos(a),.9f,.265f*std::sin(a)},{.027f,.05f,.027f}); }
    } else if (orbital) {
      mesh.sphere({0,1.16f,0},{.09f,.09f,.09f});
      if (m_kind==6) {
        mesh.ring({0,1.2f,0},.28f,.022f,{30,0,0});
        mesh.ring({0,1.2f,0},.28f,.022f,{90,30,0});
        mesh.ring({0,1.2f,0},.28f,.022f,{70,-40,0});
        mesh.sphere({0,1.52f,0},{.04f,.04f,.04f});
      } else {
        mesh.ring({0,1.19f,0},.26f,.045f,{90,0,-35},265);
        mesh.sphere({0,1.48f,0},{.035f,.035f,.035f});
      }
    } else if (woodland && m_kind==6) {
      for (float side : {-1.f,1.f}) {
        mesh.branch({side*.1f,1.24f,0},{side*.26f,1.62f,.04f},.035f);
        mesh.branch({side*.19f,1.43f,.02f},{side*.36f,1.5f,.04f},.025f);
        mesh.branch({side*.24f,1.55f,.03f},{side*.15f,1.69f,.04f},.022f);
      }
    } else if (m_kind==5 || candy || woodland) {
      for (int i=0;i<7;++i) {
        float a=2*pi*i/7;
        mesh.sphere({.22f*std::cos(a),height*.94f,.22f*std::sin(a)},{.06f,.07f,.06f});
      }
      if (m_kind==6) mesh.sphere({0,height+ .04f,0},{.08f,.12f,.08f});
    } else {
      mesh.box({0,1.49f,0},{.07f,.28f,.07f});
      mesh.box({0,1.53f,0},{.23f,.07f,.07f});
    }
  } else {
    if (m_kind==2) for (float x : {-.15f,.15f}) mesh.sphere({x,.96f,-.15f},{.025f,.03f,.03f});
    if (orbital && m_kind==1) mesh.sphere({0,.64f,-.17f},{.11f,.045f,.04f});
    if (woodland && m_kind==3) for (float x : {-.09f,.09f}) mesh.sphere({x,1.01f,-.205f},{.035f,.05f,.022f});
    if (candy && m_kind==1) for (float x : {-.06f,.06f}) mesh.sphere({x,.63f,-.17f},{.018f,.024f,.015f});
  }
  clear();
  if (mesh.vertices.empty()) mesh.triangle({0,0,0},{0,0,0},{0,0,0},{0,1,0},{0,1,0},{0,1,0});
  setVertexData(QByteArray(reinterpret_cast<const char *>(mesh.vertices.data()),int(mesh.vertices.size()*sizeof(Vertex))));
  setStride(sizeof(Vertex));
  setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
  addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,0,QQuick3DGeometry::Attribute::F32Type);
  addAttribute(QQuick3DGeometry::Attribute::NormalSemantic,sizeof(QVector3D),QQuick3DGeometry::Attribute::F32Type);
  setBounds({-.5f,0,-.5f},{.5f,1.8f,.5f});
  update();
}
