#include "surfacetexture.h"
#include <algorithm>
#include <QSize>
#include <cmath>
#include <cstring>

SurfaceTexture::SurfaceTexture(QQuick3DObject *parent) : QQuick3DTextureData(parent) { rebuild(); }
void SurfaceTexture::setStyle(int style) {
  if (m_style != style) { m_style = style; rebuild(); emit styleChanged(); }
}
void SurfaceTexture::rebuild() {
  const int width = 256, height = m_style == 4 ? 128 : 256;
  setSize({width,height});
  setHasTransparency(false);
  QByteArray data;
  if (m_style == 4) {
    setFormat(QQuick3DTextureData::RGBA32F);
    data.resize(width*height*4*sizeof(float));
    for (int y=0;y<height;++y) for (int x=0;x<width;++x) {
      float u=float(x)/width, v=float(y)/height;
      float softbox=3.5f*std::exp(-std::pow((u-.22f)/.07f,8)-std::pow((v-.32f)/.16f,8));
      float fill=1.5f*std::exp(-std::pow((u-.73f)/.13f,4)-std::pow((v-.4f)/.2f,4));
      float sky=.18f+.22f*(1-v);
      float pixel[]={sky+softbox+fill*.7f,sky+softbox*.94f+fill*.85f,sky+softbox*.84f+fill,1};
      std::memcpy(data.data()+(y*width+x)*sizeof(pixel),pixel,sizeof(pixel));
    }
  } else {
    setFormat(QQuick3DTextureData::RGBA8);
    data.resize(width*height*4);
    for (int y=0;y<height;++y) for (int x=0;x<width;++x) {
      float noise=std::sin(x*.73f+y*1.37f)*std::sin(x*2.27f-y*.37f);
      float wave=std::sin(x*.17f+3*std::sin(y*.018f)+.7f*std::sin(y*.051f));
      float value=.94f;
      if (m_style==1) value=.87f+.065f*wave+.035f*std::sin(x*.85f+wave*3)+noise*.015f;
      else if (m_style==2) value=.95f+.018f*std::sin(x*.04f+2*std::sin(y*.033f))+noise*.006f;
      else if (m_style==5) value=.94f-.06f*std::pow(.5f+.5f*std::sin(x*.035f+y*.025f+2*wave),12)+noise*.004f;
      else if (m_style==7) value=.9f+.04f*std::sin(x*.35f+std::sin(y*.023f))+noise*.012f;
      else if (m_style==0) value=.93f+noise*.02f;
      auto shade=char(std::clamp(int(value*255),0,255));
      const int offset=(y*width+x)*4;
      data[offset]=shade; data[offset+1]=shade; data[offset+2]=shade; data[offset+3]=char(255);
    }
  }
  setTextureData(data);
}
