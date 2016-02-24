//
//  bvh.cpp
//  pathtracer
//
//  Created by Brian Li on 2/20/16.
//  Copyright (c) 2016 Brian Li. All rights reserved.
//

#include "bvh.hpp"

using namespace std;

void inorder(TreeNode *node,
             std::vector<VectorTriangle> triangles) {
    if(node->children[0] != NULL) {
        inorder(node->children[0],triangles);
    }

    if (node->children[0] == NULL || node->children[1] == NULL) {
        cout << "Child | index: " << node->index << " | nPrimitives: " << node->nPrimitives << "\n";
    }
    else {
        cout << "Central Node\n";
    }
    
    if(node->children[1] != NULL) {
        inorder(node->children[1],triangles);
    }
}

void BoundingVolumeHierarchy::buildTree() {
    

	int start = 0;
	int end = vtriangles.size();

	// clear
	orderedvtriangles = std::vector<VectorTriangle>();
	totalNodes = 0;

	// build bounding volume hierarchy recursively
	TreeNode *root = recBuildTree(start,end);
    bvh = (BVH_Node*)malloc(sizeof(BVH_Node) * totalNodes);
    
//        cout << "TotalNodes: " << totalNodes << "\n";
//        cout << "unordered vt size: " << vtriangles.size() << "\n";
//        cout << "ovt size: " << orderedvtriangles.size() << "\n";
    
    inorder(root,vtriangles);
    
    // flatten tree
    int offset = 0;
    flattenTree(root,&offset);
    for(int i = 0; i < totalNodes; i++) {
        cout << i << " : ";
        cout << bvh[i].nPrimitives << " ";
        if (bvh[i].nPrimitives == 0) {
            cout << bvh[i].offset << " ";
            cout << bvh[i].secondChildOffset << " ";
        }
        cout << "\n";
    }
//    cout << "finished\n";
};

// initialize new leaf in bvh tree
void createLeaf(TreeNode *node, BoundingBox bounds,
                std::vector<VectorTriangle> &vtriangles,
                std::vector<VectorTriangle> &orderedvtriangles,
                int start, int end) {

	node->bbox = bounds;
	node->index = orderedvtriangles.size();
	node->nPrimitives = end-start;

	for (int i = start; i < end; i++) {
		orderedvtriangles.push_back(vtriangles[i]);
	}
};

TreeNode* BoundingVolumeHierarchy::recBuildTree(int start, int end) {
	
//    cout << "recBuildTree\n";
    
	//vtriangles is primitive info
	totalNodes++;
	TreeNode *node = new TreeNode();

	// compute bounding box
	BoundingBox bounds = vtriangles[start].bbox;
    
	for (int i = start+1; i < end; i++) {
		bounds = combine(bounds,vtriangles[i].bbox);
	}

	int nprimitives = end - start;
	// only one triangle left, so create leaf
	if (nprimitives == 1) {
		createLeaf(node,bounds,vtriangles,orderedvtriangles,start,end);
        return node;
	}
	
	// more than one triangle, so create interior node
	else {
		
        //Splitting Triangles based on the midpoint of centroids on an axis
		BoundingBox centroidBounds = BoundingBox(vtriangles[start].centroid);
        for (int i = start+1; i < end; i++) {
			centroidBounds = combine(centroidBounds,vtriangles[i].centroid);
		}
		int axis = centroidBounds.longestAxis();

		int mid = (start+end) / 2;
		
		if(centroidBounds.maxbounds[axis] == centroidBounds.minbounds[axis]) {
            
			// maximum triangles per leaf
			if (nprimitives <= MAX_TRIANGLES_LEAF) {
                createLeaf(node,bounds,vtriangles,orderedvtriangles,start,end);
				return node;
			}

			// need to split further
			else {
				// create children
				node->children[0] = recBuildTree(start,mid);
				node->children[1] = recBuildTree(mid,end);
				node->bbox = combine(node->children[0]->bbox,
					node->children[1]->bbox);
				node->axis = axis;
                node->nPrimitives = 0;
				return node;
			}

		}


		// Split using Surface Area Heuristic
		if (false) {
			// Partition primitives into equally sized subsets
		}
		else {
            //cout << "SAH Construction Start\n";
            
			// SAH Construction
			SurfaceAreaHeuristic sah = SurfaceAreaHeuristic(NUM_BUCKETS);

            double span = centroidBounds.maxbounds[axis] - centroidBounds.minbounds[axis];
			
			for (int i = start; i < end; i++) {
				
				int bucket = sah.nbuckets *
				(vtriangles[i].centroid[axis] - centroidBounds.minbounds[axis])/span;
				
				if (bucket == sah.nbuckets)
					bucket--;

				sah.count[bucket]++;
                
                //cout << "nbucket: " << sah.nbuckets << "\n";
                //cout << "bucket: " << bucket << " i: " << i << "\n";
				sah.triangles[bucket].push_back(i);

				if(sah.count[bucket]==1) {
					sah.bbox[bucket] = vtriangles[i].bbox;
				}
				else {
					sah.bbox[bucket] = combine(sah.bbox[bucket],vtriangles[i].bbox);
				}
			}

			// SAH Calculation
			for (int i = 0; i < sah.nbuckets-1; i++) {

				BoundingBox b0, b1;
				int count0 = 0;
				int count1 = 0;

				// first half of buckets
				for (int j = 0; j <= i; j++) {
					if(j==0) {
						b0 = vtriangles[j].bbox;
					}
					else {
						combine(b0,vtriangles[j].bbox);
					}
					count0 += sah.count[j]; 
				}

				// second half of buckets
				for (int k = i+1; k < sah.nbuckets; k++) {
					if(k==i+1) {
						b1 = vtriangles[k].bbox;
					}
					else {
						combine(b1,vtriangles[k].bbox);
					}
					count1 += sah.count[k];
				}

				// surface area cost determination
				sah.cost[i] = 0.125f + (count0 * b0.surfaceArea() + 
					count1 * b1.surfaceArea()) / bounds.surfaceArea();
			}

			// find minimum cost to partition
			float bestCost = sah.cost[0];
			int bestSplit = 0;

			for(int i = 1; i < sah.nbuckets-1; i++) {
				if(sah.cost[i] < bestCost) {
					bestCost = sah.cost[i];
					bestSplit = i;
				}
			}

			// create Splits
			if(nprimitives > MAX_TRIANGLES_LEAF || bestCost < nprimitives) {
				
                //cout << "create splits with SAH\n";
                
				std::vector<VectorTriangle> pre;
				std::vector<VectorTriangle> post;

				// sort into pre-split and post-split
				for (int k = 0; k < sah.triangles.size(); k++) {
					for(int l = 0; l < sah.triangles[k].size(); l++) {
						if(k <= bestSplit) {
							pre.push_back(vtriangles[sah.triangles[k][l]]); 
						}
						else 
							post.push_back(vtriangles[sah.triangles[k][l]]);
					}
				}

				// insert reordered triangles back into the triangles vector
				int nreplaced = start;
				for (int i = 0; i < pre.size(); i++) {
					vtriangles[nreplaced] = pre[i];
					nreplaced++;
				}

				for (int j = 0; j < post.size(); j++) {
                    vtriangles[nreplaced] = post[j];
					nreplaced++;
				}
			}

			// create leaf
			else {
				createLeaf(node,bounds,vtriangles,orderedvtriangles,start,end);
				return node;
			}			

			node->children[0] = recBuildTree(start,mid);
			node->children[1] = recBuildTree(mid,end);
			node->bbox = combine(node->children[0]->bbox,
				node->children[1]->bbox);
			node->axis = axis;
            node->nPrimitives = 0;
            return node;
		}
	}
    
    return node;
}

int BoundingVolumeHierarchy::flattenTree(TreeNode *node, int *offset) {

    int k = *offset;
//    cout << "k: " << k << "\n";
    
    bvh[k].minbounds = (cl_float3){node->bbox.minbounds[0],
        node->bbox.minbounds[1],node->bbox.minbounds[2]};
    bvh[k].maxbounds = (cl_float3){node->bbox.maxbounds[0],
        node->bbox.maxbounds[1],node->bbox.maxbounds[2]};
    
    uint32_t myOffset = (*offset)++;
    
    if (node->nPrimitives > 0) {
//        Assert(!node->children[0] && !node->children[1]);
        bvh[k].offset = (cl_short)node->index;
        bvh[k].nPrimitives = (cl_short)node->nPrimitives;
//        cout << bvh[k].nPrimitives << " " << node->nPrimitives << "\n";
    }
    else {
        bvh[k].axis = (cl_short)node->axis;
        bvh[k].nPrimitives = 0;
        flattenTree(node->children[0], offset);
        bvh[k].secondChildOffset = flattenTree(node->children[1],
                                               offset);
    }
    
    return myOffset;
}
