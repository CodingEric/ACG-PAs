#include "mesh.h"

#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <set>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

using namespace std;

namespace {
struct Edge {
  int u;
  int v;
  Edge(int _u, int _v) {
    if (_u > _v)
      std::swap(_u, _v);
    u = _u;
    v = _v;
  }
  bool operator<(const Edge &edge) const {
    if (u < edge.u)
      return true;
    else if (u > edge.u)
      return false;
    return v < edge.v;
  }
};

struct Face {
  int u;
  int v;
  int w;
  Face(int _u, int _v, int _w) {
    u = _u;
    v = _v;
    w = _w;
  }
  bool operator<(const Face &face) const {
    auto ua = u;
    auto va = v;
    auto wa = w;
    auto ub = face.u;
    auto vb = face.v;
    auto wb = face.w;
    if (wa > va) {
      std::swap(wa, va);
    }
    if (va > ua) {
      std::swap(va, ua);
    }
    if (wa > va) {
      std::swap(wa, va);
    }
    if (wb > vb) {
      std::swap(wb, vb);
    }
    if (vb > ub) {
      std::swap(vb, ub);
    }
    if (wb > vb) {
      std::swap(wb, vb);
    }
    if (ua < ub)
      return true;
    else if (ua > ub)
      return false;
    if (va < vb)
      return true;
    else if (va > vb)
      return false;
    return wa < wb;
  }
};
struct vec3compare {
  bool operator()(const glm::vec3 &v0, const glm::vec3 &v1) const {
    if (v0.x < v1.x)
      return true;
    if (v0.x > v1.x)
      return false;
    if (v0.y < v1.y)
      return true;
    if (v0.y > v1.y)
      return false;
    return v0.z < v1.z;
  }
};
}  // namespace

void Mesh::initFromVectors(const std::vector<glm::vec3> &vertices,
                           const std::vector<glm::i32vec3> &faces) {
  _vertices = vertices;
  _faces = faces;
}

void Mesh::loadFromFile(const std::string &filePath) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  // QFileInfo info(QString(filePath.c_str()));
  std::string err;
  bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err,
                              filePath.c_str(), (filePath + "/").c_str(), true);
  if (!err.empty()) {
    std::cerr << err << std::endl;
  }

  if (!ret) {
    std::cerr << "Failed to load/parse .obj file" << std::endl;
    return;
  }

  for (size_t s = 0; s < shapes.size(); s++) {
    size_t index_offset = 0;
    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
      unsigned int fv = shapes[s].mesh.num_face_vertices[f];

      glm::i32vec3 face;
      for (size_t v = 0; v < fv; v++) {
        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

        face[v] = idx.vertex_index;
      }
      _faces.push_back(face);

      index_offset += fv;
    }
  }
  for (size_t i = 0; i < attrib.vertices.size(); i += 3) {
    _vertices.emplace_back(attrib.vertices[i], attrib.vertices[i + 1],
                           attrib.vertices[i + 2]);
  }
  std::cout << "Loaded " << _faces.size() << " faces and " << _vertices.size()
            << " vertices" << std::endl;

  // TODO: Convert the mesh into your own data structure, if necessary.
  // Construct half edges.
  _halfedges.resize(_faces.size() * 3);
  for (size_t i = 0; i < _faces.size(); ++i) {
    for (size_t j = 0; j < 3; ++j) {
      _halfedges[(i * 3) + j]._face = i;
      _halfedges[(i * 3) + j]._opposite = -1;
      _halfedges[(i * 3) + j]._vertex = _faces[i][j];
      _halfedges[(i * 3) + j]._next = i * 3 + (j + 1) % 3;
      _vertToHalfEdges[_faces[i][j]].push_back((i * 3) + j);
    }
  }

  // Find the opposite pair.
  for (size_t i = 0; i < _halfedges.size(); ++i) {
    if (_halfedges[i]._opposite == -1) {
      int vertex1 = _halfedges[i]._vertex;
      int vertex2 = _halfedges[_halfedges[i]._next]._vertex;
      int face = _halfedges[i]._face;

      for (size_t j = 0; j < _halfedges.size(); ++j) {
        if (_halfedges[j]._vertex == vertex2 &&
            _halfedges[_halfedges[j]._next]._vertex == vertex1 &&
            _halfedges[j]._face != face) {
          _halfedges[i]._opposite = j;
          _halfedges[j]._opposite = i;
          break;
        }
      }
    }
  }
}

void Mesh::saveToFile(const std::string &filePath) {
  // TODO: Convert your data structure back to the basic format, if necessary.

  std::ofstream outfile;
  outfile.open(filePath);

  // Write vertices
  for (size_t i = 0; i < _vertices.size(); i++) {
    const glm::vec3 &v = _vertices[i];
    outfile << "v " << v[0] << " " << v[1] << " " << v[2] << std::endl;
  }

  // Write faces
  for (size_t i = 0; i < _faces.size(); i++) {
    const glm::i32vec3 &f = _faces[i];
    outfile << "f " << (f[0] + 1) << " " << (f[1] + 1) << " " << (f[2] + 1)
            << std::endl;
  }

  outfile.close();
}

// TODO: implement the operations
// TODO: mesh validation (tips: use assertions)
// Optional
void Mesh::validate() {
}

// TODO: Loop Subdivision
void Mesh::subdivide(int iterations) {
  for (int it = 0; it < iterations; it++) {
    std::map<glm::vec3, int, vec3compare> index_map;
    std::vector<glm::vec3> vertices_new;
    std::vector<glm::i32vec3> faces_new;

    // HalfEdge data structures.
    std::vector<HalfEdge> halfedges_new;
    halfedges_new.resize(_faces.size() * 4 * 3);
    std::map<glm::int32, std::vector<size_t>> vertToHalfEdges_new;

    auto get_vertex_index = [&](const glm::vec3 &v) {
      if (!index_map.count(v)) {
        index_map[v] = int(vertices_new.size());
        vertices_new.push_back(v);
      }
      return index_map.at(v);
    };
    auto insert_triangle = [&](const glm::vec3 v0, const glm::vec3 v1,
                               const glm::vec3 v2) {
      faces_new.emplace_back(get_vertex_index(v0), get_vertex_index(v1),
                             get_vertex_index(v2));
    };

    /*
     * This is a framework to help you merge identical vertices.
     * You can insert a triangle into the new mesh like this:
     * insert_triangle(
     *   glm::vec3{1.0f, 0.0f, 0.0f},
     *   glm::vec3{0.0f, 1.0f, 0.0f},
     *   glm::vec3{0.0f, 0.0f, 1.0f}
     * );
     * However, you still need to be careful on the triangle orientation.
     * */
    /*****************************/
    /* Write your code here. */
    // Given the face and vertex indexes, find the vertex opposite the half edge
    // corresponding to the vertex.
    auto get_opposite_vertex = [&](const size_t &face_idx,
                                   const size_t &vertex_id) -> glm::vec3 {
      return _vertices
          [_halfedges[_halfedges[_halfedges[_halfedges[face_idx * 3 + vertex_id]
                                                ._opposite]
                                     ._next]
                          ._next]
               ._vertex];
    };

    // Insert half edges for the three triangles that are on the outside.
    auto insert_ext_halfedges = [&](const int &orig_face_index,
                                    const int &new_face_offset,
                                    const glm::vec3 v0, const glm::vec3 v1,
                                    const glm::vec3 v2) {
      vertToHalfEdges_new[get_vertex_index(v0)].push_back(
          orig_face_index * 12 + new_face_offset * 3 + 0);
      vertToHalfEdges_new[get_vertex_index(v1)].push_back(
          orig_face_index * 12 + new_face_offset * 3 + 1);
      vertToHalfEdges_new[get_vertex_index(v2)].push_back(
          orig_face_index * 12 + new_face_offset * 3 + 2);
      auto &new_half_edge0 =
          halfedges_new[orig_face_index * 12 + new_face_offset * 3 + 0];
      auto &new_half_edge1 =
          halfedges_new[orig_face_index * 12 + new_face_offset * 3 + 1];
      auto &new_half_edge2 =
          halfedges_new[orig_face_index * 12 + new_face_offset * 3 + 2];
      auto opposite_halfedge0_idx =
          _halfedges[orig_face_index * 3 + new_face_offset]._opposite;
      auto opposite_halfedge2_idx =
          _halfedges[orig_face_index * 3 + (new_face_offset + 2) % 3]._opposite;
      new_half_edge0 = {
          ._face = orig_face_index * 4 + new_face_offset,
          ._next = orig_face_index * 12 + new_face_offset * 3 + 1,
          ._vertex = get_vertex_index(v0),
          ._opposite = (opposite_halfedge0_idx / 3) * 12 +
                       ((opposite_halfedge0_idx % 3 + 1) % 3) * 3 + 2};
      new_half_edge1 = {
          ._face = orig_face_index * 4 + new_face_offset,
          ._next = orig_face_index * 12 + new_face_offset * 3 + 2,
          ._vertex = get_vertex_index(v1),
          ._opposite = orig_face_index * 12 + 3 * 3 + new_face_offset};
      new_half_edge2 = {._face = orig_face_index * 4 + new_face_offset,
                        ._next = orig_face_index * 12 + new_face_offset * 3 + 0,
                        ._vertex = get_vertex_index(v2),
                        ._opposite = (opposite_halfedge2_idx / 3) * 12 +
                                     (opposite_halfedge2_idx % 3) * 3 + 0};
    };

    // Insert half edges for the three triangles that are on the inside.
    auto insert_int_halfedges = [&](const int &orig_face_index,
                                    const glm::vec3 v0, const glm::vec3 v1,
                                    const glm::vec3 v2) {
      vertToHalfEdges_new[get_vertex_index(v0)].push_back(orig_face_index * 12 +
                                                          3 * 3 + 0);
      vertToHalfEdges_new[get_vertex_index(v1)].push_back(orig_face_index * 12 +
                                                          3 * 3 + 1);
      vertToHalfEdges_new[get_vertex_index(v2)].push_back(orig_face_index * 12 +
                                                          3 * 3 + 2);
      auto &new_half_edge0 = halfedges_new[orig_face_index * 12 + 3 * 3 + 0];
      auto &new_half_edge1 = halfedges_new[orig_face_index * 12 + 3 * 3 + 1];
      auto &new_half_edge2 = halfedges_new[orig_face_index * 12 + 3 * 3 + 2];
      new_half_edge0 = {._face = orig_face_index * 4 + 3,
                        ._next = orig_face_index * 12 + 3 * 3 + 1,
                        ._opposite = orig_face_index * 12 + 0 * 3 + 1,
                        ._vertex = get_vertex_index(v0)};
      new_half_edge1 = {._face = orig_face_index * 4 + 3,
                        ._next = orig_face_index * 12 + 3 * 3 + 2,
                        ._opposite = orig_face_index * 12 + 1 * 3 + 1,
                        ._vertex = get_vertex_index(v1)};
      new_half_edge2 = {._face = orig_face_index * 4 + 3,
                        ._next = orig_face_index * 12 + 3 * 3 + 0,
                        ._opposite = orig_face_index * 12 + 2 * 3 + 1,
                        ._vertex = get_vertex_index(v2)};
    };

    for (size_t i = 0; i < _faces.size(); ++i) {
      /*
            e_0
          /    \
        o_0 -  o_2
       /  \   /  \
      e_1- o_1 - e_2
      */
      // Initialize vertices.
      glm::vec3 even[3];
      even[0] = _vertices[_faces[i][0]];
      even[1] = _vertices[_faces[i][1]];
      even[2] = _vertices[_faces[i][2]];
      glm::vec3 odd_opposite[3];
      odd_opposite[0] = get_opposite_vertex(i, 0);
      odd_opposite[1] = get_opposite_vertex(i, 1);
      odd_opposite[2] = get_opposite_vertex(i, 2);

      // Calculate odd points.
      glm::vec3 odd[3];
      odd[0] =
          0.375f * (even[0] + even[1]) + 0.125f * (even[2] + odd_opposite[0]);
      odd[1] =
          0.375f * (even[1] + even[2]) + 0.125f * (even[0] + odd_opposite[1]);
      odd[2] =
          0.375f * (even[2] + even[0]) + 0.125f * (even[1] + odd_opposite[2]);

      // Calculate new even points.
      glm::vec3 even_new[3] = {glm::vec3(0.0f), glm::vec3(0.0f),
                               glm::vec3(0.0f)};
      for (int j = 0; j < 3; ++j) {
        size_t n = _vertToHalfEdges[_faces[i][j]].size();
        glm::float32 u = (n == 3)
                             ? 3.0f / 16.0f
                             : 3.0f / (8.0f * static_cast<glm::float32>(n));
        for (size_t halfedge_idx : _vertToHalfEdges[_faces[i][j]]) {
          even_new[j] +=
              _vertices[_halfedges[_halfedges[halfedge_idx]._next]._vertex];
        }
        even_new[j] *= u;
        even_new[j] += (1 - n * u) * even[j];
      }

      // Insert triangles and their corresponding half edges.
      insert_triangle(even_new[0], odd[0], odd[2]);
      insert_ext_halfedges(i, 0, even_new[0], odd[0], odd[2]);

      insert_triangle(even_new[1], odd[1], odd[0]);
      insert_ext_halfedges(i, 1, even_new[1], odd[1], odd[0]);

      insert_triangle(even_new[2], odd[2], odd[1]);
      insert_ext_halfedges(i, 2, even_new[2], odd[2], odd[1]);

      insert_triangle(odd[2], odd[0], odd[1]);
      insert_int_halfedges(i, odd[2], odd[0], odd[1]);
    }
    // Update half edges & vert -> half edges map.
    _halfedges = halfedges_new;
    _vertToHalfEdges = vertToHalfEdges_new;
    /*****************************/
    _vertices = vertices_new;
    _faces = faces_new;
  }
}

glm::mat4 QMatrix(const glm::vec3 &p0,
                  const glm::vec3 &p1,
                  const glm::vec3 &p2) {
  /*
   * TODO: Implement this function to compute the Q matrix.
   * The Q matrix is defined with respect to the plane corresponding to the
   * given triangle. You are provided with the three vertices of a triangle.
   * Your task is to derive the plane equation and subsequently compute the Q
   * matrix. If the triangle is degenerate (collapsed), simply return a zero
   * matrix. Note: The resulting matrix should be symmetric.
   */
  glm::vec3 edge1 = p1 - p0;
  glm::vec3 edge2 = p2 - p0;
  glm::vec3 normal = glm::cross(edge1, edge2);

  // Check for degenerate triangle
  if (glm::length(normal) < 1e-6) {
    return glm::mat4(0.0f);
  }

  normal = glm::normalize(normal);

  glm::mat4 Q(0.0f);

  glm::vec4 v_tag = glm::vec4(normal, -glm::dot(normal, p0));
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      Q[i][j] += v_tag[i] * v_tag[j];
    }
  }

  return Q;
  // return glm::mat4{0.0f};
}

glm::vec3 GenerateMergedPoint(const glm::vec3 &p0,
                              const glm::vec3 &p1,
                              glm::mat4 Q) {
  /*
   * TODO: Generate the merged point using the Q matrix.
   * Provided your QMatrix function is correctly implemented,
   * the Q matrix is guaranteed to be a symmetric semi-positive definite.
   * The error value of a particular position is defined by the formula x^TQx,
   * where 'x' represents a Homogeneous linear coordinate, expressed in 4
   * dimensions but interpreted in 3 dimensions. Your task is to identify and
   * return the position that yields the minimum error. If the position cannot
   * be determined, return the midpoint between p0 and p1.
   */
  auto Q_orig = Q;
  Q[0][3] = 0.0f;
  Q[1][3] = 0.0f;
  Q[2][3] = 0.0f;
  Q[3][3] = 1.0f;
  if (glm::determinant(Q) < 1e-6) {
    // When the matrix is singular, the solution is much trickier.
    // I noticed that most projects on GitHub do not correctly implement this
    // part. For the full proof, please refer to the report.
    glm::vec3 d = p0 - p1;
    glm::mat3 A(Q);
    glm::vec3 b = glm::vec3(Q[3]);
    glm::vec3 Ad = A * d;
    glm::float32 denom = 2.0f * glm::dot(d, Ad);
    if (glm::abs(denom) < 1e-6) {
      // Compute 3 errors respectively and choose the optimal.
      float error_p0 = glm::dot(glm::vec4(p0, 1.0f), Q_orig * glm::vec4(p0, 1.0f));
      float error_p1 = glm::dot(glm::vec4(p1, 1.0f), Q_orig * glm::vec4(p1, 1.0f));
      auto mid = 0.5f * (p0 + p1);
      float error_mid =
          glm::dot(glm::vec4(mid, 1.0f), Q_orig * glm::vec4(mid, 1.0f));
      glm::vec3 ret;
      if (error_p0 < error_p1) {
        ret = p0;
      } else {
        ret = p1;
      }
      if (error_mid < error_p0 && error_mid < error_p1) {
        ret = mid;
      }
      return ret;
    }
    glm::vec3 Ap1 = A * p1;
    glm::float32 a =
        (-glm::dot(d, Ap1) - glm::dot(p1, Ad) - 2.0f * glm::dot(b, d)) /
        denom;
    a = glm::clamp(a, 0.0f, 1.0f);
    return p1 + a * d;
  } else {
    return glm::inverse(Q) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
  }

  // return (p0 + p1) * 0.5f;
}

// QEM Mesh Simplification
// You do not need to change this function under this framework.
void Mesh::simplify(int num_faces_to_remove) {
  std::vector<glm::vec3> vertices = _vertices;
  std::vector<int> leader(vertices.size());
  std::vector<glm::mat4> Qs(vertices.size(), glm::mat4{0.0f});
  std::vector<std::set<int>> adjacent_vertices(vertices.size());
  std::set<Edge> edges;
  std::vector<std::set<Face>> adjacent_faces(vertices.size());
  std::set<Face> faces;
  auto add_vertex = [&](const glm::vec3 &pos, const glm::mat4 &q) {
    auto index = int(leader.size());
    leader.push_back(index);
    Qs.push_back(q);
    vertices.push_back(pos);
    adjacent_vertices.emplace_back();
    adjacent_faces.emplace_back();
    return index;
  };

  for (size_t i = 0; i < vertices.size(); i++) {
    leader[i] = int(i);
  }

  std::function<int(int x)> find_leader;
  find_leader = [&leader, &find_leader](int x) -> int {
    return (x == leader[x]) ? x : (leader[x] = find_leader(leader[x]));
  };

  auto insert_face = [&](int u, int v, int w) {
    if (u == v || v == w || u == w)
      return;
    Face face(u, v, w);
    faces.insert(face);
    adjacent_faces[u].insert(face);
    adjacent_faces[v].insert(face);
    adjacent_faces[w].insert(face);
  };

  auto remove_face = [&](int u, int v, int w) {
    Face face(u, v, w);
    faces.erase(face);
    adjacent_faces[u].erase(face);
    adjacent_faces[v].erase(face);
    adjacent_faces[w].erase(face);
  };
  auto insert_edge = [&](int u, int v) {
    if (u == v)
      return;
    edges.insert(Edge(u, v));
    adjacent_vertices[u].insert(v);
    adjacent_vertices[v].insert(u);
  };
  auto remove_edge = [&](int u, int v) {
    edges.erase(Edge(u, v));
    adjacent_vertices[u].erase(v);
    adjacent_vertices[v].erase(u);
  };

  for (auto &face : _faces) {
    auto q = QMatrix(vertices[face.x], vertices[face.y], vertices[face.z]);
    Qs[face.x] += q;
    Qs[face.y] += q;
    Qs[face.z] += q;
    insert_face(face.x, face.y, face.z);
    insert_edge(face.x, face.y);
    insert_edge(face.y, face.z);
    insert_edge(face.x, face.z);
  }

  struct MergePack {
    float err;
    Edge e;
    glm::vec4 v;
    glm::mat4 q;
    bool operator<(const MergePack &pack) const {
      return err > pack.err;
    }
  };

  std::set<Edge> in_flight;
  std::priority_queue<MergePack> packs;
  auto add_pack = [&](const MergePack &pack) {
    if (in_flight.count(pack.e))
      return;
    in_flight.insert(pack.e);
    packs.push(pack);
  };

  auto new_pack = [&](const Edge &edge) {
    MergePack pack{
        0.0f, edge, {0.0f, 0.0f, 0.0f, 1.0f}, Qs[edge.u] + Qs[edge.v]};
    auto res = GenerateMergedPoint(vertices[edge.u], vertices[edge.v], pack.q);
    pack.v = glm::vec4{res, 1.0f};
    pack.err = glm::dot(pack.v, pack.q * pack.v);
    add_pack(pack);
  };

  for (auto &edge : edges) {
    new_pack(edge);
  }

  int target_face_num = int(_faces.size()) - num_faces_to_remove;
  std::set<Face> removing_faces;
  std::set<Edge> removing_edges;
  std::set<Face> new_faces;
  std::set<Edge> new_edges;
  while (target_face_num < faces.size()) {
    auto merge_pack = packs.top();
    packs.pop();
    auto edge = merge_pack.e;
    in_flight.erase(edge);
    if (leader[edge.u] != edge.u || leader[edge.v] != edge.v) {
      continue;
    }
    int index = add_vertex(glm::vec3{merge_pack.v}, merge_pack.q);
    leader[edge.u] = index;
    leader[edge.v] = index;
    removing_faces.clear();
    removing_edges.clear();
    new_edges.clear();
    new_faces.clear();
    for (auto ni : adjacent_vertices[edge.u]) {
      removing_edges.insert(Edge(edge.u, ni));
      new_edges.insert(Edge(find_leader(edge.u), find_leader(ni)));
    }
    for (auto ni : adjacent_vertices[edge.v]) {
      removing_edges.insert(Edge(edge.v, ni));
      new_edges.insert(Edge(find_leader(edge.v), find_leader(ni)));
    }
    for (auto face : adjacent_faces[edge.u]) {
      removing_faces.insert(face);
      new_faces.insert(
          Face(find_leader(face.u), find_leader(face.v), find_leader(face.w)));
    }
    for (auto face : adjacent_faces[edge.v]) {
      removing_faces.insert(face);
      new_faces.insert(
          Face(find_leader(face.u), find_leader(face.v), find_leader(face.w)));
    }
    for (auto face : removing_faces) {
      remove_face(face.u, face.v, face.w);
    }
    for (auto e : removing_edges) {
      remove_edge(e.u, e.v);
    }
    for (auto face : new_faces) {
      insert_face(face.u, face.v, face.w);
    }
    for (auto e : new_edges) {
      if (e.u == e.v)
        continue;
      insert_edge(e.u, e.v);
      new_pack(e);
    }
  }

  std::map<glm::vec3, int, vec3compare> index_map;
  std::vector<glm::vec3> vertices_new;
  std::vector<glm::i32vec3> faces_new;

  auto get_vertex_index = [&](const glm::vec3 &v) {
    if (!index_map.count(v)) {
      index_map[v] = int(vertices_new.size());
      vertices_new.push_back(v);
    }
    return index_map.at(v);
  };

  auto insert_triangle = [&](const glm::vec3 v0, const glm::vec3 v1,
                             const glm::vec3 v2) {
    faces_new.emplace_back(get_vertex_index(v0), get_vertex_index(v1),
                           get_vertex_index(v2));
  };

  for (auto &face : faces) {
    insert_triangle(vertices[face.u], vertices[face.v], vertices[face.w]);
  }

  _vertices = vertices_new;
  _faces = faces_new;
}
