#include "objmodel.h"
#include <QFile>
#include <QFileInfo>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

class SceneRenderer : public QOpenGLFunctions
{
public:
    SceneRenderer() : m_shader(nullptr),
        m_initialized(false) { m_padding[0] = 0; }
    ~SceneRenderer() {
        delete m_shader;
    }

    void render(ObjModel *model, const QVector3D &eyePosition, const QVector3D &lightPosition,
                const QMatrix4x4 &projectionMatrix, const QMatrix4x4 &viewMatrix,
                const QMatrix4x4 &lightViewMatrix=QMatrix4x4());

private:
    QOpenGLShaderProgram *m_shader;
    bool m_initialized;
    char m_padding[7];
};

class ShadowRenderer : public QOpenGLFunctions
{
public:
    ShadowRenderer() : m_shader(nullptr),
        m_initialized(false) { m_padding[0] = 0; }
    ~ShadowRenderer() {
        delete m_shader;
    }

    void render(ObjModel *model, const QMatrix4x4 &projectionMatrix, const QMatrix4x4 &lightViewMatrix);

private:
    QOpenGLShaderProgram *m_shader;
    bool m_initialized;
    bool m_padding[7];
};

Q_GLOBAL_STATIC(SceneRenderer, sceneRenderer)
Q_GLOBAL_STATIC(ShadowRenderer, shadowRenderer)

void ObjModel::render(const QVector3D &eyePosition,
                      const QVector3D &lightDirection,
                      const QMatrix4x4 &projectionMatrix,
                      const QMatrix4x4 &viewMatrix,
                      const QMatrix4x4 &lightViewMatrix)
{
    if(m_vertexBuffer && m_indexBuffer && !m_parts.isEmpty())
    {
        if(m_renderMode == SceneMode)
            ::sceneRenderer->render(this, eyePosition, lightDirection, projectionMatrix, viewMatrix, lightViewMatrix);
        else if(m_renderMode == ShadowMode)
            ::shadowRenderer->render(this, projectionMatrix, viewMatrix);
    }
}

typedef QMap< QString,QVector<float> > MaterialProps;
QMap<QString,MaterialProps> LoadMaterials(const QString &mtlFileName);

void ObjModel::load(const QString &fileName)
{
    struct
    {
        QVector<QVector3D> geometry;
        QVector<QVector3D> normals;
    } compressed, uncompressed;
    QVector<int> indexes;
    QMap<QString,MaterialProps> materials;
    Part currentPart;

    /*
     * Description of OBJ file format is available on Wikipedia
     * https://en.wikipedia.org/wiki/Wavefront_.obj_file
     */
    QFile file(fileName);
    if( !file.open(QFile::ReadOnly) )
        return;

    while(!file.atEnd())
    {
        const QString line = file.readLine().simplified();
        if( line.startsWith("#") )
            continue;

        const QStringList fields = line.split(" ", QString::SkipEmptyParts);
        if( fields.isEmpty() )
            continue;

        const QString type = fields.first();

        if(type == "mtllib")
        {
            const QString mtllib = fields.last();
            const QString path = QFileInfo(fileName).absolutePath();
            materials.unite( ::LoadMaterials(path + mtllib) );
            continue;
        }

        if(type == "o")
        {
            if(currentPart.isValid())
                m_parts << currentPart;

            currentPart = Part();
            currentPart.type = GL_TRIANGLES;
            continue;
        }

        if(type == "usemtl")
        {
            const MaterialProps props = materials.value( fields.last() );

            const QVector<float> diffuse = props.value("Kd");
            if(diffuse.size() == 3)
            {
                currentPart.material.color.diffuse.setRgbF( qreal(diffuse[0]), qreal(diffuse[1]), qreal(diffuse[2]) );
                currentPart.material.intensity.diffuse = 1.0;
            }

            const QVector<float> ambient = props.value("Ka");
            if(ambient.size() == 3)
            {
                currentPart.material.color.ambient.setRgbF( qreal(ambient[0]), qreal(ambient[1]), qreal(ambient[2]) );
                currentPart.material.intensity.ambient = 1.0;
            }

            const QVector<float> specular = props.value("Ks");
            if(specular.size() == 3)
                currentPart.material.color.specular.setRgbF( qreal(specular[0]), qreal(specular[1]), qreal(specular[2]) );

             currentPart.material.intensity.specular = 3.0f * props.value("Ns").first() / 1000.0f;

            if(props.contains("d"))
                currentPart.material.opacity = props.value("d").first();
            else if(props.contains("Tr"))
                currentPart.material.opacity = 1.0f - props.value("Tr").first();

            if(props.contains("illum"))
                currentPart.material.brightness = props.value("illum").first();
            else
                currentPart.material.brightness = 1.0f;

            continue;
        }

        if( type == "v" || type == "vn" )
        {
            const QVector3D v( fields[1].toFloat(), fields[2].toFloat(), fields[3].toFloat() );
            if(type == "v")
            {
                compressed.geometry.append(v);
                if(compressed.geometry.size() == 1)
                {
                    m_boundingBox.x.min = v.x();
                    m_boundingBox.x.max = v.x();

                    m_boundingBox.y.min = v.y();
                    m_boundingBox.y.max = v.y();

                    m_boundingBox.z.min = v.z();
                    m_boundingBox.z.max = v.z();
                }
                else
                {
                    m_boundingBox.x.min = qMin(v.x(), m_boundingBox.x.min);
                    m_boundingBox.x.max = qMax(v.x(), m_boundingBox.x.max);

                    m_boundingBox.y.min = qMin(v.y(), m_boundingBox.y.min);
                    m_boundingBox.y.max = qMax(v.y(), m_boundingBox.y.max);

                    m_boundingBox.z.min = qMin(v.z(), m_boundingBox.z.min);
                    m_boundingBox.z.max = qMax(v.z(), m_boundingBox.z.max);
                }
            }
            else
            {
                const QVector3D n = v.normalized();
                compressed.normals.append(n);
            }

            continue;
        }

        if(type == "f")
        {
            // Lets ignore non-triangles for now
            if(fields.size() != 4)
                continue;

            const int a = fields[1].split("/").first().toInt() - 1;
            const int b = fields[2].split("/").first().toInt() - 1;
            const int c = fields[3].split("/").first().toInt() - 1;

            const int na = fields[1].split("/").last().toInt() - 1;
            const int nb = fields[2].split("/").last().toInt() - 1;
            const int nc = fields[3].split("/").last().toInt() - 1;

            const int cgs = compressed.geometry.size();
            if(a >= cgs || b >= cgs || c >= cgs)
            {
                qDebug() << "Geometry: " << a << b << c << cgs;
                continue;
            }

            const int cns = compressed.normals.size();
            if(na >= cns || nb >= cns || nc >= cns)
            {
                qDebug() << "Normals: " << na << nb << nc << cns;
                continue;
            }

            if(currentPart.start < 0)
                currentPart.start = indexes.length();

            const int i = uncompressed.geometry.size();
            uncompressed.geometry << compressed.geometry.at(a)
                                  << compressed.geometry.at(b)
                                  << compressed.geometry.at(c);
            uncompressed.normals << compressed.normals.at(na)
                                  << compressed.normals.at(nb)
                                  << compressed.normals.at(nc);
            indexes << i << i+1 << i+2;
            currentPart.length = (indexes.length() - currentPart.start);
            continue;
        }
    }

    if(currentPart.isValid())
        m_parts << currentPart;

    std::sort(m_parts.begin(), m_parts.end());

    const QVector<QVector3D> vertices = uncompressed.geometry + uncompressed.normals;
    m_normalOffset = uncompressed.geometry.size()*int(sizeof(QVector3D));
    m_vertexBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    m_vertexBuffer->create();
    m_vertexBuffer->bind();
    m_vertexBuffer->allocate(
            static_cast<const void*>(vertices.constData()),
            vertices.size()*int(sizeof(QVector3D))
        );
    m_vertexBuffer->release();

    m_indexBuffer = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    m_indexBuffer->create();
    m_indexBuffer->bind();
    m_indexBuffer->allocate(
              static_cast<const void*>(indexes.constData()),
              indexes.size()*int(sizeof(int))
        );
    m_indexBuffer->release();
}

QMap<QString,MaterialProps> LoadMaterials(const QString &mtlFileName)
{
    QMap<QString,MaterialProps> ret;

    QFile file(mtlFileName);
    if( !file.open(QFile::ReadOnly) )
        return ret;

    QString materialName;
    MaterialProps materialProps;
    while(!file.atEnd())
    {
        const QString line = file.readLine().simplified();
        if(line.isEmpty())
            continue;

        if(line.startsWith("#"))
            continue;

        const QStringList fields = line.split(" ");
        if(fields.isEmpty())
            continue;

        const QString type = fields.first();
        if(type == "newmtl")
        {
            if(!materialProps.isEmpty())
            {
                ret[materialName] = materialProps;
                materialProps = MaterialProps();
            }

            materialName = fields.last();

            continue;
        }

        QVector<float> floats;
        for(int i=1; i<fields.size(); i++)
            floats.append( fields.at(i).toFloat() );
        materialProps[type] = floats;
    }

    if(!materialProps.isEmpty())
        ret[materialName] = materialProps;

    return ret;
}

///////////////////////////////////////////////////////////////////////////////

void SceneRenderer::render(ObjModel *model,
                              const QVector3D &eyePosition,
                              const QVector3D &lightDirection,
                              const QMatrix4x4 &projectionMatrix,
                              const QMatrix4x4 &viewMatrix,
                              const QMatrix4x4 &lightViewMatrix
                              )
{
    if(!m_initialized)
    {
        QOpenGLFunctions::initializeOpenGLFunctions();

        m_shader = new QOpenGLShaderProgram;
        m_shader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/scene_vertex.glsl");
        m_shader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/scene_fragment.glsl");
        m_shader->link();

        m_initialized = true;
    }

    m_shader->bind();
    model->m_vertexBuffer->bind();
    model->m_indexBuffer->bind();

    const QMatrix4x4 modelMatrix = model->m_sceneMatrix * model->m_matrix;
    const QMatrix4x4 modelViewMatrix = (viewMatrix * modelMatrix);
    const QMatrix4x4 modelViewProjectionMatrix = projectionMatrix * modelViewMatrix;
    const QMatrix4x4 normalMatrix = modelMatrix.inverted().transposed();

    m_shader->enableAttributeArray("qt_Vertex");
    m_shader->setAttributeBuffer("qt_Vertex", GL_FLOAT, 0, 3, 0);

    m_shader->enableAttributeArray("qt_Normal");
    m_shader->setAttributeBuffer("qt_Normal", GL_FLOAT, model->m_normalOffset, 3, 0);

    m_shader->setUniformValue("qt_ViewMatrix", viewMatrix);
    m_shader->setUniformValue("qt_NormalMatrix", normalMatrix);
    m_shader->setUniformValue("qt_ModelMatrix", modelMatrix);
    m_shader->setUniformValue("qt_ModelViewMatrix", modelViewMatrix);
    m_shader->setUniformValue("qt_ProjectionMatrix", projectionMatrix);
    m_shader->setUniformValue("qt_ModelViewProjectionMatrix", modelViewProjectionMatrix);

    m_shader->setUniformValue("qt_LightMatrix", lightViewMatrix);
    m_shader->setUniformValue("qt_LightViewMatrix", lightViewMatrix * modelMatrix);
    m_shader->setUniformValue("qt_LightViewProjectionMatrix", projectionMatrix * lightViewMatrix * modelMatrix);

    if(model->m_shadowTextureId > 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model->m_shadowTextureId);
        m_shader->setUniformValue("qt_ShadowMap", 0);
        m_shader->setUniformValue("qt_ShadowEnabled", true);
    }
    else
        m_shader->setUniformValue("qt_ShadowEnabled", false);

    m_shader->setUniformValue("qt_Light.ambient", QColor(40,40,40));
    m_shader->setUniformValue("qt_Light.diffuse", QColor(Qt::white));
    m_shader->setUniformValue("qt_Light.specular", QColor(Qt::white));
    m_shader->setUniformValue("qt_Light.direction", lightDirection);
    m_shader->setUniformValue("qt_Light.eye", eyePosition);

    Q_FOREACH(ObjModel::Part part, model->m_parts)
    {
        QColor ambient = part.material.color.ambient;
        ambient.setRgbF(
                ambient.redF()*qreal(part.material.intensity.ambient),
                ambient.greenF()*qreal(part.material.intensity.ambient),
                ambient.blueF()*qreal(part.material.intensity.ambient)
            );

        QColor diffuse = part.material.color.diffuse;
        diffuse.setRgbF(
                diffuse.redF()*qreal(part.material.intensity.diffuse),
                diffuse.greenF()*qreal(part.material.intensity.diffuse),
                diffuse.blueF()*qreal(part.material.intensity.diffuse)
            );

        QColor specular = part.material.color.ambient;

        m_shader->setUniformValue("qt_Material.ambient", ambient);
        m_shader->setUniformValue("qt_Material.diffuse", diffuse);
        m_shader->setUniformValue("qt_Material.specular", specular);
        m_shader->setUniformValue("qt_Material.specularPower", part.material.intensity.specular);
        m_shader->setUniformValue("qt_Material.brightness", part.material.brightness);
        m_shader->setUniformValue("qt_Material.opacity", part.material.opacity);

        const int offset = part.start * int(sizeof(int));
        glDrawElements(GLenum(part.type), part.length, GL_UNSIGNED_INT, (void*)offset);
    }

    if(model->m_shadowTextureId > 0)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    model->m_indexBuffer->release();
    model->m_vertexBuffer->release();
    m_shader->release();
}

///////////////////////////////////////////////////////////////////////////////

void ShadowRenderer::render(ObjModel *model,
                            const QMatrix4x4 &projectionMatrix,
                            const QMatrix4x4 &lightViewMatrix
                            )
{
    if(!m_initialized)
    {
        QOpenGLFunctions::initializeOpenGLFunctions();

        m_shader = new QOpenGLShaderProgram;
        m_shader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/shadow_vertex.glsl");
        m_shader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/shadow_fragment.glsl");
        m_shader->link();

        m_initialized = true;
    }

    m_shader->bind();
    model->m_vertexBuffer->bind();
    model->m_indexBuffer->bind();

    const QMatrix4x4 modelMatrix = model->m_sceneMatrix * model->m_matrix;
    const QMatrix4x4 lightViewProjectionMatrix = projectionMatrix * lightViewMatrix * modelMatrix;

    m_shader->enableAttributeArray("qt_Vertex");
    m_shader->setAttributeBuffer("qt_Vertex", GL_FLOAT, 0, 3, 0);

    m_shader->setUniformValue("qt_LightViewProjectionMatrix", lightViewProjectionMatrix);

    Q_FOREACH(ObjModel::Part part, model->m_parts)
    {
        const int offset = part.start * int(sizeof(int));
        glDrawElements(GLenum(part.type), part.length, GL_UNSIGNED_INT, (void*)offset);
    }

    model->m_indexBuffer->release();
    model->m_vertexBuffer->release();
    m_shader->release();
}
