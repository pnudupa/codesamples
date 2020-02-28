#include "shadowrenderwindow.h"

#include <QLabel>
#include <QOpenGLTexture>
#include <QOpenGLFramebufferObject>

static const int SHADOW_WIDTH = 2048;
static const int SHADOW_HEIGHT = 2048;

ShadowRenderWindow::ShadowRenderWindow(QWidget *parent)
    : SimpleRenderWindow(parent), m_shadowMapFBO(0), m_shadowMapTex(0)
{
    m_label->setText("Rendering in perspective view - WITH shadows");
}

ShadowRenderWindow::~ShadowRenderWindow()
{
    if(m_shadowMapTex > 0)
        glDeleteTextures(1, &m_shadowMapTex);
    if(m_shadowMapFBO > 0)
        glDeleteFramebuffers(1, &m_shadowMapFBO);
}

void ShadowRenderWindow::paintGL()
{
    // PASS #1
    // Render all models into the shadow buffer first
    Q_FOREACH(ObjModel *model, m_models)
        model->setShadowTextureId(0);
    this->renderToShadowMap();

    // PASS #2
    // Render all models into the scene buffer next
    Q_FOREACH(ObjModel *model, m_models)
        model->setShadowTextureId(m_shadowMapTex);
    this->renderToScreen();
}

void ShadowRenderWindow::renderToShadowMap()
{
    this->initDepthMap(); // init happens only once.

    // Render into the depth framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
//    glBindTexture(GL_TEXTURE_2D, m_shadowMapTex);
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    m_lightViewMatrix.setToIdentity();
    m_lightViewMatrix.lookAt( m_lightPositionMatrix.map( QVector3D(0,0,0) ),
                 m_sceneBounds.center(),
                 m_lightPositionMatrix.map( QVector3D(0,1,0) ).normalized() );

    for(int i=0; i<m_models.size()-1; i++)
    {
        ObjModel *model = m_models.at(i);
        model->setRenderMode(ObjModel::ShadowMode);
        model->render(m_projectionMatrix, m_lightViewMatrix);
    }

//    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ShadowRenderWindow::initDepthMap()
{
    // Refer http://learnopengl.com/#!Advanced-Lighting/Shadows/Shadow-Mapping
    if(m_shadowMapFBO != 0)
        return;

    // Create a texture for storing the depth map
    glGenTextures(1, &m_shadowMapTex);
    glBindTexture(GL_TEXTURE_2D, m_shadowMapTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
                 SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // Create a frame-buffer and associate the texture with it.
    glGenFramebuffers(1, &m_shadowMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMapTex, 0);

    // Let OpenGL know that we are not interested in colors for this buffer
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Cleanup for now.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
