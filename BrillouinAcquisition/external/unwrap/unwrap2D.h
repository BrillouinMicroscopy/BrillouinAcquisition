// 2D phase unwrapping, modified for inclusion in scipy by Gregor Thalhammer

//This program was written by Munther Gdeisat and Miguel Arevallilo Herraez to program the two-dimensional unwrapper
//entitled "Fast two-dimensional phase-unwrapping algorithm based on sorting by
//reliability following a noncontinuous path"
//by  Miguel Arevallilo Herraez, David R. Burton, Michael J. Lalor, and Munther A. Gdeisat
//published in the Journal Applied Optics, Vol. 41, No. 35, pp. 7437, 2002.
//This program was written by Munther Gdeisat, Liverpool John Moores University, United Kingdom.
//Date 26th August 2007
//The wrapped phase map is assumed to be of double data type. The resultant unwrapped phase map is also of double type.
//The mask is of byte data type.
//When the mask is 1 this means that the pixel is valid
//When the mask is 0 this means that the pixel is invalid (noisy or corrupted pixel)
//This program takes into consideration the image wrap around problem encountered in MRI imaging.

#ifndef UNWRAP_H
#define UNWRAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//TODO: remove global variables
//TODO: make thresholds independent

static double PI = 3.141592654f;
static double TWOPI = 6.283185307f;

#define NOMASK 0
#define MASK 1

typedef struct
{
  double mod;
  int x_connectivity;
  int y_connectivity;
  int no_of_edges;
} params_t;

//PIXELM information
struct PIXELM
{
  int increment;		//No. of 2*pi to add to the pixel to unwrap it
  int number_of_pixels_in_group;//No. of pixel in the pixel group
  double value;			//value of the pixel
  double reliability;
  unsigned char input_mask;	//0 pixel is masked. NOMASK pixel is not masked
  unsigned char extended_mask;	//0 pixel is masked. NOMASK pixel is not masked
  int group;			//group No.
  int new_group;
  struct PIXELM *head;		//pointer to the first pixel in the group in the linked list
  struct PIXELM *last;		//pointer to the last pixel in the group
  struct PIXELM *next;		//pointer to the next pixel in the group
};

typedef struct PIXELM PIXELM;

//the EDGE is the line that connects two pixels.
//if we have S pixels, then we have S horizontal edges and S vertical edges
struct EDGE
{
  double reliab;			//reliabilty of the edge and it depends on the two pixels
  PIXELM *pointer_1;		//pointer to the first pixel
  PIXELM *pointer_2;		//pointer to the second pixel
  int increment;		//No. of 2*pi to add to one of the pixels to
				//unwrap it with respect to the second
};

typedef struct EDGE EDGE;

//---------------start quicker_sort algorithm --------------------------------
#define unwrap_swap(x,y) {EDGE t; t=x; x=y; y=t;}
#define unwrap_order(x,y) if (x.reliab > y.reliab) unwrap_swap(x,y)
#define unwrap_o2(x,y) unwrap_order(x,y)
#define unwrap_o3(x,y,z) unwrap_o2(x,y); unwrap_o2(x,z); unwrap_o2(y,z)

typedef enum {yes, no} yes_no;

yes_no find_pivot(EDGE *left, EDGE *right, double *pivot_ptr);

EDGE *partition(EDGE *left, EDGE *right, double pivot);

void quicker_sort(EDGE *left, EDGE *right);

void initialisePIXELs(double *wrapped_image, unsigned char *input_mask, unsigned char *extended_mask, PIXELM *pixel, int image_width, int image_height);

double wrap(double pixel_value);

int find_wrap(double pixelL_value, double pixelR_value);

void extend_mask(unsigned char *input_mask, unsigned char *extended_mask,
	int image_width, int image_height,
	params_t *params);


void calculate_reliability(double *wrappedImage, PIXELM *pixel,
	int image_width, int image_height,
	params_t *params);

void horizontalEDGEs(PIXELM *pixel, EDGE *edge,
	int image_width, int image_height,
	params_t *params);

void verticalEDGEs(PIXELM *pixel, EDGE *edge,
	int image_width, int image_height,
	params_t *params);

void gatherPIXELs(EDGE *edge, params_t *params);

void unwrapImage(PIXELM *pixel, int image_width, int image_height);

void maskImage(PIXELM *pixel, unsigned char *input_mask, int image_width, int image_height);

void returnImage(PIXELM *pixel, double *unwrapped_image, int image_width, int image_height);

void unwrap2D(double *wrapped_image, double *UnwrappedImage, unsigned char *input_mask,
	int image_width, int image_height,
	int wrap_around_x, int wrap_around_y);

#endif UNWRAP_H