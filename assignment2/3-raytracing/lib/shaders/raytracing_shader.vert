#version 450

// Constant rect [0, 1]^2, 6 vertices
const vec2 rect[6] = vec2[](vec2(0.0, 0.0),
                            vec2(1.0, 0.0),
                            vec2(0.0, 1.0),
                            vec2(1.0, 0.0),
                            vec2(1.0, 1.0),
                            vec2(0.0, 1.0));

void main() {
  gl_Position = vec4(rect[gl_VertexIndex] * 2.0 - 1.0, 0.0, 1.0);
}
