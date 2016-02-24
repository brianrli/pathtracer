#ifndef __pathtracer__bvh__
#define __pathtracer__bvh__

#include <stdio.h>
#include <vector>
#include "common.hpp"

const int MAX_TRIANGLES_LEAF = 2;
const int NUM_BUCKETS = 12;

struct TriangleInfo {
    int index;
    Vec3f centroid;
    BoundingBox bbox;
};

struct TreeNode {
    TreeNode() {
        children[0] = NULL;
        children[1] = NULL;
    }
    BoundingBox bbox;
    TreeNode *children[2];
    int axis, index, nPrimitives;
};

struct SurfaceAreaHeuristic {
    SurfaceAreaHeuristic(int n) : nbuckets(n) {
        count = std::vector<int>(n,0);
        bbox = std::vector<BoundingBox>(n);
        cost = std::vector<float>(n-1);
        triangles = std::vector<std::vector<int> >(n);
    }
    int nbuckets;
    std::vector<float> cost;
    std::vector<int> count;
    std::vector<BoundingBox> bbox;
    std::vector<std::vector<int> > triangles;
};

class BoundingVolumeHierarchy {
private:
    std::vector<VectorTriangle> vtriangles;
    std::vector<VectorTriangle> orderedvtriangles;
    
    // Bounding Volume Hierarchy
    BVH_Node* bvh;

    // for BVH construction
    int totalNodes;
    TreeNode* recBuildTree(int start, int end);
    int flattenTree(TreeNode *node, int *offset);
    
public:
    BoundingVolumeHierarchy () {};
    BoundingVolumeHierarchy (std::vector<VectorTriangle> vt) {
        vtriangles = vt;
    };
    void buildTree();
};

#endif 
