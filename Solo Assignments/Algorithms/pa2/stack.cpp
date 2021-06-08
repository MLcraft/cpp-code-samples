#include "stack.h"
#include <iostream>

int Stack::size() {
  return st.size();
}

bool Stack::isEmpty() {
  if (size() == 0)
    return true;
  return false;
}

void Stack::push(Point p) {
  st.push_back(p);
}

Point Stack::peek() {
  if (!isEmpty())
    return st[st.size() - 1];
  return Point();
}

Point Stack::pop() {
  Point temp = peek();
  st.erase(st.begin() + size() - 1);
  return temp;
}
