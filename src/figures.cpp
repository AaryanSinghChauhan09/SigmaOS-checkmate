#include "figures.h"
#include <QVector2D>
#include <QVector3D>
#include <QVector4D>
#include <algorithm>
#include <cmath>

namespace {
using namespace Clay;
using Figures::Field;
using Figures::Mould;

// Where a mould's solid lies, found on a coarse lattice so no figure needs a hand-measured box.
bool extent(const Field &field,float margin,P &low,P &high) {
  const float step=.055f;
  const int across=11, below=2, tall=40;
  bool found=false;
  for(int k=-across;k<=across;++k) for(int j=-below;j<tall;++j) for(int i=-across;i<=across;++i) {
    const P p{i*step,j*step,k*step};
    const float d=field(p);
    if(d>=step) {
      // A field never overstates the way to its surface by much, so the lattice strides across open air.
      i+=std::max(0,int((std::min(d,2.f)*.7f-step)/step));
      continue;
    }
    if(!found) { low=high=p; found=true; }
    low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};
    high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};
  }
  low=low-P{margin,margin,margin};
  high=high+P{margin,margin,margin};
  return found;
}

void cast(const Mould &mould,SculptFigure &out) {
  const Field &field=mould.field;
  const float cell=mould.cell;
  P low,high;
  if(!extent(field,.055f+2*cell,low,high)) return;
  // The slope of the field, uphill and as steep as the field really is there.
  auto slope=[&](P p) {
    const float e=cell*.4f;
    const P a{1,-1,-1}, b{-1,-1,1}, c{-1,1,-1}, d{1,1,1};
    return (a*field(p+a*e)+b*field(p+b*e)+c*field(p+c*e)+d*field(p+d*e))*(.25f/e);
  };
  const quint32 first=quint32(out.vertices.size());
  shell(field,low,high,cell,[&](P spot) {
    // Two steps down the slope seat each vertex on the surface, so polished faces stay true.
    const P fall=slope(spot);
    const float pitch=std::max(.25f,length(fall));
    for(int step=0;step<2;++step) spot=spot-normalize(fall)*std::clamp(field(spot)/pitch,-cell*.6f,cell*.6f);
    const P rise=slope(spot), normal=normalize(rise);
    const float steep=std::max(.25f,length(rise)), here=field(spot);
    // Shadow gathers where the surface is closed in, and is cast into the vertex with the shape.
    const float open=saturate((.55f*(field(spot+normal*.03f)-here)/.03f+.45f*(field(spot+normal*.09f)-here)/.09f)/steep);
    const float lit=1-mould.shade*(1-open);
    SculptVertex vertex;
    vertex.position=QVector3D(spot.x,spot.y,spot.z);
    vertex.normal=QVector3D(normal.x,normal.y,normal.z);
    // Grain runs through the solid like a plank's, so it meets itself all the way round.
    vertex.uv=QVector2D(.5f+.62f*spot.x+.47f*spot.z,spot.y);
    vertex.color=QVector4D(lit,lit,lit,1);
    out.vertices.push_back(vertex);
    return spot;
  },[&](int a,int b,int c) {
    quint32 corner[3]={first+quint32(a),first+quint32(b),first+quint32(c)};
    const auto &v=out.vertices;
    // Qt flips back-face normals, so winding must agree with the sculpted normals.
    const QVector3D face=QVector3D::crossProduct(v[corner[1]].position-v[corner[0]].position,v[corner[2]].position-v[corner[0]].position);
    if(QVector3D::dotProduct(face,v[corner[0]].normal+v[corner[1]].normal+v[corner[2]].normal)<0) std::swap(corner[1],corner[2]);
    out.indices.insert(out.indices.end(),corner,corner+3);
  });
}
}

SculptFigure Figures::part(int design,int kind,int part) {
  const std::vector<Mould> moulds=design==0 ? Orbital::moulds(kind,part) : design==1 ? Atelier::moulds(kind,part) : Candy::moulds(kind,part);
  SculptFigure figure;
  for(const Mould &mould:moulds) cast(mould,figure);
  return figure;
}
