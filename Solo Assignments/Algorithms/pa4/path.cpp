#include "path.h"
#include <queue>
#include <stack>
using namespace std;

path::path(const PNG & im, pair<int,int> s, pair<int,int> e)
   :start(s),end(e),image(im){
    BFS();
}

void path::BFS(){
	// initialize working vectors
	vector<vector<bool>> V(image.height(), vector<bool> (image.width(),false));
	vector<vector<pair<int,int>>> P(image.height(), vector<pair<int,int>> (image.width(),end));

  /* your code here */
  queue<pair<int, int>> q;
  V[start.second][start.first] = true;
  q.push(start);
  while (!q.empty()) {
    pair<int, int> v1 = q.front();
    q.pop();
    vector<pair<int, int>> nbrs = neighbors(v1);
    for (int i = 0; i < nbrs.size(); i++) {
      if (good(V, v1, nbrs[i])) {
        q.push(nbrs[i]);
        P[nbrs[i].second][nbrs[i].first] = v1;
        V[nbrs[i].second][nbrs[i].first] = true;
      }
    }
  }
	pathPts = assemble(P,start,end);
}

PNG path::render(){

    /* your code here */
    PNG output(image);
    for (int i = 0; i < length(); i++) {
      int x = pathPts[i].first;
      int y = pathPts[i].second;
      RGBAPixel * pixel = output.getPixel(x, y);
      *pixel = RGBAPixel(255, 0, 0);
    }
    return output;

}

vector<pair<int,int>> path::getPath() { return pathPts;}

int path::length() { return pathPts.size();}

bool path::good(vector<vector<bool>> & v, pair<int,int> curr, pair<int,int> next){
    /* your code here */
    if (next.first < 0 || next.second < 0) {
      return false;
    }
    else if (next.second >= image.height() || next.first >= image.width()) {
      return false;
    }
    else if (v[next.second][next.first]) {
      return false;
    }
    else if (closeEnough(*(image.getPixel(curr.first, curr.second)), *(image.getPixel(next.first, next.second))))
      return true;
    else {
      return false;
    }
}

vector<pair<int,int>> path::neighbors(pair<int,int> curr) {
	vector<pair<int,int>> n (4);

    /* your code here */
  pair<int, int> n1(curr.first - 1, curr.second);
  pair<int, int> n2(curr.first, curr.second + 1);
  pair<int, int> n3(curr.first + 1, curr.second);
  pair<int, int> n4(curr.first, curr.second - 1);



  n[0] = n1;
  n[1] = n2;
  n[2] = n3;
  n[3] = n4;

  return n;
}

vector<pair<int,int>> path::assemble(vector<vector<pair<int,int>>> & p,pair<int,int> s, pair<int,int> e) {

    /* hint, gold code contains the following line:
	stack<pair<int,int>> S; */

    /* your code here */
  stack<pair<int,int>> S;
  vector<pair<int, int>> v1;
  pair<int, int> p1 = e;
  while (p1 != s) {
    S.push(p1);
    p1 = p[p1.second][p1.first];
    if (p1 == end) {
      break;
    }
  }
  S.push(s);
  while (!S.empty()) {
    v1.push_back(S.top());
    S.pop();
  }
  return v1;
}

bool path::closeEnough(RGBAPixel p1, RGBAPixel p2){
   int dist = (p1.r-p2.r)*(p1.r-p2.r) + (p1.g-p2.g)*(p1.g-p2.g) +
               (p1.b-p2.b)*(p1.b-p2.b);

   return (dist <= 80);
}
