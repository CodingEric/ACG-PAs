struct CameraObject {
  mat4 projection;
  mat4 camera_to_world;
  vec2 window_extent;
  uint num_triangle;
  uint num_sphere;
  vec3 ambient_light;
  uint num_point_light;
};
