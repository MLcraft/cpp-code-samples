#include "convexintersection.h"
#include "convexhull.h"
#include <iostream>
bool inside(Point p1, Point sp1, Point sp2) {
  return !ccw(sp1, p1, sp2);
}
Point computeIntersection(Point s1, Point s2, Point i1, Point i2) {
  double a1 = ((s2.y - s1.y)/(s2.x - s1.x));
  double a2 = ((i2.y - i1.y)/(i2.x - i1.x));
  double x1 = s1.x;
  double x2 = i1.x;
  double y1 = s1.y;
  double y2 = i1.y;
  double x;
  double y;
  if ((s2.x - s1.x) != 0 && (i2.x - i1.x) != 0) {
    x = (a1 * x1 - a2 * x2 - y1 + y2)/(a1 - a2);
    y = a1 * (x - x1) + y1;
  } else if ((s2.x - s1.x) == 0) {
    x = s2.x;
    y = y2 + a2 * (x - x2);
  } else {
    x = i2.x;
    y = y1 + a1 * (x - x1);
  }
  return Point(x, y);
}
vector<Point> getConvexIntersection(vector<Point>& poly1, vector<Point>& poly2) {
    vector<Point> result = poly2;
    poly1.insert(poly1.end(), poly1[0]);
    for (int i = 0; i < poly1.size() - 1; i++) {
       vector<Point> v = result;
       result.erase(result.begin(), result.end());
       if (v.size() == 0) {
         return v;
       }
       Point last = v[v.size()-1];
       for (int j = 0; j < v.size(); j++) {
         Point p = v[j];
          if (inside(p, poly1[i], poly1[i+1])) {
             if (!inside(last, poly1[i], poly1[i+1])) {
                result.push_back(computeIntersection(last,p,poly1[i], poly1[i+1]));
              }
             result.push_back(p);
           }
          else if (inside(last, poly1[i], poly1[i+1])) {
             result.push_back(computeIntersection(last,p,poly1[i], poly1[i+1]));
           }
          last = p;
       }
    }
    return result;
}
