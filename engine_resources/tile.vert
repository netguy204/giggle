uniform mat4 mvpMatrix;

attribute vec4 vertex;
attribute vec2 tex_coord0;

varying vec2 tex0;
varying vec2 pos;

void main(void) {
  gl_Position = mvpMatrix * vertex;
  tex0 = tex_coord0;
  pos = gl_Position.xy;
}
