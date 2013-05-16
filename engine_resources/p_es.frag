#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D textureUnit0;
uniform vec2 tex_bl;
uniform vec2 tex_tr;

void main(void) {
  vec2 scale = (tex_tr - tex_bl);
  vec2 coord = tex_bl + (gl_PointCoord * scale);
  gl_FragColor = texture2D(textureUnit0, coord); //vec4(1.0, 0.0, 1.0, 1.0);
}
