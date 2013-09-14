#ifdef GL_ES
precision mediump float;
#endif

varying vec4 vcolor;

void main(void) {
  gl_FragColor = vcolor;
}
