uniform mat4 mvpMatrix;

attribute vec2 vertex;
attribute vec4 color;

varying vec4 vcolor;

void main(void) {
  gl_Position = mvpMatrix * vec4(vertex, 0, 1);
  vcolor = color;
}
