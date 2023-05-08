#include <iostream>
#include <unistd.h>
#include <fstream>
#include <vector>
#include <chrono>

using namespace std;

using namespace std::chrono;
using std::ifstream;
using std::ofstream;

#pragma pack(1)
#pragma once

typedef int LONG;
typedef unsigned short WORD;
typedef unsigned int DWORD;


typedef struct tagBITMAPFILEHEADER
{
  WORD bfType;
  DWORD bfSize;
  WORD bfReserved1;
  WORD bfReserved2;
  DWORD bfOffBits;
} BITMAPFILEHEADER, *PBITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
  DWORD biSize;
  LONG biWidth;
  LONG biHeight;
  WORD biPlanes;
  WORD biBitCount;
  DWORD biCompression;
  DWORD biSizeImage;
  LONG biXPelsPerMeter;
  LONG biYPelsPerMeter;
  DWORD biClrUsed;
  DWORD biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

int rows;
int cols;

void allocate_pixels(WORD***& pixels){
  pixels = new WORD**[3];
  for(int k = 0; k < 3; k++){
    pixels[k] = new WORD*[rows];
    for(int i = 0; i < rows; i++){
      pixels[k][i] = new WORD[cols];
    }
  }
}
void delete_pixels(WORD***& pixels){
  for(int k = 0; k < 3; k++){
    for(int i = 0; i < rows; i++){
        delete [] pixels[k][i];
    }
    delete [] pixels[k];
  }
  delete [] pixels;
}

bool fillAndAllocate(char *&buffer, const char *fileName, int &rows, int &cols, int &bufferSize)
{
  std::ifstream file(fileName);

  if (file)
  {
    file.seekg(0, std::ios::end);
    std::streampos length = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer = new char[length];
    file.read(&buffer[0], length);

    PBITMAPFILEHEADER file_header;
    PBITMAPINFOHEADER info_header;

    file_header = (PBITMAPFILEHEADER)(&buffer[0]);
    info_header = (PBITMAPINFOHEADER)(&buffer[0] + sizeof(BITMAPFILEHEADER));
    rows = info_header->biHeight;
    cols = info_header->biWidth;
    bufferSize = file_header->bfSize;
    return 1;
  }
  else
  {
    cout << "File" << fileName << " doesn't exist!" << endl;
    return 0;
  }
}

void getPixlesFromBMP24(int end, int rows, int cols, char *fileReadBuffer, WORD*** pixels)
{
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--)
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          // fileReadBuffer[end - count] is the red value
          pixels[k][i][j] = (WORD)fileReadBuffer[end - count] % 256;
          break;
        case 1:
          // fileReadBuffer[end - count] is the green value
          pixels[k][i][j] = (WORD)fileReadBuffer[end - count] % 256;
          break;
        case 2:
          // fileReadBuffer[end - count] is the blue value
          pixels[k][i][j] = (WORD)fileReadBuffer[end - count] % 256;
          break;
        // go to the next position in the buffer
        }
        count++;
      }
  }
}

void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize,  WORD*** pixels)
{
  std::ofstream write(nameOfFileToCreate);
  if (!write)
  {
    cout << "Failed to write " << nameOfFileToCreate << endl;
    return;
  }
  int count = 1;
  int extra = cols % 4;
  for (int i = 0; i < rows; i++)
  {
    count += extra;
    for (int j = cols - 1; j >= 0; j--){
      for (int k = 0; k < 3; k++)
      {
        switch (k)
        {
        case 0:
          // write red value in fileBuffer[bufferSize - count]
          fileBuffer[bufferSize - count] = pixels[k][i][j];
          break;
        case 1:
          // write green value in fileBuffer[bufferSize - count]
          fileBuffer[bufferSize - count] = pixels[k][i][j];
          break;
        case 2:
          // write blue value in fileBuffer[bufferSize - count]
          fileBuffer[bufferSize - count] = pixels[k][i][j];
          break;
        // go to the next position in the buffer
        }
        count++;
      }
      
    }
  }
  write.write(fileBuffer, bufferSize);
}

int box_r[9] = {-1,-1,-1,0,0,0,1,1,1};
int box_c[9] = {-1,0,1,-1,0,1,-1,0,1};

void apply_smoothing_filter(WORD***& pixels){
  int mean_box;
  for(int k = 0; k < 3; k++){
    for(int i = 0; i < rows; i++)
      for(int j = 0; j < cols; j++){
        mean_box = 0;
        for(int t = 0; t < 9; t++)
          if(0 <= i + box_r[t] && i + box_r[t] < rows && 0 <= j + box_c[t] && j + box_c[t] < cols)
            mean_box += pixels[k][i + box_r[t]][j + box_c[t]];
        mean_box /= 9;
        pixels[k][i][j] = (WORD)mean_box;
      }
  }
}

void apply_sepia_filter(WORD***& pixels){
  for(int i = 0; i < rows; i++)
    for(int j = 0; j < cols; j++){
      int red, green, blue;
      red = (WORD)min(255.0, (0.393 * pixels[0][i][j] + 0.769 * pixels[1][i][j] + 0.189 * pixels[2][i][j]));
      green = (WORD)min(255.0, (0.349 * pixels[0][i][j] + 0.686 * pixels[1][i][j] + 0.168 * pixels[2][i][j]));
      blue = (WORD)min(255.0, (0.272 * pixels[0][i][j] + 0.534 * pixels[1][i][j] + 0.131 * pixels[2][i][j]));
      pixels[0][i][j] = red; pixels[1][i][j] = green; pixels[2][i][j] = blue;
    }
}

void mean_filter_on_channel(WORD**& channel){
  double mean = 0;
  for(int i = 0; i < rows; i++)
    for(int j = 0; j < cols; j++)
      mean += channel[i][j];
  mean /= (rows * cols);
  for(int i = 0; i < rows; i++)
    for(int j = 0; j < cols; j++)
      channel[i][j] = (WORD)min(255.0, channel[i][j] * 0.4 + mean * 0.6);
}

void apply_mean_filter(WORD***& pixels){
  mean_filter_on_channel(pixels[0]);
  mean_filter_on_channel(pixels[1]);
  mean_filter_on_channel(pixels[2]);
}

void cross_on_channel(WORD** channel){
  int i;
  for(i = 0; i < rows-1; i++)
    channel[i][i] = channel[i][i+1] = channel[i+1][i] = 255;
  channel[i][i] = 255;
  for(; i > 0; i--)
    channel[i][rows-1-i] = channel[i][rows-i] = channel[i-1][rows-1-i] = 255;
  channel[i][rows-1] = 255;
}

void apply_cross_filter(WORD*** pixels){
  cross_on_channel(pixels[0]);
  cross_on_channel(pixels[1]);
  cross_on_channel(pixels[2]);
}

int main(int argc, char *argv[])
{
  char *fileBuffer;
  int bufferSize;
  char *fileName = argv[1];
	auto time_p = high_resolution_clock::now();
  if (!fillAndAllocate(fileBuffer, fileName, rows, cols, bufferSize))
  {
    cout << "File read error" << endl;
    return 1;
  }
  WORD ***pixels;
  allocate_pixels(pixels);
  getPixlesFromBMP24(bufferSize, rows, cols, fileBuffer, pixels);
  apply_smoothing_filter(pixels);
  apply_sepia_filter(pixels);
  apply_mean_filter(pixels);
  apply_cross_filter(pixels);
  writeOutBmp24(fileBuffer, "output.bmp", bufferSize, pixels);
  delete_pixels(pixels);
  auto exe_time = duration_cast<milliseconds>(high_resolution_clock::now() - time_p).count();
  cout << exe_time << '\n';
  return 0;
}