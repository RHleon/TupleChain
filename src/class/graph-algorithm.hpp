#ifndef _GRAPH_ALGORITHM_H
#define _GRAPH_ALGORITHM_H

#include <vector>
#include <cstdint>

#define MPC

struct GraphEdge {
  int from;
  int to;
  int weight;
  GraphEdge() {};
  GraphEdge(int x, int y, int z = 1)
  {
     from = x;
     to = y;
     weight = z;
  }
};

std::vector<std::vector<int>>
minimumPathCovering(int nNodes, const std::vector<GraphEdge>& edges);

std::vector<std::vector<int>>
balanceHints(const std::vector<int>& hints, int nCores); 

#endif // _GRAPH_ALGORITHM_H
