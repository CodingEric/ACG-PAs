#version 450

layout(location = 0) out vec2 tex_coord;

// Define a constant rectangle [0, 1]*[0, 1]
const vec2[4] rectangle =
    vec2[4](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(0.0, 1.0), vec2(1.0, 1.0));

void main() {
  // Compute the index of the vertex
  uint index = gl_VertexIndex % 4;

  // Compute the texture coordinate
  tex_coord = rectangle[index];

  // Compute the position of the vertex
  gl_Position = vec4(2.0 * tex_coord - 1.0, 0.0, 1.0);
}
