//复杂度：O(n*e)
#include <graph-algorithm.hpp>
#ifdef MPC
#pragma comment(linker,"/STACK:102400000,102400000")
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <string>
#include <math.h>
#include <queue>
#include <stack>
#include <map>
#include <set>
using namespace std;
typedef long long ll;   //记得必要的时候改成无符号
const int maxn=100005;   //点数
const int maxm=100005;   //边数
struct EdgeNode
{
    int to;
    int next;
}edge[maxm];
int head[maxn],cnt;
void add(int x,int y)
{
    edge[cnt].to=y;edge[cnt].next=head[x];head[x]=cnt++;
}

int n,m,v[maxn],pre[maxn],pip[maxn];

void init()
{
    cnt=1;
    memset(head,-1,sizeof(head));
}

bool dfs(int x)
{
    int y;
    for(int i=head[x];i!=-1;i=edge[i].next)
    {
        y=edge[i].to;
        if(!v[y])
        {
            v[y]=1;
            if(!pre[y]||dfs(pre[y]))
            {
                pre[y]=x;
                pip[x]=y;
                return true;
            }
        }
    }
    return false;
}

int erfenpipei()
{
    int sum=0;
    memset(pre,0,sizeof(pre));
    memset(pip,0,sizeof(pip));
    for(int i=1;i<=n;i++)
    {
        memset(v,0,sizeof(v));
        if(dfs(i))
            sum++;
    }
    return sum;
}

std::vector<std::vector<int>>
minimumPathCovering(int nNodes, const std::vector<GraphEdge>& edges)
{
  n = nNodes; m = edges.size();
  init();

  std::vector<std::unordered_map<int, bool> > validEdges(nNodes);
  // increase node id by 1
  for (const auto& e : edges) {
    add(e.from + 1, e.to + 1);
    validEdges[e.from][e.to] = true;
  }
  auto bubbleSort = [&validEdges] (std::vector<int>& path) {
    for (int i = 0; i < path.size() - 1; i++) { // times
      for (int j = 0; j < path.size() - i - 1; j++) { // position
	if (validEdges[path[j+1]][path[j]]) {
	  int temp = path[j];
	  path[j] = path[j + 1];
	  path[j + 1] = temp;
	}
      }
    }
  };

  int ans = erfenpipei();

  std::vector<std::vector<int>> paths;
  std::vector<int> v(n + 10, 0);

  for(int i = 1; i <= n; ++ i){
    if(!v[i]) {
      std::vector<int> path;

      v[i] = 1; path.push_back(i);
      for (int j = pre[i]; j; j = pre[j]) {
	// printf("pre: %d\n", j);
	v[j] = 1; path.push_back(j);
      }
      for (int j = pip[i]; j; j = pip[j]) {
	// printf("pip: %d\n", j);
	v[j] = 1; path.push_back(j);
      }

      // decrease node by 1
      int cv = path.size(); for (int j = 0; j < cv; j ++) path[j] --;
      bubbleSort(path);
      
      paths.push_back(path);
    }
  }

  return paths;
}
#endif
