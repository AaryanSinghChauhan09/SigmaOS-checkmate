#include "figures.h"

// Candy Kingdom is moulded confectionery: gummy rings stacked into a tower, a bonbon on a
// stand, a piped kiss, a carousel pony with a rolled liquorice mane, and royalty in gold leaf.
namespace {
using namespace Clay;
using Figures::Field;
using Figures::Mould;

// A gummy ring, rounded almost through.
float gum(P p,float height,float radius,float half) { return disc(p,height,radius,half,half*.88f); }
// Every sweet sits on two rings, scaled to its rank.
float stand(P p,float scale) { return smin(gum(p,.065f*scale,.34f*scale,.085f*scale),gum(p,.2f*scale,.262f*scale,.062f*scale),.012f*scale); }

float bonbon(P p) {
  const float d=smin(stand(p,.82f),horn(p,{0,.2f,0},{0,.33f,0},.105f,.09f),.02f);
  return smin(d,ball(p,{0,.5f,0},.19f),.03f);
}

// The pony: a round head on a stout neck, a short soft muzzle and two small ears.
float pony(P p) {
  const P m{std::abs(p.x),p.y,p.z};
  float body=horn(narrow(p,.88f),{0,.3f,.03f},{0,.66f,.02f},.2f,.165f)*.88f;
  body=smin(body,egg(p,{0,.87f,-.04f},{.165f,.17f,.185f}),.05f);
  body=smin(body,horn(narrow(p,.9f),{0,.84f,-.13f},{0,.77f,-.235f},.125f,.1f)*.9f,.04f);
  body=smin(body,horn(m,{.085f,.99f,.02f},{.115f,1.12f,.03f},.05f,.02f),.025f);
  return smin(stand(p,.95f),body,.02f);
}

// The mane rolls over the poll and down the neck, and ends wound up like a liquorice wheel.
Field mane() {
  const std::vector<P> fall{{0,.985f,-.155f},{0,1.045f,-.06f},{0,1.04f,.04f},{0,.97f,.12f},{0,.87f,.16f},{0,.75f,.18f},{0,.63f,.2f},{0,.53f,.175f},{0,.45f,.15f}};
  std::vector<P> curl;
  for(int i=0;i<12;++i) {
    const float turn=(270-40*i)*pi/180, reach=.078f-.0052f*i;
    curl.push_back({0,.45f+reach*std::cos(turn),.228f+reach*std::sin(turn)});
  }
  return [=](P p) { return smin(strand(p,fall,.058f,.05f),strand(p,curl,.036f,.03f),.01f); };
}

// Eyes closed in a smile: an arch drawn onto each side of the head.
Field lashes() {
  std::vector<P> lid;
  for(int i=0;i<=8;++i) {
    const float a=(25+130*i/8.f)*pi/180;
    lid.push_back(Figures::land(pony,{.5f,.885f+.036f*std::sin(a),-.105f+.042f*std::cos(a)},{-1,0,0})+P{.003f,0,0});
  }
  return [=](P p) { return strand({std::abs(p.x),p.y,p.z},lid,.0105f,.0105f); };
}

// A kiss piped in a slow spiral, drawn up to a point.
float kiss(P p) {
  const float d=smin(stand(p,.92f),gum(p,.295f,.175f,.062f),.012f);
  const float swirl=.011f*std::sin(3*std::atan2(p.z,p.x)+15*(p.y-.5f))*smooth(.4f,.52f,p.y)*smooth(1.02f,.9f,p.y)*std::min(1.f,hypot2(p.x,p.z)/.1f);
  return smin(d,(horn(p,{0,.565f,0},{0,1.1f,0},.215f,.022f)-swirl)*.85f,.012f);
}

// Three rings and a keep, its wall parted into four fat merlons.
float tower(P p) {
  float d=smin(gum(p,.075f,.335f,.095f),gum(p,.255f,.3f,.09f),.012f);
  d=smin(d,gum(p,.43f,.275f,.09f),.012f);
  float keep=smax(disc(p,.71f,.287f,.21f,.055f),-disc(p,.98f,.18f,.14f,.03f),.02f);
  const float gaps=std::min(brick({p.x,p.y-.95f,p.z},{.052f,.105f,.5f},.02f),brick({p.x,p.y-.95f,p.z},{.5f,.105f,.052f},.02f));
  keep=smax(keep,-gaps,.015f);
  return smin(d,keep,.012f);
}

Field queen() {
  const Lathe gown({{0,.22f},{.19f,.235f},{.178f,.31f},{.152f,.42f},{.128f,.55f},{.113f,.68f},{.107f,.8f},{0,.81f}});
  const Lathe bust({{0,.85f},{.112f,.86f},{.127f,.91f},{.162f,.99f},{.197f,1.05f},{.21f,1.09f},{.202f,1.125f},{.162f,1.15f},{.08f,1.165f},{0,1.168f}});
  return [=](P p) {
    const float rho=hypot2(p.x,p.z);
    float d=smin(stand(p,1),gown.distance(rho,p.y),.012f);
    d=smin(d,gum(p,.835f,.166f,.04f),.012f);
    return smin(d,bust.distance(rho,p.y),.012f);
  };
}

Field king() {
  const Lathe gown({{0,.22f},{.195f,.235f},{.184f,.32f},{.158f,.45f},{.133f,.6f},{.118f,.76f},{.112f,.89f},{0,.9f}});
  const Lathe bust({{0,.94f},{.117f,.95f},{.132f,1.0f},{.167f,1.09f},{.202f,1.16f},{.215f,1.2f},{.207f,1.235f},{.167f,1.26f},{.08f,1.275f},{0,1.278f}});
  return [=](P p) {
    const float rho=hypot2(p.x,p.z);
    float d=smin(stand(p,1),gown.distance(rho,p.y),.012f);
    d=smin(d,gum(p,.925f,.172f,.042f),.012f);
    return smin(d,bust.distance(rho,p.y),.012f);
  };
}

// A gold crown: a flaring band whose rim rises into points, one of them to the front.
float coronet(P p,float base,float radius,float flare,float band,float peak,int points) {
  const float rho=hypot2(p.x,p.z), h=p.y-base;
  const float turn=(std::atan2(p.z,p.x)+pi/2)*points/(2*pi), valley=2*std::abs(turn-std::round(turn));
  const float wall=std::abs(rho-radius-flare*h)-.015f;
  // The rim climbs steeply, so the distance is eased to stay an honest underestimate.
  return smax(wall,std::max(-h,h-band-peak*(1-valley)),.006f)*.55f;
}

// Five tall points tipped with gold beads.
float tiara(P p) {
  return std::min(coronet(p,1.11f,.188f,.22f,.07f,.16f,5),ball(wedge(p,5,-pi/2),{0,1.345f,-.239f},.036f));
}

// A low band under four arches that meet in an orb and a cross.
float regalia(P p) {
  static const std::vector<P> rib{{0,1.29f,-.2f},{0,1.4f,-.222f},{0,1.49f,-.17f},{0,1.54f,-.085f},{0,1.555f,0}};
  float d=std::min(coronet(p,1.22f,.192f,.12f,.075f,.03f,8),strand(wedge(p,4,-pi/2),rib,.024f,.024f));
  d=smin(d,ball(p,{0,1.585f,0},.05f),.008f);
  d=smin(d,horn(p,{0,1.62f,0},{0,1.655f,0},.03f,.026f),.006f);
  return smin(d,pattee(p,{0,1.715f,0},.08f,.022f,.045f,.022f),.006f);
}
}

std::vector<Mould> Candy::moulds(int kind,int part) {
  if(kind<1 || kind>6) return {};
  if(part==0) {
    const Field sweet=kind==1 ? Field(bonbon) : kind==2 ? Field(pony) : kind==3 ? Field(kiss) : kind==4 ? Field(tower) : kind==5 ? queen() : king();
    return {{sweet,kind==1 ? .016f : .018f,.3f}};
  }
  if(part==1 && kind==5) return {{tiara,.008f,.3f}};
  if(part==1 && kind==6) return {{regalia,.008f,.3f}};
  if(part==2 && kind==2) return {{lashes(),.005f,0}};
  if(part==3 && kind==2) return {{mane(),.012f,.3f}};
  // A cushion of the darker sweet swells between the arches of the king's crown.
  if(part==3 && kind==6) return {{+[](P p) { return egg(p,{0,1.36f,0},{.185f,.17f,.185f}); },.014f,.3f}};
  return {};
}
