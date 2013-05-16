#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D textureUnit0;
uniform vec3 light;

varying vec2 tex0;
varying vec2 pos;

vec3 hsv(float h,float s,float v) {
  return mix(vec3(1.),
             clamp((abs(fract(h+vec3(3.,2.,1.)/3.)*6.-3.)-1.),
                   0.,1.), s)*v;
}

const vec2 size = vec2(2.0,0.0);
const vec3 off = vec3(-1.0/1024.0,0,1.0/1024.0);

vec4 bump_normal(sampler2D unit, vec2 tex_coord) {
  vec4 wave = texture2D(unit, tex_coord);
  float s11 = wave.a;
  float s01 = texture2D(unit, tex_coord + off.xy).a;
  float s21 = texture2D(unit, tex_coord + off.zy).a;
  float s10 = texture2D(unit, tex_coord + off.yx).a;
  float s12 = texture2D(unit, tex_coord + off.yz).a;
  vec3 va = normalize(vec3(size.x, s21-s01, size.y));
  vec3 vb = normalize(vec3(size.y, s12-s10, -size.x));
  return vec4( cross(va,vb), s11 );
}

void main(void) {
  vec3 color = vec3(1.0, 1.0, 1.0);

  vec3 to_pix = normalize(vec3(pos, 0) - light);

  vec4 samp = texture2D(textureUnit0, tex0);
  vec4 norm = bump_normal(textureUnit0, tex0);

  float ds = max(0.0, dot(norm.xyz, to_pix));
  vec3 frag = samp.rgb + light * ds;

  gl_FragColor = vec4(frag, 1.0);
}
