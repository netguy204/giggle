#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D textureUnit0;
varying vec2 tex0;
varying vec4 color0;

void main(void) {
  gl_FragColor = texture2D(textureUnit0, tex0) * color0;
}
