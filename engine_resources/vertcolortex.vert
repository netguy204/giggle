uniform mat4 mvpMatrix;

attribute vec2 vertex;
attribute vec4 color;
attribute vec2 tex_coord0;

varying vec4 vcolor;
varying vec2 vtex;

void main(void) {
  gl_Position = mvpMatrix * vec4(vertex, 0, 1);
  vcolor = color;
  vtex = tex_coord0;
}
