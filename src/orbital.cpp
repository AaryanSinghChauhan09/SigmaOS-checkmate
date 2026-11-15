#include "figures.h"

// Orbital Orders is a court among the stars in miniature: pale ceramic or gunmetal hulls on
// ringed decks, gold instruments for the royals, and small lights behind dark glass.
namespace {
using namespace Clay;
using Figures::Field;
using Figures::Mould;

// Every piece stands on a deck, scaled to its rank: a plate under a low cone, a gold ring let into the step.
const Lathe deck({{0,.04f},{.31f,.05f},{.3f,.085f},{.25f,.118f},{.2f,.15f},{0,.15f}});
const float rank[]={0,.82f,.95f,.92f,.98f,1,1};
// The steed's head is laid out along its own axis: across, above the nose line, and down it from the poll.
const Frame skull({0,1.0f,-.04f},{0,.832f,-.555f},{0,-.555f,-.832f});

float base(P p,float scale) {
  const P q=p*(1/scale);
  return smin(disc(q,.025f,.35f,.045f,.014f),deck.distance(hypot2(q.x,q.z),q.y),.006f)*scale;
}
float ring(P p,float height,float radius,float tube) { return hoop({p.x,p.y-height,p.z},radius,tube); }

// The sentinel: a beacon dome on a flared collar, parted all the way round by a slot of dark glass.
Field sentinel() {
  const Lathe neck({{0,.1f},{.168f,.11f},{.15f,.17f},{.122f,.25f},{.106f,.33f},{.116f,.38f},{.15f,.408f},{0,.42f}});
  return [=](P p) {
    const float rho=hypot2(p.x,p.z);
    const float dome=smax(egg(p,{0,.6f,0},{.2f,.215f,.2f}),-std::max(std::abs(p.y-.6f)-.036f,.183f-rho),.006f);
    return smin(smin(base(p,.82f),neck.distance(rho,p.y),.008f),dome,.008f);
  };
}

// The comet steed: a blade of a neck over a breastplate, a long tapering head, and ears swept back like fins.
float steed(P p) {
  const P m{std::abs(p.x),p.y,p.z};
  float body=horn(narrow(p,.62f),{0,.2f,.03f},{0,.62f,.07f},.215f,.13f)*.62f;
  body=smin(body,horn(narrow(p,.6f),{0,.62f,.07f},{0,.96f,-.02f},.13f,.105f)*.6f,.04f);
  body=smin(body,egg(p,{0,.36f,-.1f},{.13f,.2f,.14f}),.04f);
  float head=horn(narrow(p,.62f),{0,1.0f,-.04f},{0,.82f,-.31f},.108f,.04f)*.62f;
  head=smin(head,egg(p,{0,.9f,-.09f},{.07f,.085f,.11f}),.03f);
  head=smin(head,horn(m,{.045f,1.04f,0},{.075f,1.3f,.2f},.04f,.008f),.02f);
  return smin(base(p,.95f),smin(body,head,.03f),.02f);
}

// Its mane is one gold crescent standing off the back of the neck.
float crest(P p) {
  const float fin=std::max(hypot2(p.y-.66f,p.z+.07f)-.36f,.02f-p.z);
  return smax(fin,std::abs(p.x)-.012f-.3f*std::min(std::max(-fin,0.f),.05f),.006f)*.8f;
}

// The halo spire: a slender hull that carries a ring and its light.
Field spire() {
  const Lathe hull({{0,.11f},{.19f,.12f},{.172f,.2f},{.14f,.34f},{.108f,.52f},{.082f,.7f},{.066f,.86f},{.06f,.96f},{.072f,1.0f},{0,1.01f}});
  return [=](P p) { return smin(smin(base(p,.92f),hull.distance(hypot2(p.x,p.z),p.y),.008f),disc(p,.9f,.085f,.014f,.008f),.004f); };
}
float halo(P p) {
  float d=std::min(hoop({p.x,p.z,p.y-1.2f},.165f,.021f),horn(p,{0,1.03f,0},{0,1.38f,0},.011f,.011f));
  d=smin(d,horn(p,{0,.99f,0},{0,1.045f,0},.05f,.036f),.008f);
  return smin(d,ball(p,{0,1.405f,0},.03f),.006f);
}
// A seam of light runs down the front of the hull.
Field seam() {
  const Field hull=spire();
  std::vector<P> line;
  for(int i=0;i<8;++i) line.push_back(Figures::land(hull,{0,.25f+.085f*i,-.6f},{0,0,1}));
  return [=](P p) { return strand(p,line,.011f,.011f); };
}

// The bastion: a waisted keep under a ring of eight merlons, a lit port below each.
Field bastion() {
  const Lathe keep({{0,.12f},{.215f,.13f},{.2f,.2f},{.186f,.32f},{.186f,.5f},{.2f,.6f},{.236f,.66f},{.262f,.7f},{.27f,.74f},{.27f,.93f},{.262f,.95f},{0,.95f}});
  return [=](P p) {
    float d=smin(base(p,.98f),keep.distance(hypot2(p.x,p.z),p.y),.008f);
    d=smax(d,-disc(p,1.0f,.2f,.11f,.012f),.006f);
    const P gap=wedge(p,8,pi/8), port=wedge(p,8,0);
    d=smax(d,-brick({gap.x,gap.y-.955f,gap.z+.25f},{.04f,.05f,.1f},.006f),.005f);
    return smax(d,-brick({port.x,port.y-.81f,port.z+.27f},{.022f,.05f,.04f},.01f),.005f);
  };
}

// The crescent regent: a gown of panels that overlap as they wind upward, under a standing moon.
Field regent() {
  const Lathe gown({{0,.12f},{.225f,.13f},{.215f,.2f},{.19f,.34f},{.16f,.52f},{.13f,.72f},{.11f,.88f},{.105f,.98f},{.125f,1.04f},{.14f,1.075f},{.12f,1.09f},{0,1.09f}});
  return [=](P p) {
    const float turn=5*std::atan2(p.z,p.x)/(2*pi)+1.1f*p.y, lap=turn-std::floor(turn)-.5f;
    const float step=.03f*lap*smooth(.16f,.3f,p.y)*smooth(1.0f,.86f,p.y);
    return smin(base(p,1),gown.distance(hypot2(p.x,p.z)+step,p.y),.008f);
  };
}
// Her moon is thickest at its back and fine at the horns, and stands in a boss on her collar.
float moon(P p) {
  const float sickle=std::max(hypot2(p.x-.01f,p.y-1.345f)-.255f,.215f-hypot2(p.x+.075f,p.y-1.385f));
  float d=smax(sickle,std::abs(p.z)-.014f-.42f*std::min(std::max(-sickle,0.f),.07f),.008f)*.8f;
  d=smin(d,horn(p,{0,1.07f,0},{0,1.13f,0},.07f,.04f),.012f);
  return std::min(d,ring(p,.99f,.107f,.014f));
}

// The armillary sovereign: a pleated robe whose high collar opens in a vee, under the rings of an armillary.
Field sovereign() {
  const Lathe robe({{0,.12f},{.245f,.13f},{.24f,.22f},{.222f,.4f},{.2f,.6f},{.178f,.8f},{.16f,.95f},{.158f,1.02f},{.185f,1.09f},{.215f,1.14f},{.21f,1.165f},{.17f,1.16f},{.1f,1.13f},{0,1.12f}});
  return [=](P p) {
    const float pleat=.02f*(1-std::abs(std::sin(4*std::atan2(p.z,p.x))))*smooth(.18f,.32f,p.y)*smooth(.98f,.84f,p.y);
    const float d=smax(robe.distance(hypot2(p.x,p.z)+pleat,p.y),-std::max(std::abs(p.x)-(p.y-.99f)*.5f,p.z+.06f)*.85f,.008f);
    return smin(base(p,1),d,.008f);
  };
}
Field armillary() {
  const P heart{0,1.44f,0};
  const Frame meridian(heart,{0,0,1},{0,1,0}), ecliptic(heart,{.34f,.94f,0},{0,0,1}), orbit(heart,{.68f,.33f,.65f},{0,1,0});
  return [=](P p) {
    float d=std::min(hoop(meridian.local(p),.215f,.013f),std::min(hoop(ecliptic.local(p),.198f,.013f),hoop(orbit.local(p),.181f,.013f)));
    d=std::min(d,horn(p,{0,1.1f,0},{0,1.7f,0},.012f,.012f));
    d=smin(d,horn(p,{0,1.11f,0},{0,1.2f,0},.05f,.02f),.01f);
    d=std::min(d,ball(p,{0,1.715f,0},.03f));
    // A torque rings the collar, broken where the vee opens.
    return std::min(d,smax(ring(p,1.15f,.212f,.013f),-std::max(std::abs(p.x)-(p.y-.99f)*.5f-.012f,p.z+.06f),.004f));
  };
}
}

std::vector<Mould> Orbital::moulds(int kind,int part) {
  if(kind<1 || kind>6) return {};
  if(part==0) {
    const Field hull=kind==1 ? sentinel() : kind==2 ? Field(steed) : kind==3 ? spire() : kind==4 ? bastion() : kind==5 ? regent() : sovereign();
    return {{hull,kind<3 ? .015f : .017f,.5f}};
  }
  if(part==1) {
    const float scale=rank[kind];
    std::vector<Mould> gold{{[=](P p) { return ring(p,.07f*scale,.322f*scale,.013f); },.007f,.3f}};
    if(kind==1) gold.push_back({+[](P p) { return ring(p,.405f,.148f,.016f); },.008f,.3f});
    if(kind==2) gold.push_back({crest,.009f,.3f});
    if(kind==3) gold.push_back({halo,.007f,.3f});
    if(kind==4) gold.push_back({+[](P p) { return std::min(ring(p,.7f,.264f,.014f),ring(p,.41f,.186f,.012f)); },.007f,.3f});
    if(kind==5) gold.push_back({moon,.0075f,.3f});
    if(kind==6) gold.push_back({armillary(),.007f,.3f});
    return gold;
  }
  if(part==2) {
    // Dark glass: the band round the sentinel's dome, and the well of the bastion.
    if(kind==1) return {{+[](P p) { return disc(p,.6f,.19f,.033f,.006f); },.01f,.2f}};
    if(kind==4) return {{+[](P p) { return disc(p,.892f,.196f,.008f,.003f); },.006f,.2f}};
  }
  if(part==3) {
    // Lights: a lens, eye slits, the orbs the instruments hold, and what shows through ports and seams.
    if(kind==1) return {{+[](P p) { return egg(p,{0,.6f,-.186f},{.052f,.034f,.03f}); },.006f,0}};
    if(kind==2) return {{+[](P p) { const P q=skull.local(p); return egg({std::abs(q.x),q.y,q.z},{.047f,.03f,.1f},{.013f,.014f,.055f}); },.005f,0}};
    if(kind==3) return {{+[](P p) { return ball(p,{0,1.2f,0},.068f); },.01f,0},{seam(),.005f,0}};
    if(kind==4) return {{+[](P p) { return disc(p,.81f,.247f,.046f,.004f); },.01f,0}};
    if(kind==5) return {{+[](P p) { return ball(p,{-.075f,1.245f,0},.085f); },.012f,0}};
    const P gem=Figures::land(sovereign(),{0,.9f,-.6f},{0,0,1});
    return {{+[](P p) { return ball(p,{0,1.44f,0},.06f); },.01f,0},{[=](P p) { return egg(p,gem,{.036f,.048f,.02f}); },.006f,0}};
  }
  return {};
}
