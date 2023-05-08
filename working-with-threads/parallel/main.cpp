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

pthread_t threads[3];

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

struct BuffChannelArg {
	char* buffer;
	WORD** channel;
	int end;
};

void* get_channels_from_BMP24(void* arg){
	struct BuffChannelArg* channel_arg = (struct BuffChannelArg*)arg;
  int extra = cols % 4, end = channel_arg->end, count = 1;
	for (int i = 0; i < rows; i++){
    count += extra;
    for (int j = cols - 1; j >= 0; j--){
      channel_arg->channel[i][j] = (WORD)channel_arg->buffer[end - count] % 256;
			count+=3;
		}
  }
	pthread_exit(NULL);
}

void getPixlesFromBMP24(int end, char *fileReadBuffer, WORD*** pixels)
{
  int k;
	struct BuffChannelArg channels_args[3];
	for(k = 0; k < 3; k++){
		channels_args[k].buffer = fileReadBuffer;
		channels_args[k].channel = pixels[k];
		channels_args[k].end = end - k;
	}
	pthread_create(&threads[0], NULL, &get_channels_from_BMP24, (void *)(channels_args+0));
	pthread_create(&threads[1], NULL, &get_channels_from_BMP24, (void *)(channels_args+1));
	pthread_create(&threads[2], NULL, &get_channels_from_BMP24, (void *)(channels_args+2));
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	pthread_join(threads[2], NULL);
}

void* write_channels_buff(void* arg){
	struct BuffChannelArg* channel_arg = (struct BuffChannelArg*)arg;
  int extra = cols % 4, end = channel_arg->end, count = 1;
	for (int i = 0; i < rows; i++){
    count += extra;
    for (int j = cols - 1; j >= 0; j--){
      channel_arg->buffer[end - count] = channel_arg->channel[i][j];
			count+=3;
		}
  }
	pthread_exit(NULL);
}

void writeOutBmp24(char *fileBuffer, const char *nameOfFileToCreate, int bufferSize,  WORD*** pixels)
{
  std::ofstream write(nameOfFileToCreate);
  if (!write)
  {
    cout << "Failed to write " << nameOfFileToCreate << endl;
    return;
  }
  int k;
  int extra = cols % 4;
	struct BuffChannelArg channels_args[3];
	for(k = 0; k < 3; k++){
		channels_args[k].buffer = fileBuffer;
		channels_args[k].channel = pixels[k];
		channels_args[k].end = bufferSize - k;
	}
	pthread_create(&threads[0], NULL, &write_channels_buff, (void *)(channels_args+0));
	pthread_create(&threads[1], NULL, &write_channels_buff, (void *)(channels_args+1));
	pthread_create(&threads[2], NULL, &write_channels_buff, (void *)(channels_args+2));
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	pthread_join(threads[2], NULL);

  write.write(fileBuffer, bufferSize);
}

int box_r[9] = {-1,-1,-1,0,0,0,1,1,1};
int box_c[9] = {-1,0,1,-1,0,1,-1,0,1};

void* smoothing_filter_on_channel(void* arg){
  WORD** channel = (WORD**)arg;
	int mean_box;
	for(int i = 0; i < rows; i++)
		for(int j = 0; j < cols; j++){
			mean_box = 0;
			for(int t = 0; t < 9; t++)
				if(0 <= i + box_r[t] && i + box_r[t] < rows && 0 <= j + box_c[t] && j + box_c[t] < cols)
					mean_box += channel[i + box_r[t]][j + box_c[t]];
			mean_box /= 9;
			channel[i][j] = (WORD)mean_box;
		}
	pthread_exit(NULL);
}

void apply_smoothing_filter(WORD***& pixels){
	pthread_create(&threads[0], NULL, &smoothing_filter_on_channel, (void *)pixels[0]);
	pthread_create(&threads[1], NULL, &smoothing_filter_on_channel, (void *)pixels[1]);
	pthread_create(&threads[2], NULL, &smoothing_filter_on_channel, (void *)pixels[2]);
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	pthread_join(threads[2], NULL);
}

typedef struct ChannelPortion {
  WORD*** pixels;
  int ch_num;
  int r_start;
  int r_end;
  int c_start;
  int c_end;
} ChannelPortion;

void* sepia_filter_on_portion(void* arg){
  int red, green, blue;
  ChannelPortion* pr_inf = (ChannelPortion*)arg;
  for(int i = pr_inf->r_start; i <= pr_inf->r_end; i++)
    for(int j = pr_inf->c_start; j <= pr_inf->c_end; j++){
      red = (WORD)min(255.0, (0.393 * pr_inf->pixels[0][i][j] + 0.769 * pr_inf->pixels[1][i][j] + 0.189 * pr_inf->pixels[2][i][j]));
      green = (WORD)min(255.0, (0.349 * pr_inf->pixels[0][i][j] + 0.686 * pr_inf->pixels[1][i][j] + 0.168 * pr_inf->pixels[2][i][j]));
      blue = (WORD)min(255.0, (0.272 * pr_inf->pixels[0][i][j] + 0.534 * pr_inf->pixels[1][i][j] + 0.131 * pr_inf->pixels[2][i][j]));
      pr_inf->pixels[0][i][j] = red; pr_inf->pixels[1][i][j] = green; pr_inf->pixels[2][i][j] = blue;
    }
  pthread_exit(NULL);
}

void apply_sepia_filter(WORD***& pixels){
  int c = 0, i, j, POR_AMOUNT;
  pthread_t thread_id;
  ChannelPortion temp_info;
  vector<pthread_t> thread_ids;
  vector<ChannelPortion> portions;

  temp_info.pixels = pixels;
  portions.push_back(temp_info); c++;
  POR_AMOUNT = !(rows % 4) ? (int)(rows / 4) : (int)(1.0 * rows / 4) + 1;
  for(i = 0; i < rows; i+= POR_AMOUNT)
    for(j = 0; j < cols; j+= POR_AMOUNT){
      temp_info.r_start = i;  temp_info.r_end = min(rows-1, i+POR_AMOUNT-1);
      temp_info.c_start = j;  temp_info.c_end = min(cols-1, j+POR_AMOUNT-1);
      portions.push_back(temp_info);
      pthread_create(&thread_id, NULL, &sepia_filter_on_portion, (void *)&(portions[c++]));
      thread_ids.push_back(thread_id); thread_id = 0;
    }
  for(i = 1; i < c; i++){
    pthread_join(thread_ids[i-1], NULL);
  }
}

void* mean_filter_on_channel(void* arg){
  WORD** channel = (WORD**)arg;
  double mean = 0;
  for(int i = 0; i < rows; i++)
    for(int j = 0; j < cols; j++)
      mean += channel[i][j];
  mean /= (rows * cols);
  for(int i = 0; i < rows; i++)
    for(int j = 0; j < cols; j++)
      channel[i][j] = (WORD)(channel[i][j] * 0.4 + mean * 0.6);
  pthread_exit(NULL);
}

void apply_mean_filter(WORD***& pixels){
	pthread_create(&threads[0], NULL, &mean_filter_on_channel, (void *)pixels[0]);
	pthread_create(&threads[1], NULL, &mean_filter_on_channel, (void *)pixels[1]);
	pthread_create(&threads[2], NULL, &mean_filter_on_channel, (void *)pixels[2]);
	pthread_join(threads[0], NULL);
	pthread_join(threads[1], NULL);
	pthread_join(threads[2], NULL);
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
  getPixlesFromBMP24(bufferSize, fileBuffer, pixels);
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