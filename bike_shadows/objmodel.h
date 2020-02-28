#ifndef OBJ_MODEL_H
#define OBJ_MODEL_H

#include <QColor>
#include <QMatrix4x4>
#include <QVector>
#include <QVector3D>
#include <QOpenGLBuffer>

class SceneRenderer;
class ShadowRenderer;

typedef struct _BoundingBox {
    _BoundingBox() {
        x.min = 0; x.max = 0;
        y.min = 0; y.max = 0;
        z.min = 0; z.max = 0;
    }
    struct { float min, max; } x, y, z;
    void unite(const _BoundingBox &other) {
        x.min = qMin(x.min, other.x.min);
        y.min = qMin(y.min, other.y.min);
        z.min = qMin(z.min, other.z.min);
        x.max = qMax(x.max, other.x.max);
        y.max = qMax(y.max, other.y.max);
        z.max = qMax(z.max, other.z.max);
    }
    _BoundingBox & operator |= (const _BoundingBox &other) {
        this->unite(other);
        return *this;
    }

    QVector3D center() const {
        return QVector3D( (x.min+x.max)/2.0f, (y.min+y.max)/2.0f, (z.min+z.max)/2.0f );
    }
    float width() const { return x.max-x.min; }
    float height() const { return y.max-y.min; }
    float depth() const { return z.max-z.min; }

} BoundingBox;

class ObjModel
{
public:
    ObjModel(const QString &fileName)
        : m_vertexBuffer(nullptr), m_indexBuffer(nullptr),
          m_normalOffset(0), m_renderMode(SceneMode),
          m_shadowTextureId(0) {
        this->load(fileName);
    }
    ~ObjModel() {
        delete m_indexBuffer;
        delete m_vertexBuffer;
    }

    BoundingBox boundingBox() const { return m_boundingBox; }

    void setSceneMatrix(const QMatrix4x4 &matrix) { m_sceneMatrix = matrix; }
    QMatrix4x4 sceneMatrix() const { return m_sceneMatrix; }

    const QMatrix4x4 &matrix() const { return m_matrix; }
    QMatrix4x4 &matrix() { return m_matrix; }

    ObjModel &translate(float x, float y, float z) {
        m_matrix.translate(x, y, z);
        return *this;
    }
    ObjModel &rotate(float angle, float x, float y, float z) {
        m_matrix.rotate(angle, x, y, z);
        return *this;
    }
    ObjModel &scale(float s) {
        m_matrix.scale(s);
        return *this;
    }
    ObjModel &scale(float x, float y, float z) {
        m_matrix.scale(x, y, z);
        return *this;
    }

    enum RenderMode { ShadowMode, SceneMode };
    void setRenderMode(RenderMode mode) {
        m_renderMode = mode;
    }
    RenderMode renderMode() const { return m_renderMode; }

    void setShadowTextureId(const uint &val) {
        m_shadowTextureId = val;
    }
    uint shadowTextureId() const { return m_shadowTextureId; }

    void render(const QVector3D &eyePosition, const QVector3D &lightDirection,
                const QMatrix4x4 &projectionMatrix, const QMatrix4x4 &viewMatrix,
                const QMatrix4x4 &lightViewMatrix=QMatrix4x4());
    void render(const QMatrix4x4 &projection, const QMatrix4x4 &view) {
        this->render( QVector3D(0,0,-1), QVector3D(1,1,1), projection, view );
    }
    void render() {
        this->render( QVector3D(0,0,-1), QVector3D(1,1,1), QMatrix4x4(), QMatrix4x4() );
    }

private:
    void load(const QString &fileName);

private:
    friend class SceneRenderer;
    friend class ShadowRenderer;

    QOpenGLBuffer *m_vertexBuffer;
    QOpenGLBuffer *m_indexBuffer;
    int m_normalOffset;
    struct Part
    {
        Part() : type(0), start(-1), length(0) {
            material.reset();
        }
        int type, start, length;
        struct
        {
            struct { QColor ambient, diffuse, specular; } color;
            struct { float ambient, diffuse, specular; } intensity;
            float brightness, opacity;
            void reset() {
                color.ambient = Qt::white;
                color.diffuse = Qt::white;
                color.diffuse = Qt::white;
                intensity.ambient = 0.1f;
                intensity.diffuse = 1.0f;
                intensity.specular = 0.0f;
                brightness = 1.0f;
                opacity = 1.0f;
            }
        } material;

        bool isValid() const { return start >= 0 && length >= 0 && type != 0; }
        bool operator < (const Part &other) const {
            return material.opacity > other.material.opacity;
        }
    };
    QList<Part> m_parts;
    QMatrix4x4 m_matrix;
    QMatrix4x4 m_sceneMatrix;
    BoundingBox m_boundingBox;
    RenderMode m_renderMode;
    uint m_shadowTextureId;
};

#endif // OBJ_MODEL_H
