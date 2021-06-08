#include "block.h"
#include <iostream>
using namespace std;

int Block::width() const{
   return data.size();
}
int Block::height() const{
   return data[0].size();
}

void Block::render(PNG & im, int upLeftX, int upLeftY) const {
   for (int x = upLeftX; x < upLeftX + width(); x++) {
     vector < HSLAPixel > col = data[x - upLeftX];
     for (int y = upLeftY; y < upLeftY + height(); y++) {
       *im.getPixel(x, y) = col[y - upLeftY];
     }
   }

}

void Block::build(PNG & im, int upLeftX, int upLeftY, int cols, int rows) {
   vector < vector < HSLAPixel > > data1(cols);
   for (int x = upLeftX; x < upLeftX + cols; x++) {
     vector < HSLAPixel > col(rows);
     for (int y = upLeftY; y < upLeftY + rows; y++) {
       HSLAPixel * pixel = im.getPixel(x, y);
       col[y - upLeftY] = *pixel;
     }
     data1[x - upLeftX] = col;
   }
   data = data1;
}
