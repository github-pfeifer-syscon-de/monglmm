
in vec3 position;
in vec3 color;
in vec3 normal;
uniform vec3 light;
uniform mat4 mvp;	/* model view perspective transform */
out vec4 vertexColor;
out vec4 gamma;
void main() {
  float x = dot(light, normal);   // normalize done with glm
  x = (x < 0.2) ? 0.2 : x;
  vec3 colorX = color * x;
  vertexColor = vec4(colorX, 1.0);
  gamma = vec4(1.0 / 2.2); // gamma 2.2 fixed here coud be uniform...
  gamma.w = 1.0;

  gl_Position = mvp * vec4(position, 1.0);
}
