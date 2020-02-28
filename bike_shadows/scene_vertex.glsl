attribute vec4 qt_Vertex;
attribute vec4 qt_Normal;

uniform mat4 qt_NormalMatrix;
uniform mat4 qt_LightViewProjectionMatrix;
uniform mat4 qt_ModelViewProjectionMatrix;

varying vec4 v_Normal;
varying vec4 v_ShadowPosition;

void main(void)
{
    v_Normal = normalize(qt_NormalMatrix * qt_Normal);
    v_ShadowPosition = qt_LightViewProjectionMatrix * vec4(qt_Vertex.xyz, 1.0);

    gl_Position = qt_ModelViewProjectionMatrix * qt_Vertex;
}

