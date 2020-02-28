#ifndef SIMPLERENDERER_H
#define SIMPLERENDERER_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include "objmodel.h"

class QLabel;

class SimpleRenderWindow : public QOpenGLWidget, public QOpenGLFunctions
{
public:
    SimpleRenderWindow(QWidget *parent=nullptr);
    ~SimpleRenderWindow();

protected:
    void keyPressEvent(QKeyEvent *) { this->update(); }
    void resizeEvent(QResizeEvent *e);

    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();

    void renderToScreen();
    void updateMatricesForScreenRendering();

protected:
    QList<ObjModel*> m_models;
    QMatrix4x4 m_sceneMatrix;
    QMatrix4x4 m_projectionMatrix;
    QMatrix4x4 m_viewMatrix;
    BoundingBox m_sceneBounds;
    QMatrix4x4 m_cameraPositionMatrix;
    QMatrix4x4 m_lightPositionMatrix;
    QMatrix4x4 m_lightViewMatrix;
    QLabel *m_label;
};

#endif // SIMPLERENDERER_H
