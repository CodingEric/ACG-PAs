#ifndef MESH_H
#define MESH_H

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>

struct HalfEdge
{
  int _vertex; // Index of the first vertex.
  int _next; // Index of next HalfEdge.
  int _face; // Index of corresponding face.
  int _opposite; // Index of the opposite HalfEdge.
};



class Mesh {
 public:
  void initFromVectors(const std::vector<glm::vec3> &vertices,
                       const std::vector<glm::i32vec3> &faces);
  void loadFromFile(const std::string &filePath);
  void saveToFile(const std::string &filePath);
  void subdivide(int iterations);
  void simplify(int num_faces_to_remove);
  void validate();
  std::vector<glm::vec3> getVertices() const {
    return _vertices;
  }
  std::vector<glm::i32vec3> getFaces() const {
    return _faces;
  }

 private:
  std::vector<glm::vec3> _vertices;
  std::vector<glm::i32vec3> _faces;

  std::vector<HalfEdge> _halfedges;
  std::map<glm::int32, std::vector<size_t> > _vertToHalfEdges;
};

#endif  // MESH_H
