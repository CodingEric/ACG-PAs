#version 450
#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_debug_printf : enable

// clang-format off
#include "camera_object.glsl"
#include "hit_record.glsl"
#include "sphere.glsl"
#include "triangle.glsl"
#include "point_light.glsl"
// clang-format on

layout(location = 0) out vec4 outColor;

layout(std140, binding = 0) readonly uniform camera_object_struct {
  CameraObject camera_object;
};
layout(std140, binding = 1) readonly buffer triangle_struct {
  Triangle triangles[];
};
layout(std140, binding = 2) readonly buffer triangle_material_struct {
  Material triangle_materials[];
};
layout(std140, binding = 3) readonly buffer sphere_struct {
  Sphere spheres[];
};
layout(std140, binding = 4) readonly buffer sphere_material_struct {
  Material sphere_materials[];
};
layout(std140, binding = 5) readonly buffer point_light_struct {
  PointLight point_lights[];
};

bool intersectTriangleCheck(in vec3 origin, in vec3 direction)
{
  for(int i=0;i<camera_object.num_triangle;++i)
  {
      vec3 v0v1 = triangles[i].v1 - triangles[i].v0;
      vec3 v0v2 = triangles[i].v2 - triangles[i].v0;
      vec3 pvec = cross(direction, v0v2);
      float det = dot(v0v1, pvec);
      if(abs(det) < 1e-6) continue;
      float invDet = 1 / det;
      vec3 tvec = origin - triangles[i].v0;
      float u = dot(tvec, pvec) * invDet;
      if(u < 0 || u > 1) continue;
      vec3 qvec = cross(tvec, v0v1);
      float v = dot(direction, qvec) * invDet;
      if(v < 0 || u + v > 1) continue;
      float t = dot(v0v2, qvec) * invDet;
      if(t < 1e-3) continue;
      return true;
  }
  return false;
}

bool intersectSphereCheck(in vec3 origin, in vec3 direction)
{
  for(int i=0;i<camera_object.num_sphere;++i)
  {
      vec3 e = origin - spheres[i].origin;
      float b = dot(direction, e);
      float c = dot(e, e) - spheres[i].radius * spheres[i].radius;
      float k = b * b - c;
      float t = 0;
      if(k < 0){
        continue;
      }
      else if (k == 0)
      {
        float a = dot(direction, direction);
        t = -b / a;
      }else{
        float sqrtk = sqrt(k);
        float a = dot(direction, direction);
        t = min((-b-sqrtk)/a, (-b+sqrtk)/a);
      }
      if(t < 1e-3) continue;
      return true;
  }
  return false;
}

// incident points to the intersection point.
vec3 computeReflection(in vec3 incident, in vec3 normal) {
    return incident - 2.0 * dot(incident, normal) * normal;
}

vec3 SampleRay(vec3 origin, vec3 direction) {
  vec3 result_color = vec3(0);
  /*
   * TODO: This function should return the color of the sampling ray.
   * @origin denote the ray's starting position.
   * @direction is a normalized vector denote the marching direction of the ray.
   * The result should consider all the geometries, materials, point
   * lights (with shadow), and ambient light. You can limit the number of light
   * bounces to 16.
   */
  vec3 factor = vec3(1.0f);
  for(int it=0;it<16;++it)
  {
    // Begin scene intersection
    HitRecord hit;
    bool intersected = false;
    for(int i = 0;i<camera_object.num_triangle;++i)
    {
      vec3 v0v1 = triangles[i].v1 - triangles[i].v0;
      vec3 v0v2 = triangles[i].v2 - triangles[i].v0;
      vec3 pvec = cross(direction, v0v2);
      float det = dot(v0v1, pvec);
      if(abs(det) < 1e-6) continue;
      float invDet = 1 / det;
      vec3 tvec = origin - triangles[i].v0;
      float u = dot(tvec, pvec) * invDet;
      if(u < 0 || u > 1) continue;
      vec3 qvec = cross(tvec, v0v1);
      float v = dot(direction, qvec) * invDet;
      if(v < 0 || u + v > 1) continue;
      float t = dot(v0v2, qvec) * invDet;
      if(t < 1e-3) continue;
      // return vec3(1.0f, 0.0f, 0.0f);
      if(intersected == false){
        hit.t_min = t;
        hit.material = triangle_materials[i];
        hit.normal = normalize(cross(v0v1, v0v2));
        if(dot(hit.normal, direction) > 0)
        {
          hit.normal = -hit.normal;
        }
        intersected = true;
      }else{
        // hit.t_min = min(hit.t_min, t);
        if(t < hit.t_min)
        {
          hit.material = triangle_materials[i];
          hit.t_min = t;
          hit.normal = normalize(cross(v0v1, v0v2));
          if(dot(hit.normal, direction) > 0)
          {
            hit.normal = -hit.normal;
          }
        }
      }
    }
    for(int i = 0;i<camera_object.num_sphere;++i)
    {
      vec3 a = spheres[i].origin - origin;
      float l = dot(direction, a);
      float a2 = dot(a, a);
      float t = 0;
      if(a2>spheres[i].radius*spheres[i].radius && l < 0)
      {
        continue;
      }
      float m2 = a2 - l*l;
      float R2 = spheres[i].radius*spheres[i].radius;
      if(m2 > R2){
        continue;
      }
      float q = sqrt(R2 - m2);
      if(a2 > R2)
      {
        t = l - q;
      }else{
        t = l + q;
      }
      if(t <1e-3) continue;
      if(intersected == false){
        intersected = true;
        hit.t_min = t;
        hit.material = sphere_materials[i];
        hit.normal = normalize(origin + t * direction - spheres[i].origin);
        if(dot(hit.normal, direction) > 0)
        {
          hit.normal = -hit.normal;
        }
      }
      else{
        // hit.t_min = min(hit.t_min, t);
        if(t < hit.t_min){
          hit.t_min = t;
          hit.material = sphere_materials[i];
          hit.normal = normalize(origin + t * direction - spheres[i].origin);
          if(dot(hit.normal, direction) > 0)
          {
            hit.normal = -hit.normal;
          }
        }
      }
    }
    // End scene intersection
    if(!intersected)
    {
      break;
    }
    vec3 hit_position = origin + hit.t_min * direction;
    if(hit.material.material_type == MaterialDiffuse)
    {
      for(int i = 0;i<camera_object.num_point_light;++i)
      {
        vec3 test_ray_direction = normalize(point_lights[i].position - hit_position);
        if(!intersectTriangleCheck(hit_position, test_ray_direction)&&!intersectSphereCheck(hit_position, test_ray_direction))
        {
          result_color += hit.material.albedo_color * point_lights[i].power / dot(point_lights[i].position - hit_position, point_lights[i].position - hit_position) * max(0, dot(hit.normal, test_ray_direction));
        }
      }
      result_color += hit.material.albedo_color * camera_object.ambient_light;
      break;
    }
    else // MaterialSpecular
    {
      origin = hit_position;
      direction = computeReflection(direction, hit.normal);
      factor *= hit.material.albedo_color;
      continue;
    }
  }
  return factor * result_color;
}

void main() {
  vec3 color = vec3(0.0);
#define SUPER_SAMPLE_X 4
#define SUPER_SAMPLE_Y 4
  for (int x = 0; x < SUPER_SAMPLE_X; x++) {
    for (int y = 0; y < SUPER_SAMPLE_Y; y++) {
      vec2 pixel_offset = vec2((x + 0.5) * 1.0 / SUPER_SAMPLE_X,
                               (y + 0.5) * 1.0 / SUPER_SAMPLE_Y);
      vec4 direction =
          camera_object.projection * vec4(((gl_FragCoord.xy + pixel_offset) /
                                           camera_object.window_extent) *
                                                  2.0 -
                                              1.0,
                                          1.0, 1.0);
      vec3 subpixel_color = SampleRay(
          (camera_object.camera_to_world * vec4(0.0, 0.0, 0.0, 1.0)).xyz,
          (camera_object.camera_to_world * vec4(normalize(direction.xyz), 0.0))
              .xyz);
      subpixel_color = clamp(subpixel_color, 0.0, 1.0);
      color += subpixel_color;
    }
  }
  outColor = vec4(color / (SUPER_SAMPLE_X * SUPER_SAMPLE_Y), 1.0);
}
