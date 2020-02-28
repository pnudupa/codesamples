#include "simplerenderwindow.h"

#include <QLabel>

SimpleRenderWindow::SimpleRenderWindow(QWidget *parent)
    : QOpenGLWidget(parent)
{
    m_label = new QLabel(this);
    QFont font = m_label->font();
    font.setPixelSize(40);
    font.setBold(true);
    m_label->setFont(font);
    m_label->setBackgroundRole(QPalette::NoRole);
    m_label->setWordWrap(true);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setText("Rendering in perspective view - WITHOUT shadows");
}

SimpleRenderWindow::~SimpleRenderWindow()
{
    qDeleteAll(m_models);
    m_models.clear();
}

void SimpleRenderWindow::resizeEvent(QResizeEvent *e)
{
    QOpenGLWidget::resizeEvent(e);

    QFontMetrics fm( m_label->font() );

    QRectF labelRect = fm.boundingRect( m_label->text() );
    labelRect.setWidth( qMin(labelRect.width(), double(this->width()-20)) );
    labelRect.setHeight( labelRect.height()*2 );
    labelRect.moveCenter( this->rect().center() );
    labelRect.moveTop( this->rect().top() + 10 );

    m_label->setGeometry(labelRect.toRect());
}

void SimpleRenderWindow::initializeGL()
{
    QOpenGLFunctions::initializeOpenGLFunctions();

    glClearColor(0.25f, 0.45f, 0.65f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    glDepthFunc(GL_LEQUAL);
    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-0.03125f, -0.03125f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ObjModel *bike1 = new ObjModel(":/bike.obj");
    bike1->translate(-2.0, 0, 0);
    bike1->rotate(20, 0, 1, 0);

    ObjModel *bike2 = new ObjModel(":/bike.obj");
    bike2->translate(2.0, 0, 0);
    bike2->rotate(-20, 0, 1, 0);

    m_models << bike1 << bike2;
    m_models << new ObjModel(":/platform.obj");
}

void SimpleRenderWindow::resizeGL(int /*w*/, int /*h*/)
{
    this->updateMatricesForScreenRendering();
}

void SimpleRenderWindow::paintGL()
{
    this->renderToScreen();
}

void SimpleRenderWindow::renderToScreen()
{
    const int devicePixelRatio = this->devicePixelRatio();
    const int w = this->width() * devicePixelRatio;
    const int h = this->height() * devicePixelRatio;
    glViewport(0, 0, w, h);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    const QVector3D center = m_sceneBounds.center();
    const QVector3D eye(center.x(), center.y(), m_sceneBounds.z.max);
    const QVector3D lightDirection = m_lightPositionMatrix.map( QVector3D(0,0,-1) ).normalized();

    m_sceneMatrix.rotate(3, 0, 1, 0);

    for(int i=m_models.size()-1; i>=0; i--)
    {
        ObjModel *model = m_models.at(i);
        model->setRenderMode(ObjModel::SceneMode);
        model->render(eye, lightDirection, m_projectionMatrix, m_viewMatrix, m_lightViewMatrix);
        model->setSceneMatrix(m_sceneMatrix);
    }
}

void SimpleRenderWindow::updateMatricesForScreenRendering()
{
    if(m_models.isEmpty())
        return;

    m_sceneBounds = m_models.first()->boundingBox();
    for(int i=1; i<m_models.size()-1; i++)
        m_sceneBounds |= m_models.at(i)->boundingBox();

    const float width = m_sceneBounds.width();
    const float height = m_sceneBounds.height();
    const float depth = m_sceneBounds.depth();
    const float size = qMax( qMax(width, height), depth );
    const QVector3D center = m_sceneBounds.center();

    m_cameraPositionMatrix.setToIdentity();
    m_cameraPositionMatrix.translate(center.x(), center.y(), center.z());
    m_cameraPositionMatrix.rotate(20, 0, 1, 0);
    m_cameraPositionMatrix.rotate(-25, 1, 0, 0);

    m_lightPositionMatrix = m_cameraPositionMatrix;
    m_lightPositionMatrix.rotate(45, 0, 1, 0);

    m_lightPositionMatrix.translate(0, 0, size*5.0f);
    m_cameraPositionMatrix.translate(0, 0, size*1.5f);

    m_viewMatrix.setToIdentity();
    m_viewMatrix.lookAt( m_cameraPositionMatrix.map( QVector3D(0,0,0) ),
                         center,
                         m_cameraPositionMatrix.map( QVector3D(0,1,0) ).normalized() );

    const int w = this->width();
    const int h = this->height();
    m_projectionMatrix.setToIdentity();
    m_projectionMatrix.perspective(45.0, float(w)/float(h), 0.1f, 1000.0f);
}

