#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D textureUnit0;

varying vec4 vcolor;
varying vec2 vtex;

void main(void) {
  gl_FragColor = texture2D(textureUnit0, vtex) * vcolor;
}
