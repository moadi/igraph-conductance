//
//  community.h
//  igraph-conductance
//
//  Created by moadi on 11/1/14.
//  Copyright (c) 2014 PSU. All rights reserved.
//

#ifndef igraph_conductance_community_h
#define igraph_conductance_community_h

#include <vector>

class Community
{
    public:
        std::vector<int> orig_nodes;
        std::vector<int> neighbors;
};

#endif
