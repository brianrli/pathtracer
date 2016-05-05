#ifndef __pathtracer__bvh__
#define __pathtracer__bvh__

#include <stdio.h>
#include <vector>
#include "common.hpp"

const int MAX_TRIANGLES_LEAF = 24;
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
    BVH_Node_BBox *bvh_bbox;
    BVH_Node_Info *bvh_info;

    bool bvh_initialized;

    // for BVH construction
    int totalNodes;
    TreeNode* recBuildTree(int start, int end);
    int flattenTree(TreeNode *node, int *offset);
    
public:
    BoundingVolumeHierarchy () {};
    BoundingVolumeHierarchy (std::vector<VectorTriangle> vt) {
        vtriangles = vt;
        bvh_initialized = true;
        totalNodes = 0;
    };
    
    void buildTree();
    std::vector<VectorTriangle> get_triangles();
    
    BVH_Node* get_bvh();
    BVH_Node_BBox* get_bvh_bbox();
    BVH_Node_Info* get_bvh_info();

    int get_n_nodes();
};

#endif 
