#include "sculptgeometry.h"
#include <QMatrix4x4>
#include <QQuaternion>
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <algorithm>
#include <map>
#include <vector>
#include <cmath>

namespace {
constexpr float pi = 3.14159265358979323846f;
struct Vertex { QVector3D position, normal; QVector2D uv; };
struct Mesh {
  std::vector<Vertex> vertices;
  void triangle(QVector3D a, QVector3D b, QVector3D c, QVector3D na, QVector3D nb, QVector3D nc) {
    const auto face=QVector3D::crossProduct((b-a).normalized(),(c-a).normalized()).normalized();
    if(na.lengthSquared()<.01f) na=face;
    if(nb.lengthSquared()<.01f) nb=face;
    if(nc.lengthSquared()<.01f) nc=face;
    // Qt flips back-face normals, so winding must agree with the sculpted normals.
    if (QVector3D::dotProduct(QVector3D::crossProduct(b-a,c-a),na+nb+nc)<0) {
      std::swap(b,c); std::swap(nb,nc);
    }
    auto uv = [](QVector3D p) { return QVector2D(std::atan2(p.z(),p.x())/(2*pi)+.5f,p.y()); };
    vertices.push_back({a,na,uv(a)}); vertices.push_back({b,nb,uv(b)}); vertices.push_back({c,nc,uv(c)});
  }
  void lathe(const std::vector<QVector2D> &profile, const QMatrix4x4 &transform = {}, int sides = 48) {
    const QMatrix4x4 normals = transform.inverted().transposed();
    auto point = [&](QVector2D p, float a) { return transform.map(QVector3D(p.x()*std::cos(a),p.y(),p.x()*std::sin(a))); };
    for (size_t j = 0; j + 1 < profile.size(); ++j) {
      auto a = profile[j], b = profile[j+1];
      float dr = b.x()-a.x(), dy = b.y()-a.y();
      for (int i = 0; i < sides; ++i) {
        float t = 2*pi*i/sides, u = 2*pi*(i+1)/sides;
        auto normal = [&](float angle) { return normals.mapVector(QVector3D(dy*std::cos(angle),-dr,dy*std::sin(angle))).normalized(); };
        auto n = normal(t), m = normal(u);
        triangle(point(a,t),point(b,t),point(b,u),n,n,m);
        triangle(point(a,t),point(b,u),point(a,u),n,m,m);
      }
    }
  }
  void sphere(QVector3D center, QVector3D scale) {
    auto point=[&](float u,float v) { return center+QVector3D(scale.x()*std::sin(v)*std::cos(u),scale.y()*std::cos(v),scale.z()*std::sin(v)*std::sin(u)); };
    auto norm=[&](float u,float v) { return QVector3D(std::sin(v)*std::cos(u)/scale.x(),std::cos(v)/scale.y(),std::sin(v)*std::sin(u)/scale.z()).normalized(); };
    for(int j=0;j<12;++j) for(int i=0;i<24;++i) {
      float a=2*pi*i/24,b=2*pi*(i+1)/24,c=pi*j/12,d=pi*(j+1)/12;
      triangle(point(a,c),point(a,d),point(b,d),norm(a,c),norm(a,d),norm(b,d));
      triangle(point(a,c),point(b,d),point(b,c),norm(a,c),norm(b,d),norm(b,c));
    }
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
  void roundedBox(QVector3D center,QVector3D size,float radius,float rotation) {
    QMatrix4x4 matrix; matrix.translate(center); matrix.rotate(rotation,0,1,0);
    QVector3D core=size*.5f-QVector3D(radius,radius,radius);
    for(int axis=0;axis<3;++axis) for(float sign:{-1.f,1.f}) {
      auto sample=[&](float u,float v) {
        QVector3D p; p[axis]=sign*size[axis]*.5f; p[(axis+1)%3]=u*size[(axis+1)%3]; p[(axis+2)%3]=v*size[(axis+2)%3];
        QVector3D nearest; for(int k=0;k<3;++k) nearest[k]=std::clamp(p[k],-core[k],core[k]);
        auto normal=(p-nearest).normalized();
        return std::pair<QVector3D,QVector3D>(matrix.map(nearest+normal*radius),matrix.mapVector(normal));
      };
      for(int i=0;i<8;++i) for(int j=0;j<8;++j) {
        auto a=sample(i/8.f-.5f,j/8.f-.5f),b=sample((i+1)/8.f-.5f,j/8.f-.5f),c=sample((i+1)/8.f-.5f,(j+1)/8.f-.5f),d=sample(i/8.f-.5f,(j+1)/8.f-.5f);
        triangle(a.first,b.first,c.first,a.second,b.second,c.second); triangle(a.first,c.first,d.first,a.second,c.second,d.second);
      }
    }
  }
  void branch(QVector3D a, QVector3D b, float radius) {
    auto direction = b-a;
    QMatrix4x4 matrix; matrix.translate(a); matrix.rotate(QQuaternion::rotationTo(QVector3D(0,1,0),direction.normalized()));
    lathe({{0,0},{radius,0},{radius*.7f,direction.length()},{0,direction.length()}},matrix);
  }

  void turned(const std::vector<QVector2D> &profile,const QMatrix4x4 &transform = {}) {
    std::vector<QVector2D> smooth;
    for (size_t i=0;i+1<profile.size();++i) {
      auto a=profile[i ? i-1 : i], b=profile[i], c=profile[i+1], d=profile[std::min(i+2,profile.size()-1)];
      for (int j=0;j<5;++j) {
        float t=j/5.f;
        auto p=.5f*((2*b)+(-a+c)*t+(2*a-5*b+4*c-d)*t*t+(-a+3*b-3*c+d)*t*t*t);
        p.setX(std::max(0.f,p.x())); smooth.push_back(p);
      }
    }
    smooth.push_back(profile.back()); lathe(smooth,transform);
  }
  void tube(const std::vector<QVector3D> &points, float radius, float endRadius=-1) {
    if (endRadius<0) endRadius=radius;
    auto tangent=[&](int i) { return (points[std::min(i+1,int(points.size()-1))]-points[std::max(0,i-1)]).normalized(); };
    auto normal=[&](int i,float angle) {
      auto t=tangent(i);
      auto reference=std::abs(t.z())<.9f ? QVector3D(0,0,1) : QVector3D(1,0,0);
      auto u=QVector3D::crossProduct(t,reference).normalized(),v=QVector3D::crossProduct(t,u);
      return u*std::cos(angle)+v*std::sin(angle);
    };
    auto point=[&](int i,float angle) { return points[i]+normal(i,angle)*(radius+(endRadius-radius)*i/(points.size()-1)); };
    for(int i=0;i+1<int(points.size());++i) for(int j=0;j<12;++j) {
      float a=2*pi*j/12,b=2*pi*(j+1)/12;
      triangle(point(i,a),point(i+1,a),point(i+1,b),normal(i,a),normal(i+1,a),normal(i+1,b));
      triangle(point(i,a),point(i+1,b),point(i,b),normal(i,a),normal(i+1,b),normal(i,b));
    }
    for(int i:{0,int(points.size()-1)}) for(int j=0;j<12;++j) {
      auto n=tangent(i)*(i ? 1.f : -1.f);
      triangle(points[i],point(i,2*pi*j/12),point(i,2*pi*(j+1)/12),n,n,n);
    }
  }
  // Elliptical sections follow a curved spine, giving horses a continuous silhouette.
  void loft(const std::vector<QVector4D> &sections, int sides=32) {
    std::vector<QVector4D> rows;
    for (size_t i=0;i+1<sections.size();++i) {
      auto a=sections[i ? i-1 : i],b=sections[i],c=sections[i+1],d=sections[std::min(i+2,sections.size()-1)];
      for (int j=0;j<5;++j) {
        float t=j/5.f;
        rows.push_back(.5f*(2*b+(-a+c)*t+(2*a-5*b+4*c-d)*t*t+(-a+3*b-3*c+d)*t*t*t));
      }
    }
    rows.push_back(sections.back());
    auto point=[&](int row,float angle) {
      auto p=rows[row]; return QVector3D(std::max(.001f,p.y())*std::cos(angle),p.x(),p.z()+std::max(.001f,p.w())*std::sin(angle));
    };
    auto normal=[&](int row,float angle) {
      auto along=point(std::min(row+1,int(rows.size()-1)),angle)-point(std::max(0,row-1),angle);
      auto around=point(row,angle+.01f)-point(row,angle-.01f);
      return QVector3D::crossProduct(along,around).normalized();
    };
    for (int row=0;row+1<int(rows.size());++row) for(int i=0;i<sides;++i) {
      float a=2*pi*i/sides,b=2*pi*(i+1)/sides;
      triangle(point(row,a),point(row+1,a),point(row+1,b),normal(row,a),normal(row+1,a),normal(row+1,b));
      triangle(point(row,a),point(row+1,b),point(row,b),normal(row,a),normal(row+1,b),normal(row,b));
    }
    for(int row:{0,int(rows.size()-1)}) {
      const QVector3D center(0,rows[row].x(),rows[row].z());
      const QVector3D n(0,row ? 1.f : -1.f,0);
      for(int i=0;i<sides;++i) triangle(center,point(row,2*pi*i/sides),point(row,2*pi*(i+1)/sides),n,n,n);
    }
  }
  template<typename Point>
  void sweptSurface(Point point,bool invert=false) {
    auto normal=[&](float u,float v) {
      auto along=point(std::min(1.f,u+.001f),v)-point(std::max(0.f,u-.001f),v);
      auto around=point(u,v+.001f)-point(u,v-.001f);
      return QVector3D::crossProduct(along.normalized(),around.normalized()).normalized()*(invert ? -1.f : 1.f);
    };
    for(int i=0;i<40;++i) for(int j=0;j<16;++j) {
      float a=i/40.f,b=(i+1)/40.f,c=2*pi*j/16,d=2*pi*(j+1)/16;
      triangle(point(a,c),point(b,c),point(b,d),normal(a,c),normal(b,c),normal(b,d));
      triangle(point(a,c),point(b,d),point(a,d),normal(a,c),normal(b,d),normal(a,d));
    }
    for(float u:{0.f,1.f}) {
      auto center=(point(u,0)+point(u,pi))*.5f;
      auto n=QVector3D::crossProduct(point(u,0)-center,point(u,pi*.5f)-center).normalized();
      auto along=point(std::min(1.f,u+.001f),0)-point(std::max(0.f,u-.001f),0);
      if(QVector3D::dotProduct(n,along)*(u==0 ? -1.f : 1.f)<0) n=-n;
      for(int j=0;j<16;++j) triangle(center,point(u,2*pi*j/16),point(u,2*pi*(j+1)/16),n,n,n);
    }
  }
  void panel(float angle) {
    sweptSurface([&](float t,float v) {
      float a=angle+t*.8f,r=.24f-.15f*std::sin(pi*t),width=.085f+.07f*t*t;
      return QVector3D((r+.025f*std::sin(v))*std::cos(a)-width*std::cos(v)*std::sin(a),.20f+t*.92f,(r+.025f*std::sin(v))*std::sin(a)+width*std::cos(v)*std::cos(a));
    },true);
  }
  void crescent(QVector3D center) {
    sweptSurface([&](float t,float v) {
      float a=(65+245*t)*pi/180,taper=std::pow(std::max(0.f,std::sin(pi*t)),.7f);
      float radius=.28f+(.002f+.075f*taper)*std::cos(v);
      return center+QVector3D(radius*std::cos(a),radius*std::sin(a),(.002f+.035f*taper)*std::sin(v));
    });
  }
  void leaf(QVector3D a,QVector3D tip,float width,float bulge=.025f) {
    QVector3D axis=tip-a;
    auto side=QVector3D::crossProduct(axis,{0,0,1}).normalized()*width;
    for (int i=0;i<8;++i) {
      float t=i/8.f,u=(i+1)/8.f;
      auto center=[&](float v) { return a+axis*v+QVector3D(0,0,-bulge*std::sin(pi*v)); };
      auto edge=[&](float v,float sign) { return a+axis*v+side*(sign*std::sin(pi*v)); };
      for(float sign:{-1.f,1.f}) {
        auto p=center(t),q=center(u),r=edge(u,sign),v=edge(t,sign);
        auto n=QVector3D::crossProduct(q-p,r-p).normalized(); if(n.z()>0)n=-n;
        triangle(p,q,r,n,n,n); triangle(p,r,v,n,n,n);
      }
    }
  }
  void flower(QVector3D center,float radius) {
    for(int i=0;i<5;++i) {
      float a=2*pi*i/5;
      sphere(center+QVector3D(std::cos(a)*radius*.6f,std::sin(a)*radius*.6f,-.01f),{radius*.45f,radius*.45f,.025f});
    }
  }
  void arc(QVector3D center,float radius,float start,float end,float tubeRadius) {
    std::vector<QVector3D> points;
    for(int i=0;i<=16;++i) { float a=(start+(end-start)*i/16)*pi/180; points.push_back(center+QVector3D(radius*std::cos(a),radius*std::sin(a),0)); }
    tube(points,tubeRadius);
  }
};
}
SculptGeometry::SculptGeometry(QQuick3DObject *parent) : QQuick3DGeometry(parent) { rebuild(); }
void SculptGeometry::setKind(int value) { if (m_kind != value) { m_kind=value; rebuild(); emit changed(); } }
void SculptGeometry::setDesign(int value) { if (m_design != value) { m_design=value; rebuild(); emit changed(); } }
void SculptGeometry::setPart(int value) { if (m_part != value) { m_part=value; rebuild(); emit changed(); } }
void SculptGeometry::rebuild() {
  struct Sculpture { QByteArray vertices; QVector3D minimum, maximum; };
  static std::map<int,Sculpture> sculptures;
  const int key=(m_design*6+m_kind-1)*6+m_part;
  auto upload=[&](const Sculpture &shape) {
    clear();
    setVertexData(shape.vertices);
    setStride(sizeof(Vertex));
    setPrimitiveType(QQuick3DGeometry::PrimitiveType::Triangles);
    addAttribute(QQuick3DGeometry::Attribute::PositionSemantic,0,QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::NormalSemantic,sizeof(QVector3D),QQuick3DGeometry::Attribute::F32Type);
    addAttribute(QQuick3DGeometry::Attribute::TexCoord0Semantic,2*sizeof(QVector3D),QQuick3DGeometry::Attribute::F32Type);
    setBounds(shape.minimum,shape.maximum);
    update();
  };
  if(m_design<0 || m_design>3 || m_kind<1 || m_kind>6 || m_part<0 || m_part>5) { clear(); update(); return; }
  const auto found=sculptures.find(key);
  if(found!=sculptures.end()) { upload(found->second); return; }
  Mesh mesh;
  const int kind=m_kind,part=m_part;
  const bool orbital=m_design==0, atelier=m_design==1, woodland=m_design==2, candy=m_design==3;
  auto base=[&]() {
    if (candy) mesh.turned({{0,0},{.27f,0},{.34f,.045f},{.35f,.1f},{.32f,.16f},{.27f,.18f},{0,.18f}});
    else mesh.turned({{0,0},{.29f,0},{.335f,.025f},{.34f,.065f},{.33f,.10f},{.29f,.14f},{.27f,.17f},{0,.17f}});
  };
  auto stalk=[&](float top,float neck=.105f) {
    mesh.turned({{0,.14f},{.26f,.14f},{.27f,.21f},{.23f,.27f},{.17f,.34f},{neck,top-.16f},{neck,top-.05f},{.16f,top},{.18f,top+.045f},{.16f,top+.08f},{0,top+.08f}});
  };
  auto crown=[&](float y,bool royal) {
    mesh.ring({0,y,0},.19f,.035f);
    for(int i=0;i<8;++i) {
      float a=2*pi*i/8;
      float h=royal && i%2==0 ? .25f : .17f;
      QVector3D root(.18f*std::cos(a),y,.18f*std::sin(a));
      QVector3D tip(.27f*std::cos(a),y+h,.27f*std::sin(a));
      mesh.tube({root,(root+tip)/2+QVector3D(0,-.03f,0),tip},.032f,.025f);
      mesh.sphere(tip,{.047f,.047f,.047f});
    }
  };
  auto cross=[&](float y) {
    mesh.box({0,y,0},{.065f,.25f,.07f}); mesh.box({0,y+.025f,0},{.20f,.065f,.07f});
  };
  if(part==0) base();
  if (!woodland && part==1 && !candy) mesh.lathe({{.295f,.018f},{.338f,.018f},{.338f,.055f},{.295f,.055f}});

  if (orbital) {
    if(part==0) {
      if(kind==2) {
        mesh.loft({{.15f,.23f,.035f,.22f},{.25f,.25f,.06f,.23f},{.48f,.15f,.11f,.17f},{.76f,.095f,.06f,.14f},{1.03f,.16f,-.04f,.18f},{1.15f,.14f,-.15f,.24f},{1.23f,.10f,-.20f,.21f},{1.28f,.005f,-.17f,.01f}},12);
        for(float side:{-1.f,1.f}) {
          mesh.leaf({side*.10f,.94f,.10f},{side*.20f,1.43f,.23f},.09f,.04f);
          mesh.leaf({side*.06f,.55f,-.12f},{side*.24f,.95f,.12f},.13f,.08f);
        }
      } else if(kind==4) {
        mesh.lathe({{0,.14f},{.27f,.14f},{.28f,.24f},{.23f,.30f},{.21f,.78f},{.29f,.82f},{.30f,.92f},{.23f,.92f},{.23f,.80f},{0,.80f}});
        for(int i=0;i<8;++i) {float a=2*pi*i/8;mesh.box({.265f*std::cos(a),.98f,.265f*std::sin(a)},{.14f,.12f,.10f},-a*180/pi);}
      } else {
        float top=kind==1?.45f:kind==3?.96f:1.12f;
        mesh.loft({{.16f,.25f,0,.25f},{.26f,.23f,0,.23f},{.40f,.16f,0,.16f},{top-.12f,.075f,0,.075f},{top,.10f,0,.10f}},kind==1?32:8);
        if(kind==1) mesh.sphere({0,.61f,0},{.225f,.20f,.225f});
        if(kind>=5) for(int i=0;i<5;++i) mesh.panel(2*pi*i/5);
        if(kind==5) mesh.crescent({.02f,1.28f,0});
      }
    } else if(part==1) {
      if(kind>=5) for(int i=0;i<5;++i) {
        std::vector<QVector3D> edge;
        for(int j=0;j<=16;++j) {
          float t=j/16.f,a=2*pi*i/5+t*.8f,r=.24f-.15f*std::sin(pi*t),width=.085f+.07f*t*t;
          edge.push_back({r*std::cos(a)-width*std::sin(a),.20f+t*.92f,r*std::sin(a)+width*std::cos(a)});
        }
        mesh.tube(edge,.009f);
      }
      if(kind==1) {
        mesh.ring({0,.57f,0},.214f,.015f);
        mesh.ring({0,.61f,-.208f},.068f,.015f,{90,0,0});
      } else if(kind==2) {
        for(float side:{-1.f,1.f}) mesh.tube({{side*.19f,.24f,-.09f},{side*.10f,.54f,-.07f},{side*.15f,.92f,-.19f},{side*.10f,1.18f,-.37f}},.012f);
      } else if(kind==3) {
        mesh.ring({0,1.16f,0},.22f,.024f,{90,0,0});
        mesh.sphere({0,1.42f,0},{.035f,.035f,.035f});
        mesh.tube({{0,.23f,-.23f},{0,.65f,-.11f},{0,1.16f,0}},.012f);
      } else if(kind==4) {
        mesh.ring({0,.84f,0},.293f,.018f);
        for(int i=0;i<8;++i) {float a=2*pi*i/8;mesh.branch({.23f*std::cos(a),.29f,.23f*std::sin(a)},{.23f*std::cos(a),.79f,.23f*std::sin(a)},.012f);}
      } else if(kind==5) {
        // Two offset crescents wrap a floating orb, rather than a conventional crown.
        for(float side:{-1.f,1.f}) {
          std::vector<QVector3D> rim;
          for(int i=0;i<=32;++i) {
            float t=i/32.f,a=(65+245*t)*pi/180;
            float radius=.28f+side*(.002f+.075f*std::pow(std::max(0.f,std::sin(pi*t)),.7f));
            rim.push_back({.02f+radius*std::cos(a),1.28f+radius*std::sin(a),0});
          }
          mesh.tube(rim,.008f);
        }
        mesh.tube({{-.19f,1.02f,0},{-.12f,.86f,-.03f},{.10f,.49f,-.13f},{.24f,.22f,-.12f}},.025f);
      } else {
        mesh.ring({0,1.39f,0},.29f,.016f,{25,0,0});
        mesh.ring({0,1.39f,0},.29f,.016f,{90,20,0});
        mesh.ring({0,1.39f,0},.29f,.016f,{70,-40,0});
        mesh.branch({0,1.08f,0},{0,1.73f,0},.013f);
        for(auto center:{QVector3D(0,1.73f,0),QVector3D(-.30f,1.39f,0),QVector3D(.30f,1.39f,0)}) mesh.sphere(center,{.037f,.037f,.037f});
      }
    } else if(part==2) {
      if(kind==1) mesh.sphere({0,.61f,-.208f},{.17f,.055f,.028f});
      if(kind==2) for(float side:{-1.f,1.f}) mesh.leaf({side*.143f,1.15f,-.18f},{side*.11f,1.19f,-.36f},.022f);
    } else if(part==3) {
      if(kind==1) mesh.sphere({0,.61f,-.24f},{.043f,.043f,.02f});
      if(kind==2) for(float side:{-1.f,1.f}) mesh.leaf({side*.151f,1.16f,-.19f},{side*.115f,1.20f,-.32f},.014f);
      if(kind==3) mesh.sphere({0,1.16f,0},{.074f,.074f,.074f});
      if(kind==5) mesh.sphere({.14f,1.30f,0},{.10f,.10f,.10f});
      if(kind==6) mesh.sphere({0,1.10f,-.12f},{.06f,.075f,.035f});
      if(kind==4) for(int i=0;i<8;++i) {float a=2*pi*i/8;mesh.branch({.296f*std::cos(a),.86f,.296f*std::sin(a)},{.296f*std::cos(a),.94f,.296f*std::sin(a)},.012f);}
    }
  } else if(atelier || candy) {
    if(part==0) {
      if(kind==1) {stalk(candy?.36f:.43f,candy?.14f:.105f);mesh.sphere({0,candy?.61f:.65f,0},{.205f,.205f,.205f});}
      if(kind==2) {
        mesh.loft({{.15f,.24f,.03f,.22f},{.28f,.24f,.05f,.23f},{.48f,.18f,.09f,.19f},{.70f,.125f,.08f,.145f},{.91f,.145f,-.005f,.16f},{1.10f,.16f,-.08f,.17f},{1.22f,.10f,-.06f,.12f},{1.26f,.001f,-.04f,.001f}});
        mesh.loft({{.88f,.045f,-.31f,.08f},{.92f,.11f,-.29f,.18f},{1.0f,.145f,-.22f,.24f},{1.10f,.14f,-.13f,.20f},{1.18f,.08f,-.08f,.12f},{1.21f,.001f,-.07f,.001f}});
        for(float side:{-1.f,1.f}) mesh.tube({{side*.105f,1.15f,.015f},{side*.13f,1.29f,.025f},{side*.10f,1.37f,-.015f}},.06f,.006f);
      }
      if(kind==3) {
        stalk(.76f);
        mesh.turned({{0,.80f},{.10f,.82f},{.20f,.94f},{.205f,1.04f},{.14f,1.18f},{.05f,1.32f},{0,1.43f}});
        if(atelier) mesh.sphere({0,1.43f,0},{.045f,.045f,.045f});
      }
      if(kind==4) {
        mesh.turned(candy ? std::vector<QVector2D>{{0,.15f},{.26f,.15f},{.32f,.24f},{.31f,.34f},{.23f,.39f},{.28f,.47f},{.28f,.58f},{.23f,.65f},{.30f,.73f},{.31f,.87f},{.25f,.92f},{.20f,.87f},{.20f,.74f},{0,.74f}}
                         : std::vector<QVector2D>{{0,.15f},{.25f,.15f},{.27f,.23f},{.22f,.31f},{.18f,.68f},{.23f,.80f},{.30f,.83f},{.30f,.93f},{.23f,.93f},{.23f,.80f},{0,.80f}});
        for(int i=0;i<6;++i) {
          float a=2*pi*i/6;
          if(candy) mesh.roundedBox({.26f*std::cos(a),.97f,.26f*std::sin(a)},{.16f,.22f,.14f},.035f,-a*180/pi);
          else mesh.box({.26f*std::cos(a),1.0f,.26f*std::sin(a)},{.15f,.16f,.10f},-a*180/pi);
        }
      }
      if(kind>=5) {
        float y=kind==5?.93f:1.08f;
        stalk(y,candy?.14f:.11f);
        mesh.turned({{0,y+.06f},{.15f,y+.06f},{.18f,y+.14f},{.25f,y+.29f},{.23f,y+.31f},{.14f,y+.20f},{0,y+.20f}});
      }
    } else if(part==1) {
      if(atelier) {
        if(kind==3) mesh.ring({0,.78f,0},.18f,.022f);
        if(kind>=5) mesh.ring({0,kind==5?.98f:1.13f,0},.18f,.023f);
        if(kind==5) crown(1.16f,false);
        if(kind==6) cross(1.51f);
      } else {
        if(kind==5) crown(1.12f,true);
        if(kind==6) {
          crown(1.27f,false);
          mesh.ring({0,1.42f,0},.20f,.025f,{90,0,0});
          mesh.ring({0,1.42f,0},.20f,.025f,{90,90,0});
          cross(1.72f);
        }
      }
    } else if(part==2) {
      if(kind==2) for(float side:{-1.f,1.f}) {
        if(candy) mesh.arc({side*.145f,1.075f,-.24f},.035f,15,165,.009f);
        else mesh.sphere({side*.15f,1.095f,-.20f},{.014f,.018f,.024f});
        mesh.sphere({side*.085f,.96f,-.451f},{.016f,.012f,.01f});
      }
      if(kind==3 && atelier) mesh.tube({{0,1.06f,-.203f},{0,1.20f,-.13f},{0,1.35f,-.04f}},.012f);
    } else if(part==3 && kind==2) {
      if(candy) {
        mesh.tube({{0,1.24f,.06f},{0,1.23f,.19f},{0,1.08f,.25f},{0,.84f,.26f},{0,.65f,.27f},{0,.57f,.36f},{0,.65f,.42f},{0,.73f,.38f}},.075f,.04f);
        mesh.tube({{0,1.26f,.03f},{0,1.28f,-.07f},{0,1.20f,-.18f}},.07f,.045f);
      } else {
        for(int i=0;i<15;++i) {
          float t=i/14.f;
          mesh.tube({{0,1.24f-t*.72f,.08f+t*.08f},{0,1.19f-t*.72f,.19f+t*.10f},{0,1.10f-t*.66f,.23f+t*.09f}},.018f,.008f);
        }
      }
    }
  } else {
    // Woodland has complete characters instead of themed Staunton bodies.
    if(kind==1) {
      if(part==0) {
        mesh.turned({{0,.14f},{.23f,.14f},{.25f,.22f},{.16f,.40f},{.12f,.47f},{0,.47f}});
        mesh.sphere({0,.61f,-.015f},{.19f,.17f,.17f});
        for(float side:{-1.f,1.f}) mesh.sphere({side*.09f,.23f,-.14f},{.075f,.11f,.09f});
      } else if(part==1) for(int i=0;i<7;++i) {float a=2*pi*i/7;mesh.leaf({.11f*std::cos(a),.48f,.11f*std::sin(a)},{.25f*std::cos(a),.28f,.25f*std::sin(a)},.07f);}
      else if(part==2) {
        for(float side:{-1.f,1.f}) mesh.arc({side*.066f,.62f,-.174f},.027f,200,340,.007f);
        mesh.arc({0,.56f,-.179f},.024f,210,330,.005f);
      } else if(part==3) {
        QMatrix4x4 cap; cap.translate(0,.035f,.06f);
        mesh.turned({{0,.70f},{.25f,.70f},{.26f,.74f},{.21f,.84f},{.11f,.91f},{0,.93f}},cap);
        mesh.branch({0,.945f,.06f},{.02f,1.035f,.06f},.035f);
        for(int row=0;row<3;++row) for(int i=0;i<14;++i) {
          float a=2*pi*(i+.5f*(row%2))/14,r=.235f-row*.05f,y=.76f+row*.055f;
          mesh.sphere({r*std::cos(a),y+.035f,.06f+r*std::sin(a)},{.035f,.027f,.035f});
        }
      }
    } else if(kind==2) {
      if(part==0) {
        mesh.sphere({0,.53f,.06f},{.24f,.36f,.24f});
        mesh.loft({{.75f,.14f,-.015f,.13f},{.91f,.20f,-.06f,.19f},{1.04f,.18f,-.10f,.22f},{1.12f,.08f,-.10f,.15f},{1.15f,.001f,-.08f,.001f}});
        for(float side:{-1.f,1.f}) {
          mesh.tube({{side*.14f,1.05f,.025f},{side*.19f,1.19f,.04f},{side*.17f,1.34f,.015f}},.09f,.005f);
          mesh.tube({{side*.11f,.51f,-.11f},{side*.11f,.23f,-.16f}},.055f,.045f);
        }
        mesh.tube({{.13f,.27f,.12f},{.28f,.27f,.22f},{.30f,.43f,.25f},{.28f,.62f,.22f}},.12f,.07f);
      } else if(part==3) {
        mesh.loft({{.90f,.04f,-.27f,.08f},{.95f,.11f,-.23f,.19f},{1.015f,.10f,-.22f,.15f},{1.04f,.001f,-.17f,.001f}});
        for(int i=0;i<7;++i) {float x=(i-3)*.037f;mesh.leaf({x,.78f,-.16f},{x*.7f,.43f,-.18f},.055f,.035f);}
        mesh.sphere({.28f,.64f,.22f},{.075f,.12f,.08f});
      } else if(part==2) {
        mesh.sphere({0,.965f,-.423f},{.026f,.022f,.025f});
        for(float side:{-1.f,1.f}) mesh.sphere({side*.16f,1.05f,-.19f},{.017f,.018f,.02f});
      } else if(part==1) for(float side:{-1.f,1.f}) for(int i=0;i<4;++i) mesh.leaf({side*.19f,.34f+i*.06f,.01f},{side*.24f,.42f+i*.06f,-.07f},.03f);
    } else if(kind==3) {
      if(part==0) {
        mesh.sphere({0,.62f,0},{.245f,.40f,.21f});
        mesh.sphere({0,1.05f,0},{.27f,.25f,.18f});
        for(float side:{-1.f,1.f}) mesh.tube({{side*.17f,1.16f,0},{side*.25f,1.31f,.02f}},.075f,.005f);
        for(int row=0;row<5;++row) for(int col=-2;col<=2;++col) {
          float x=col*.075f+(row%2)*.02f,y=.4f+row*.10f;
          mesh.leaf({x,y+.10f,-.18f},{x,y,-.20f},.045f,.025f);
        }
      } else if(part==1) for(float side:{-1.f,1.f}) for(int i=0;i<5;++i) mesh.leaf({side*.13f,.95f-i*.035f,-.04f},{side*(.29f-i*.015f),.54f-i*.025f,-.06f},.055f,.03f);
      else if(part==3) for(float side:{-1.f,1.f}) mesh.sphere({side*.12f,1.06f,-.16f},{.13f,.15f,.065f});
      else if(part==4) {
        for(float side:{-1.f,1.f}) mesh.sphere({side*.12f,1.06f,-.218f},{.073f,.084f,.022f});
        mesh.tube({{0,1.02f,-.22f},{0,.93f,-.26f}},.035f,.002f);
      } else if(part==2) for(float side:{-1.f,1.f}) mesh.sphere({side*.12f,1.07f,-.239f},{.045f,.059f,.017f});
    } else if(kind==4) {
      if(part==0) {
        mesh.lathe({{0,.16f},{.26f,.16f},{.25f,.30f},{.22f,.86f},{.29f,.94f},{.29f,1.09f},{.21f,1.09f},{.21f,.92f},{0,.92f}},QMatrix4x4(),32);
        for(int i=0;i<9;++i) {float a=2*pi*i/9;mesh.box({.25f*std::cos(a),1.13f,.25f*std::sin(a)},{.12f,.18f,.12f},-a*180/pi);}
      } else if(part==2) {
        mesh.loft({{.16f,.09f,-.245f,.012f},{.45f,.09f,-.239f,.013f},{.57f,.06f,-.23f,.013f},{.61f,.001f,-.23f,.001f}});
        for(int i=0;i<12;++i) {float a=2*pi*i/12; if(std::sin(a)<-.8f)continue;mesh.tube({{.252f*std::cos(a),.21f,.252f*std::sin(a)},{.232f*std::cos(a+.03f),.60f,.232f*std::sin(a+.03f)},{.24f*std::cos(a),.89f,.24f*std::sin(a)}},.007f);}
      } else if(part==1) for(int i=0;i<5;++i) mesh.leaf({.11f+i*.025f,.22f+i*.06f,-.24f},{.22f+i*.01f,.28f+i*.06f,-.24f},.035f);
      else if(part==3) {
        for(float side:{-1.f,1.f}) {
          float y=side<0?.48f:.70f;
          mesh.branch({side*.22f,.16f,0},{side*.31f,y,0},.025f);
          mesh.sphere({side*.31f,y,0},{.11f,.045f,.10f});
        }
      }
    } else {
      const bool king=kind==6;
      if(part==0) {
        mesh.loft({{.15f,.27f,0,.23f},{.24f,.29f,0,.25f},{.49f,.24f,0,.19f},{.83f,.19f,0,.16f},{1.01f,.15f,0,.12f},{1.09f,.09f,0,.08f}});
        for(float side:{-1.f,1.f}) mesh.tube({{side*.13f,.97f,0},{side*.21f,.77f,-.025f},{side*.18f,.58f,-.13f}},.07f,.055f);
      } else if(part==5) {
        mesh.sphere({0,1.19f,-.02f},{.145f,.19f,.14f});
        mesh.sphere({0,1.19f,-.163f},{.034f,.047f,.035f});
        for(float side:{-1.f,1.f}) mesh.sphere({side*.18f,.61f,-.14f},{.045f,.06f,.04f});
      } else if(part==1) {
        for(float side:{-1.f,1.f}) {
          mesh.leaf({side*.07f,1.03f,-.12f},{side*.25f,.22f,-.19f},.12f,.05f);
          mesh.leaf({side*.13f,.96f,.08f},{side*.28f,.24f,.08f},.16f,.04f);
        }
        for(float side:{-1.f,1.f}) for(int i=0;i<6;++i) {
          float y=.28f+i*.105f;
          mesh.leaf({side*.09f,y+.13f,-.20f},{side*(.23f-i*.014f),y,-.17f},.06f,.025f);
        }
        if(king) {
          QMatrix4x4 hat; hat.translate(0,1.34f,.035f); hat.rotate(25,1,0,0);
          mesh.turned({{0,0},{.26f,0},{.28f,.04f},{.18f,.09f},{.14f,.21f},{.04f,.29f},{0,.28f}},hat);
        } else for(int i=0;i<9;++i) {float a=2*pi*i/9;mesh.leaf({.15f*std::cos(a),1.34f,.15f*std::sin(a)},{.22f*std::cos(a),1.45f,.22f*std::sin(a)},.05f);}
      } else if(part==3) {
        if(king) {
          for(int i=-4;i<=4;++i) {
            float x=i*.03f;
            mesh.tube({{x,1.13f,-.14f},{x*1.3f,1.02f,-.21f},{x*.5f,.87f+std::abs(i)*.025f,-.20f}},.027f,.008f);
          }
          for(float side:{-1.f,1.f}) mesh.tube({{side*.01f,1.16f,-.18f},{side*.09f,1.13f,-.22f},{side*.15f,1.09f,-.17f}},.025f,.01f);
        } else {
          for(float side:{-1.f,1.f}) for(int i=0;i<4;++i) mesh.tube({{side*(.07f+i*.027f),1.36f,.005f},{side*(.13f+i*.018f),1.20f,.045f},{side*(.14f+i*.02f),.99f,.055f},{side*(.19f+i*.02f),.86f,.02f}},.035f,.018f);
        }
      } else if(part==4 && !king) {
        for(auto center:{QVector3D(-.18f,.78f,-.15f),QVector3D(.20f,.48f,-.17f)}) mesh.flower(center,.065f);
        for(int i=0;i<10;++i) {float a=2*pi*i/10;mesh.flower({.18f*std::cos(a),1.38f+.02f*std::sin(a),.16f*std::sin(a)},.055f);}
      } else if(part==4 && king) {
        for(float side:{-1.f,1.f}) {
          mesh.tube({{side*.14f,1.38f,.02f},{side*.28f,1.51f,.04f},{side*.34f,1.70f,.03f},{side*.31f,1.83f,0}},.036f,.012f);
          mesh.tube({{side*.28f,1.53f,.03f},{side*.43f,1.65f,.04f},{side*.46f,1.76f,.02f}},.023f,.007f);
          mesh.tube({{side*.33f,1.68f,.03f},{side*.20f,1.76f,.02f},{side*.20f,1.86f,0}},.021f,.007f);
        }
      } else if(part==2) {
        for(float side:{-1.f,1.f}) mesh.arc({side*.06f,1.24f,-.147f},.025f,195,345,.006f);
        if(!king) mesh.arc({0,1.12f,-.15f},.022f,205,335,.005f);
      }
    }
  }
  if (mesh.vertices.empty()) mesh.triangle({0,0,0},{0,0,0},{0,0,0},{0,1,0},{0,1,0},{0,1,0});
  QVector3D minimum=mesh.vertices.front().position,maximum=minimum;
  for (const auto &vertex : mesh.vertices) for(int axis=0;axis<3;++axis) {
    minimum[axis]=std::min(minimum[axis],vertex.position[axis]);
    maximum[axis]=std::max(maximum[axis],vertex.position[axis]);
  }
  auto stored=sculptures.emplace(key,Sculpture{QByteArray(reinterpret_cast<const char *>(mesh.vertices.data()),int(mesh.vertices.size()*sizeof(Vertex))),minimum,maximum});
  upload(stored.first->second);
}
