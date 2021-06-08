#include "convexhull.h"
#include <iostream>

using namespace std;
void sortByAngle(vector<Point>& v) {
  sortSmallest(v);
  sortAngles(v);
}

void sortSmallest(vector<Point>& v) {
  Point smallest = v[0];
  int index = 0;
  int size = v.size();
  for (int i = 1; i < size; i++) {
    if (v[i].y < smallest.y) {
         index = i;
         smallest = v[i];
    } else {
         if (v[i].y == smallest.y) {
           if (v[i].x < smallest.x) {
             index = i;
             smallest = v[i];
           }
       }
     }
  }
  Point temp = v[index];
  v[index] = v[0];
  v[0] = temp;
}

void sortAngles(vector<Point>& v) {
  int size = v.size();
  for (int i=size-1; i>0; i--) {
    for (int j=2; j<=i; j++) {
      if (acos((v[j - 1].x - v[0].x) / sqrt((v[j - 1].x - v[0].x) * (v[j - 1].x - v[0].x) + (v[j - 1].y - v[0].y) * (v[j - 1].y - v[0].y))) > acos((v[j].x - v[0].x) / sqrt((v[j].x - v[0].x) * (v[j].x - v[0].x) + (v[j].y - v[0].y) * (v[j].y - v[0].y)))) {
        Point t = v[j];
        v[j] = v[j-1];
        v[j-1] = t;
      }
    }
  }
}
bool ccw(Point p1, Point p2, Point p3) {
  double crossproduct = (p2.x - p1.x) * (p3.y - p1.y) - (p2.y - p1.y) * (p3.x - p1.x);
  return crossproduct > 0;
}

vector<Point> getConvexHull(vector<Point>& v) {
  sortByAngle(v);
  int size = v.size();
  Stack s1;
  v.insert(v.end(), v[0]);
  s1.push(v[0]);
  s1.push(v[1]);
  for (int i = 2; i <= size; i++) {
      Point top = s1.pop();
      bool convex = ccw(s1.peek(), top, v[i]);
      s1.push(top);
      while (!convex) {
        int sizes1 = s1.size();
        s1.pop();
        if (sizes1 >= 2) {
          Point top = s1.pop();
          convex = ccw(s1.peek(), top, v[i]);
          s1.push(top);
        }
        else {
          break;
        }
      }
      s1.push(v[i]);

  }
  v.erase(v.begin(), v.end());
  s1.pop();
  size = s1.size();
  for (int i = 0; i < size; i++) {
    v.insert(v.begin(), s1.pop());
  }
  return v;
}
