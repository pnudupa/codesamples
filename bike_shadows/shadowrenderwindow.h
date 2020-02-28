#ifndef SHADOWRENDERER_H
#define SHADOWRENDERER_H

#include "simplerenderwindow.h"

class ShadowRenderWindow : public SimpleRenderWindow
{
public:
    ShadowRenderWindow(QWidget *parent=nullptr);
    ~ShadowRenderWindow();

protected:
    void paintGL();

    void renderToShadowMap();
    void initDepthMap();

private:
    uint m_shadowMapFBO;
    uint m_shadowMapTex;
};

#endif // SHADOWRENDERER_H
