#pragma once

// #include <CGAL/IO/Color.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_tree.h>
#include <cassert>
#include <optional>
#include <vector>

#include "cinder/Ray.h"
#include "cinder/gl/gl.h"
#include "geometry/ColorManager.h"
#include "geometry/ModelImporter.h"
#include "geometry/Triangle.h"

namespace pepr3d {

using Direction = K::Direction_3;
using Ft = K::FT;
using Ray = K::Ray_3;
using My_AABB_traits = CGAL::AABB_traits<K, DataTriangleAABBPrimitive>;
using Tree = CGAL::AABB_tree<My_AABB_traits>;
using Ray_intersection = boost::optional<Tree::Intersection_and_primitive_id<Ray>::Type>;

class Geometry {
   public:
    using ColorIndex = GLuint;

   private:
    /// Triangle soup of the model mesh, containing CGAL::Triangle_3 data for AABB tree.
    std::vector<DataTriangle> mTriangles;

    /// Vertex buffer with the same data as mTriangles for OpenGL to render the mesh.
    /// Contains position and color data for each vertex.
    std::vector<glm::vec3> mVertexBuffer;

    /// Color buffer, keeping the invariant that every triangle has only one color - all three vertices have to have the
    /// same color. It is aligned with the vertex buffer and its size should be always equal to the vertex buffer.
    std::vector<ColorIndex> mColorBuffer;

    /// Normal buffer, the triangle has same normal for its every vertex.
    /// It is aligned with the vertex buffer and its size should be always equal to the vertex buffer.
    std::vector<glm::vec3> mNormalBuffer;

    /// Index buffer for OpenGL frontend., specifying the same triangles as in mTriangles.
    std::vector<uint32_t> mIndexBuffer;

    /// AABB tree from the CGAL library, to find intersections with rays generated by user mouse clicks and the mesh.
    std::unique_ptr<Tree> mTree;

    /// A vector based map mapping size_t into ci::ColorA
    ColorManager mColorManager;

   public:
    /// Empty constructor rendering a triangle to debug
    Geometry() {
        mTriangles.emplace_back(glm::vec3(-1, 0, -1), glm::vec3(-1, 0, 1), glm::vec3(1, 0, -1), glm::vec3(0, 1, 0), 0);
        mTriangles.emplace_back(glm::vec3(1, 0, -1), glm::vec3(1, 0, 1), glm::vec3(-1, 0, 1), glm::vec3(0, 1, 0), 0);

        generateVertexBuffer();
        generateColorBuffer();
        generateNormalBuffer();
        generateIndexBuffer();

        assert(mIndexBuffer.size() == mVertexBuffer.size());

        mTree = std::make_unique<Tree>(mTriangles.begin(), mTriangles.end());
        assert(mTree->size() == mTriangles.size());
    }

    /// Returns a constant iterator to the vertex buffer
    std::vector<glm::vec3>& getVertexBuffer() {
        return mVertexBuffer;
    }

    /// Returns a constant iterator to the index buffer
    std::vector<uint32_t>& getIndexBuffer() {
        return mIndexBuffer;
    }

    std::vector<ColorIndex>& getColorBuffer() {
        return mColorBuffer;
    }

    std::vector<glm::vec3>& getNormalBuffer() {
        return mNormalBuffer;
    }

    /// Loads new geometry into the private data, rebuilds the vertex and index buffers
    /// automatically.
    void loadNewGeometry(const std::string& fileName) {
        /// Load into mTriangles
        ModelImporter modelImporter(fileName);  // only first mesh [0]

        if(modelImporter.isModelLoaded()) {
            mTriangles = modelImporter.getTriangles();

            /// Generate new vertex buffer
            generateVertexBuffer();

            /// Generate new index buffer
            generateIndexBuffer();

            /// Generate new color buffer from triangle color data
            generateColorBuffer();

            /// Generate new normal buffer, copying the triangle normal to each vertex
            generateNormalBuffer();

            /// Rebuild the AABB tree
            mTree->rebuild(mTriangles.begin(), mTriangles.end());  // \todo Uncomment this when CGAL is in.
            assert(mTree->size() == mTriangles.size());

            mColorManager = modelImporter.getColorManager();
            assert(!mColorManager.empty());
        } else {
            CI_LOG_E("Model not loaded --> write out message for user");
        }
    }

    /// Set new triangle color
    void setTriangleColor(const size_t triangleIndex, const size_t newColor) {
        /// Change it in the buffer
        // Color buffer has 1 ColorA for each vertex, each triangle has 3 vertices
        const size_t vertexPosition = triangleIndex * 3;

        // Change all vertices of the triangle to the same new color
        assert(vertexPosition + 2 < mColorBuffer.size());

        ColorIndex newColorIndex = static_cast<ColorIndex>(newColor);
        mColorBuffer[vertexPosition] = newColorIndex;
        mColorBuffer[vertexPosition + 1] = newColorIndex;
        mColorBuffer[vertexPosition + 2] = newColorIndex;

        /// Change it in the triangle soup
        assert(triangleIndex < mTriangles.size());
        mTriangles[triangleIndex].setColor(newColor);
    }

    /// Get the color of the indexed triangle
    size_t getTriangleColor(const size_t triangleIndex) {
        assert(triangleIndex < mTriangles.size());
        return mTriangles[triangleIndex].getColor();
    }

    /// Intersects the mesh with the given ray and returns the index of the triangle intersected, if it exists.
    /// Example use: generate ray based on a mouse click, call this method, then call setTriangleColor.
    std::optional<size_t> intersectMesh(const ci::Ray& ray) const {
        assert(!mTree->empty());
        const glm::vec3 source = ray.getOrigin();
        const glm::vec3 direction = ray.getDirection();

        const Ray rayQuery(Point(source.x, source.y, source.z), Direction(direction.x, direction.y, direction.z));

        // Find the two intersection parameters - place and triangle
        Ray_intersection intersection = mTree->first_intersection(rayQuery);
        if(intersection) {
            // The intersected triangle
            if(boost::get<DataTriangleAABBPrimitive::Id>(intersection->second) != mTriangles.end()) {
                const DataTriangleAABBPrimitive::Id intersectedTriIter =
                    boost::get<DataTriangleAABBPrimitive::Id>(intersection->second);
                assert(intersectedTriIter != mTriangles.end());
                const size_t retValue = intersectedTriIter - mTriangles.begin();
                assert(retValue < mTriangles.size());
                return retValue;  // convert the iterator into an index
            }
        }

        /// No intersection detected.
        return {};
    }

    /// Return the number of triangles in the model
    size_t getTriangleCount() const {
        return mTriangles.size();
    }

    const ColorManager& getColorManager() const {
        return mColorManager;
    }

    ColorManager& getColorManager() {
        return mColorManager;
    }

   private:
    /// Generates the vertex buffer linearly - adding each vertex of each triangle as a new one.
    /// We need to do this because each triangle has to be able to be colored differently, therefore no vertex sharing
    /// is possible.
    void generateVertexBuffer() {
        mVertexBuffer.clear();
        mVertexBuffer.reserve(3 * mTriangles.size());

        for(const auto& mTriangle : mTriangles) {
            mVertexBuffer.push_back(mTriangle.getVertex(0));
            mVertexBuffer.push_back(mTriangle.getVertex(1));
            mVertexBuffer.push_back(mTriangle.getVertex(2));
        }
    }

    /// Generating a linear index buffer, since we do not reuse any vertices.
    void generateIndexBuffer() {
        mIndexBuffer.clear();
        mIndexBuffer.reserve(mVertexBuffer.size());

        for(uint32_t i = 0; i < mVertexBuffer.size(); ++i) {
            mIndexBuffer.push_back(i);
        }
    }

    /// Generating triplets of colors, since we only allow a single-colored triangle.
    void generateColorBuffer() {
        mColorBuffer.clear();
        mColorBuffer.reserve(mVertexBuffer.size());

        for(const auto& mTriangle : mTriangles) {
            const ColorIndex triColorIndex = static_cast<ColorIndex>(mTriangle.getColor());
            mColorBuffer.push_back(triColorIndex);
            mColorBuffer.push_back(triColorIndex);
            mColorBuffer.push_back(triColorIndex);
        }
        assert(mColorBuffer.size() == mVertexBuffer.size());
    }

    /// Generate a buffer of normals. Generates only "triangle normals" - all three vertices have the same normal.
    void generateNormalBuffer() {
        mNormalBuffer.clear();
        mNormalBuffer.reserve(mVertexBuffer.size());
        for(const auto& mTriangle : mTriangles) {
            mNormalBuffer.push_back(mTriangle.getNormal());
            mNormalBuffer.push_back(mTriangle.getNormal());
            mNormalBuffer.push_back(mTriangle.getNormal());
        }
        assert(mNormalBuffer.size() == mVertexBuffer.size());
    }
};

}  // namespace pepr3d