uniform mat4 mvpMatrix;
uniform float scale;

attribute vec2 vertex;

void main(void) {
  gl_Position = mvpMatrix * vec4(vertex.xy, 0, 1);
  gl_PointSize = scale;
}
