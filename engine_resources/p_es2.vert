uniform mat4 mvpMatrix;

attribute vec2 vertex;
attribute vec2 tex_coord0;

varying vec2 tex0;

void main(void) {
  gl_Position = mvpMatrix * vec4(vertex.xy, 0, 1);
  tex0 = tex_coord0;
}
