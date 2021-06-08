#include "stats.h"

long stats::getSum(char channel, pair<int,int> ul, pair<int,int> lr) {
    if (ul.first == 0 && ul.second == 0) {
      if (channel == 'r')
        return sumRed[lr.first][lr.second];
      else if (channel == 'g')
        return sumGreen[lr.first][lr.second];
      else
        return sumBlue[lr.first][lr.second];
    } else if (ul.first == 0) {
      if (channel == 'r')
        return sumRed[lr.first][lr.second] - (sumRed[lr.first][ul.second - 1]);
      else if (channel == 'g')
        return sumGreen[lr.first][lr.second] - (sumGreen[lr.first][ul.second - 1]);
      else
        return sumBlue[lr.first][lr.second] - (sumBlue[lr.first][ul.second - 1]);

    } else if (ul.second == 0) {
      if (channel == 'r')
        return sumRed[lr.first][lr.second] - (sumRed[ul.first - 1][lr.second]);
      else if (channel == 'g')
        return sumGreen[lr.first][lr.second] - (sumGreen[ul.first - 1][lr.second]);
      else
        return sumBlue[lr.first][lr.second] - (sumBlue[ul.first - 1][lr.second]);

    } else {
      if (channel == 'r')
        return sumRed[lr.first][lr.second] - (sumRed[ul.first - 1][lr.second]) - (sumRed[lr.first][ul.second - 1]) + (sumRed[ul.first - 1][ul.second - 1]);
      else if (channel == 'g')
        return sumGreen[lr.first][lr.second] - (sumGreen[ul.first - 1][lr.second]) - (sumGreen[lr.first][ul.second - 1]) + (sumGreen[ul.first - 1][ul.second - 1]);
      else
        return sumBlue[lr.first][lr.second] - (sumBlue[ul.first - 1][lr.second]) - (sumBlue[lr.first][ul.second - 1]) + (sumBlue[ul.first - 1][ul.second - 1]);

    }
}

long stats::getSumSq(char channel, pair<int,int> ul, pair<int,int> lr) {
  if (ul.first == 0 && ul.second == 0) {
    if (channel == 'r')
      return sumsqRed[lr.first][lr.second];
    else if (channel == 'g')
      return sumsqGreen[lr.first][lr.second];
    else
      return sumsqBlue[lr.first][lr.second];
  } else if (ul.first == 0) {
    if (channel == 'r')
      return sumsqRed[lr.first][lr.second] - (sumsqRed[lr.first][ul.second - 1]);
    else if (channel == 'g')
      return sumsqGreen[lr.first][lr.second] - (sumsqGreen[lr.first][ul.second - 1]);
    else
      return sumsqBlue[lr.first][lr.second] - (sumsqBlue[lr.first][ul.second - 1]);

  } else if (ul.second == 0) {
    if (channel == 'r')
      return sumsqRed[lr.first][lr.second] - (sumsqRed[ul.first - 1][lr.second]);
    else if (channel == 'g')
      return sumsqGreen[lr.first][lr.second] - (sumsqGreen[ul.first - 1][lr.second]);
    else
      return sumsqBlue[lr.first][lr.second] - (sumsqBlue[ul.first - 1][lr.second]);

  } else {
    if (channel == 'r')
      return sumsqRed[lr.first][lr.second] - (sumsqRed[ul.first - 1][lr.second]) - (sumsqRed[lr.first][ul.second - 1]) + (sumsqRed[ul.first - 1][ul.second - 1]);
    else if (channel == 'g')
      return sumsqGreen[lr.first][lr.second] - (sumsqGreen[ul.first - 1][lr.second]) - (sumsqGreen[lr.first][ul.second - 1]) + (sumsqGreen[ul.first - 1][ul.second - 1]);
    else
      return sumsqBlue[lr.first][lr.second] - (sumsqBlue[ul.first - 1][lr.second]) - (sumsqBlue[lr.first][ul.second - 1]) + (sumsqBlue[ul.first - 1][ul.second - 1]);

  }
}

stats::stats(PNG & im) {

  for (int x = 0; x < im.width(); x++) {

    vector<long> dataRed;
    vector<long> dataGreen;
    vector<long> dataBlue;
    vector<long> datasqRed;
    vector<long> datasqGreen;
    vector<long> datasqBlue;

    long sumRedChannel = 0;
    long sumGreenChannel = 0;
    long sumBlueChannel = 0;
    long sumsqRedChannel = 0;
    long sumsqGreenChannel = 0;
    long sumsqBlueChannel = 0;
    for (int y = 0; y < im.height(); y++) {
        RGBAPixel * pixel = im.getPixel(x, y);

        sumRedChannel += pixel->r;

        sumGreenChannel += pixel->g;

        sumBlueChannel += pixel->b;

        sumsqRedChannel += pixel->r * pixel->r;

        sumsqGreenChannel += pixel->g * pixel->g;

        sumsqBlueChannel += pixel->b * pixel->b;
        if (x == 0) {
          dataRed.push_back(sumRedChannel);
          dataGreen.push_back(sumGreenChannel);
          dataBlue.push_back(sumBlueChannel);
          datasqRed.push_back(sumsqRedChannel);
          datasqGreen.push_back(sumsqGreenChannel);
          datasqBlue.push_back(sumsqBlueChannel);
        } else {
          dataRed.push_back(sumRedChannel + sumRed[x - 1][y]);
          dataGreen.push_back(sumGreenChannel + sumGreen[x - 1][y]);
          dataBlue.push_back(sumBlueChannel + sumBlue[x - 1][y]);
          datasqRed.push_back(sumsqRedChannel + sumsqRed[x - 1][y]);
          datasqGreen.push_back(sumsqGreenChannel + sumsqGreen[x - 1][y]);
          datasqBlue.push_back(sumsqBlueChannel + sumsqBlue[x - 1][y]);
        }
    }

    sumRed.push_back(dataRed);
    sumGreen.push_back(dataGreen);
    sumBlue.push_back(dataBlue);
    sumsqRed.push_back(datasqRed);
    sumsqGreen.push_back(datasqGreen);
    sumsqBlue.push_back(datasqBlue);

  }
}

long stats::getScore(pair<int,int> ul, pair<int,int> lr) {
  long numPixels = rectArea(ul, lr);
  long sumOfRed = getSumSq('r', ul, lr) - ((getSum('r', ul, lr) * getSum('r', ul, lr))/numPixels);
  long sumOfGreen = getSumSq('g', ul, lr) - ((getSum('g', ul, lr) * getSum('g', ul, lr))/numPixels);
  long sumOfBlue = getSumSq('b', ul, lr) - ((getSum('b', ul, lr) * getSum('b', ul, lr))/numPixels);
  return sumOfRed + sumOfGreen + sumOfBlue;
}

RGBAPixel stats::getAvg(pair<int,int> ul, pair<int,int> lr) {
  long numPixels = rectArea(ul, lr);
  long averageRed = (getSum('r', ul, lr))/numPixels;
  long averageGreen = (getSum('g', ul, lr))/numPixels;
  long averageBlue = (getSum('b', ul, lr))/numPixels;

  return RGBAPixel(averageRed, averageGreen, averageBlue);
}

long stats::rectArea(pair<int,int> ul, pair<int,int> lr) {
    return (lr.first - ul.first + 1) * (lr.second - ul.second + 1);
}
