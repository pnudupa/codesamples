attribute vec3 qt_Vertex;
uniform mat4 qt_LightViewProjectionMatrix;
const float c_one = 1.0;

void main(void)
{
    gl_Position = qt_LightViewProjectionMatrix * vec4(qt_Vertex, c_one);
}

