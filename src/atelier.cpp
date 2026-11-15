#include "figures.h"

// Midnight Atelier is a turned Staunton set: boxwood and walnut on brass shoes, a brass ring
// under each crowned head, and a horse carved the way a club knight is.
namespace {
using namespace Clay;
using Figures::Field;
using Figures::Mould;

// Every piece stands on the same foot, scaled to its rank: a wooden bead over a brass shoe.
const Lathe sole({{0,-.02f},{.34f,-.02f},{.35f,0},{.35f,.06f},{.336f,.078f},{.334f,.09f},{.346f,.115f},{.343f,.14f},{.318f,.168f},{.275f,.188f},{.235f,.2f},{0,.2f}});
const Lathe shoe({{0,-.022f},{.346f,-.022f},{.361f,-.012f},{.365f,.005f},{.365f,.05f},{.359f,.066f},{.34f,.075f},{0,.075f}});
const float rank[]={0,.8f,.95f,.92f,.95f,.98f,1};
// The horse's head is laid out along its own axis: across, above the nose line, and down it from the poll.
const Frame skull({0,1.0f,-.03f},{0,.762f,-.648f},{0,-.648f,-.762f});

float foot(P p,float scale) { return sole.distance(hypot2(p.x,p.z)/scale,p.y/scale)*scale; }
Field band(float height,float radius,float tube) { return [=](P p) { return hoop({p.x,p.y-height,p.z},radius,tube); }; }

Field pawn() {
  const Lathe stem({{0,.12f},{.19f,.135f},{.172f,.168f},{.138f,.215f},{.108f,.29f},{.09f,.38f},{.082f,.46f},{0,.47f}});
  return [=](P p) {
    float d=smin(foot(p,.8f),stem.distance(hypot2(p.x,p.z),p.y),.012f);
    d=smin(d,disc(p,.487f,.168f,.021f,.019f),.006f);
    d=smin(d,horn(p,{0,.5f,0},{0,.57f,0},.088f,.078f),.01f);
    return smin(d,ball(p,{0,.668f,0},.158f),.012f);
  };
}

// The horse: an arched neck over a deep chest, the head tucked in, and a mane of overlapping locks.
Field knight() {
  const Frame forelock({0,1.095f,-.085f},{0,-.648f,-.762f},{1,0,0});
  // Each lock of the mane: its height and depth, the lean of its length, and its size.
  const float fall[7][5]={{1.09f,.06f,-.55f,.83f,.8f},{1.0f,.13f,-.75f,.66f,.9f},{.885f,.182f,-.9f,.44f,1},{.76f,.218f,-.96f,.28f,1},
                          {.63f,.236f,-.98f,.2f,1},{.5f,.234f,-.99f,.14f,1},{.375f,.228f,-.995f,.1f,.95f}};
  std::vector<std::pair<Frame,float>> locks;
  for(const auto &lock:fall) locks.push_back({Frame({0,lock[0],lock[1]},{0,lock[2],lock[3]},{1,0,0}),lock[4]});
  return [=](P p) {
    const float d=smin(foot(p,.95f),disc(p,.215f,.258f,.032f,.022f),.01f);
    if(p.y<.12f) return d;
    const P m{std::abs(p.x),p.y,p.z};
    float horse=horn(narrow(p,.8f),{0,.34f,0},{0,.72f,.07f},.215f,.16f)*.8f;
    horse=smin(horse,horn(narrow(p,.72f),{0,.72f,.07f},{0,.98f,.01f},.16f,.135f)*.72f,.04f);
    horse=smin(horse,egg(p,{0,.42f,-.105f},{.165f,.17f,.15f}),.05f);
    if(p.y>.6f && p.z<.25f) {
      float head=horn(narrow(p,.74f),{0,1.0f,-.03f},{0,.8f,-.265f},.125f,.068f)*.74f;
      head=smin(head,egg(p,{0,.9f,-.07f},{.1f,.1f,.105f}),.03f);
      head=smin(head,egg(p,{0,.782f,-.272f},{.058f,.06f,.062f}),.02f);
      head=smin(head,egg(forelock.local(p),{0,0,0},{.03f,.068f,.042f}),.012f);
      // Pricked ears, hollowed toward the front.
      const float ear=smax(horn(m,{.058f,1.075f,.01f},{.074f,1.24f,-.015f},.042f,.013f),-egg(m,{.068f,1.16f,-.036f},{.02f,.055f,.02f}),.006f);
      head=smin(head,ear,.015f);
      // The mouth is a cut along the muzzle, with a nostril flared above it.
      const P q=skull.local(p), s{std::abs(q.x),q.y,q.z};
      head=smax(head,-egg(s,{0,-.04f,.37f},{.2f,.013f,.07f}),.004f);
      head=smax(head,-ball(s,{.04f,.005f,.365f},.017f),.004f);
      horse=smin(horse,head,.035f);
    }
    if(p.z>-.05f) {
      float mane=1e9f;
      for(const auto &lock:locks) mane=smin(mane,egg(lock.first.local(p),{0,0,0},{.042f*lock.second,.095f*lock.second,.055f*lock.second}),.006f);
      horse=smin(horse,mane,.012f);
    }
    return smin(d,horse,.03f);
  };
}

Field bishop() {
  const Lathe stem({{0,.14f},{.225f,.155f},{.205f,.195f},{.168f,.25f},{.135f,.34f},{.112f,.47f},{.098f,.62f},{.092f,.74f},{.094f,.82f},{0,.83f}});
  const Lathe mitre({{0,.86f},{.1f,.87f},{.128f,.915f},{.155f,.985f},{.166f,1.055f},{.158f,1.13f},{.128f,1.215f},{.08f,1.29f},{.035f,1.332f},{0,1.342f}});
  return [=](P p) {
    const float rho=hypot2(p.x,p.z);
    float d=smin(foot(p,.92f),stem.distance(rho,p.y),.012f);
    d=smin(d,disc(p,.838f,.188f,.021f,.019f),.006f);
    float head=smin(mitre.distance(rho,p.y),ball(p,{0,1.372f,0},.043f),.008f);
    // The mitre's cut climbs from its middle to one shoulder, open to the front and the back.
    const float across=-.819f*(p.x-.01f)+.574f*(p.y-1.09f), along=.574f*(p.x-.01f)+.819f*(p.y-1.09f);
    head=smax(head,-std::max(std::abs(across)-.017f,-.03f-along),.004f);
    return smin(d,head,.006f);
  };
}

Field rook() {
  const Lathe tower({{0,.15f},{.245f,.165f},{.228f,.205f},{.204f,.265f},{.188f,.36f},{.18f,.5f},{.184f,.62f},{.2f,.7f},{.214f,.735f},{0,.74f}});
  const Lathe turret({{0,.77f},{.2f,.775f},{.238f,.8f},{.262f,.83f},{.268f,.87f},{.268f,.985f},{.26f,1.0f},{.25f,1.005f},{0,1.005f}});
  return [=](P p) {
    const float rho=hypot2(p.x,p.z);
    float d=smin(foot(p,.95f),tower.distance(rho,p.y),.012f);
    d=smin(d,disc(p,.757f,.247f,.022f,.02f),.006f);
    d=smin(d,turret.distance(rho,p.y),.006f);
    // The turret is hollowed, and six crenels are sawn through its wall.
    d=smax(d,-disc(p,1.03f,.19f,.1f,.012f),.008f);
    const P q=wedge(p,6,pi/6);
    return smax(d,-brick({q.x,q.y-1.0f,q.z+.25f},{.048f,.058f,.12f},.008f),.006f);
  };
}

Field queen() {
  const Lathe stem({{0,.15f},{.245f,.168f},{.222f,.21f},{.18f,.27f},{.146f,.37f},{.124f,.52f},{.11f,.7f},{.102f,.86f},{.1f,.96f},{0,.97f}});
  const Lathe cup({{0,1.0f},{.108f,1.01f},{.117f,1.06f},{.13f,1.15f},{.158f,1.25f},{.192f,1.335f},{.214f,1.39f},{.222f,1.425f},{.212f,1.445f},{.185f,1.43f},{.15f,1.435f},{.105f,1.462f},{.05f,1.482f},{0,1.486f}});
  return [=](P p) {
    const float rho=hypot2(p.x,p.z);
    float d=smin(foot(p,.98f),stem.distance(rho,p.y),.012f);
    d=smin(d,disc(p,.985f,.2f,.022f,.02f),.006f);
    // The coronet: the rim scooped between eight points with a pearl on each, around a domed cap and its ball.
    float head=smax(cup.distance(rho,p.y),-ball(wedge(p,8,pi/8),{0,1.472f,-.228f},.056f),.008f);
    head=smin(head,ball(wedge(p,8,0),{0,1.462f,-.207f},.033f),.006f);
    head=smin(head,horn(p,{0,1.47f,0},{0,1.5f,0},.045f,.032f),.008f);
    head=smin(head,ball(p,{0,1.54f,0},.05f),.006f);
    return smin(d,head,.006f);
  };
}

Field king() {
  const Lathe stem({{0,.15f},{.25f,.17f},{.228f,.212f},{.186f,.275f},{.15f,.38f},{.128f,.54f},{.113f,.74f},{.104f,.92f},{.102f,1.03f},{0,1.04f}});
  const Lathe cup({{0,1.07f},{.112f,1.08f},{.12f,1.13f},{.134f,1.22f},{.165f,1.32f},{.2f,1.4f},{.221f,1.45f},{.226f,1.48f},{.218f,1.5f},{.19f,1.512f},{.12f,1.54f},{.05f,1.556f},{0,1.56f}});
  return [=](P p) {
    const float rho=hypot2(p.x,p.z);
    float d=smin(foot(p,1),stem.distance(rho,p.y),.012f);
    d=smin(d,disc(p,1.052f,.205f,.023f,.021f),.006f);
    return smin(d,cup.distance(rho,p.y),.006f);
  };
}
}

std::vector<Mould> Atelier::moulds(int kind,int part) {
  if(kind<1 || kind>6) return {};
  if(part==0) {
    const Field wood=kind==1 ? pawn() : kind==2 ? knight() : kind==3 ? bishop() : kind==4 ? rook() : kind==5 ? queen() : king();
    return {{wood,kind==2 ? .014f : kind==3 ? .015f : kind==1 ? .016f : .017f,.45f}};
  }
  if(part==1) {
    const float scale=rank[kind];
    std::vector<Mould> brass{{[=](P p) { return shoe.distance(hypot2(p.x,p.z)/scale,p.y/scale)*scale; },.011f,.3f}};
    // A ring rests on the collar beneath each crowned head, and the king carries his cross.
    if(kind==3) brass.push_back({band(.876f,.13f,.026f),.009f,.3f});
    if(kind==5) brass.push_back({band(1.03f,.136f,.027f),.009f,.3f});
    if(kind==6) {
      brass.push_back({band(1.1f,.14f,.028f),.009f,.3f});
      brass.push_back({+[](P p) { return smin(pattee(p,{0,1.685f,0},.095f,.024f,.05f,.024f),horn(p,{0,1.55f,0},{0,1.61f,0},.042f,.03f),.008f); },.008f,.3f});
    }
    return brass;
  }
  // The knight's eyes are beads of ebony set either side of the skull.
  if(part==2 && kind==2) return {{+[](P p) { const P q=skull.local(p); return egg({std::abs(q.x),q.y,q.z},{.073f,.05f,.056f},{.017f,.018f,.027f}); },.006f,.15f}};
  return {};
}
