#version 450

layout(location = 0) in vec4 color;
layout(location = 0) out vec4 color_out;

void main() {
  color_out = color;
}