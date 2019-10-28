// 2D phase unwrapping, modified for inclusion in scipy by Gregor Thalhammer

//This program was written by Munther Gdeisat and Miguel Arevallilo Herraez to program the two-dimensional unwrapper
//entitled "Fast two-dimensional phase-unwrapping algorithm based on sorting by
//reliability following a noncontinuous path"
//by Miguel Arevallilo Herraez, David R. Burton, Michael J. Lalor, and Munther A. Gdeisat
//published in the Journal Applied Optics, Vol. 41, No. 35, pp. 7437, 2002.
//This program was written by Munther Gdeisat, Liverpool John Moores University, United Kingdom.
//Date 26th August 2007
//The wrapped phase map is assumed to be of float data type. The resultant unwrapped phase map is also of float type.
//This program takes into consideration the image wrap around problem encountered in MRI imaging.

#include "unwrap2D.h"

yes_no find_pivot(EDGE *left, EDGE *right, float *pivot_ptr) {
	EDGE a, b, c, *p;

	a = *left;
	b = *(left + (right - left) /2 );
	c = *right;
	unwrap_o3(a,b,c);

	if (a.reliab < b.reliab) {
		*pivot_ptr = b.reliab;
		return yes;
	}

	if (b.reliab < c.reliab) {
		*pivot_ptr = c.reliab;
		return yes;
	}

	for (p = left + 1; p <= right; ++p) {
		if (p->reliab != left->reliab) {
			*pivot_ptr = (p->reliab < left->reliab) ? left->reliab : p->reliab;
			return yes;
		}
		return no;
	}

	return no;
}

EDGE *partition(EDGE *left, EDGE *right, float pivot) {
	while (left <= right) {
		while (left->reliab < pivot)
			++left;
		while (right->reliab >= pivot)
			--right;

		if (left < right) {
			unwrap_swap (*left, *right);
			++left;
			--right;
		}
	}
	return left;
}

void quicker_sort(EDGE *left, EDGE *right) {
	EDGE *p;
	float pivot;

	if (find_pivot(left, right, &pivot) == yes) {
		p = partition(left, right, pivot);
		quicker_sort(left, p - 1);
		quicker_sort(p, right);
	}
}
//--------------end quicker_sort algorithm -----------------------------------

//--------------------start initialize pixels ----------------------------------
//initialize pixels. See the explanation of the pixel class above.
//initially every pixel is assumed to belong to a group consisting of only itself
void initialisePIXELs(float *wrapped_image, PIXELM *pixel, int image_width, int image_height) {
	PIXELM *pixel_pointer = pixel;
	float *wrapped_image_pointer = wrapped_image;
	int i, j;

	for (i=0; i < image_height; i++) {
		for (j=0; j < image_width; j++) {
			pixel_pointer->increment = 0;
			pixel_pointer->number_of_pixels_in_group = 1;
			pixel_pointer->value = *wrapped_image_pointer;
			pixel_pointer->reliability = 9999999.f + rand();
			pixel_pointer->head = pixel_pointer;
			pixel_pointer->last = pixel_pointer;
			pixel_pointer->next = NULL;
			pixel_pointer->new_group = 0;
			pixel_pointer->group = -1;
			pixel_pointer++;
			wrapped_image_pointer++;
		}
	}
}
//-------------------end initialize pixels -----------

//gamma function in the paper
float wrap(float pixel_value) {
	float wrapped_pixel_value;
	if (pixel_value > PI) {
		wrapped_pixel_value = pixel_value - TWOPI;
	} else if (pixel_value < -PI) {
		wrapped_pixel_value = pixel_value + TWOPI;
	} else {
		wrapped_pixel_value = pixel_value;
	}
	return wrapped_pixel_value;
}

// pixelL_value is the left pixel, pixelR_value is the right pixel
int find_wrap(float pixelL_value, float pixelR_value) {
	float difference;
	int wrap_value;
	difference = pixelL_value - pixelR_value;

	if (difference > PI) {
		wrap_value = -1;
	} else if (difference < -PI) {
		wrap_value = 1;
	} else {
		wrap_value = 0;
	}
	return wrap_value;
}

void calculate_reliability(float *wrappedImage, PIXELM *pixel,
	int image_width, int image_height,
	params_t *params)
{
	int image_width_plus_one = image_width + 1;
	int image_width_minus_one = image_width - 1;
	PIXELM *pixel_pointer = pixel + image_width_plus_one;
	float *WIP = wrappedImage + image_width_plus_one; //WIP is the wrapped image pointer
	float H, V, D1, D2;
	int i, j;

	for (i = 1; i < image_height -1; ++i) {
		for (j = 1; j < image_width - 1; ++j) {
			H = wrap(*(WIP - 1) - *WIP) - wrap(*WIP - *(WIP + 1));
			V = wrap(*(WIP - image_width) - *WIP) - wrap(*WIP - *(WIP + image_width));
			D1 = wrap(*(WIP - image_width_plus_one) - *WIP) - wrap(*WIP - *(WIP + image_width_plus_one));
			D2 = wrap(*(WIP - image_width_minus_one) - *WIP) - wrap(*WIP - *(WIP + image_width_minus_one));
			pixel_pointer->reliability = H*H + V*V + D1*D1 + D2*D2;

			pixel_pointer++;
			WIP++;
		}
		pixel_pointer += 2;
		WIP += 2;
	}

	if (params->x_connectivity == 1) {
		//calculating the reliability for the left border of the image
		PIXELM *pixel_pointer = pixel + image_width;
		float *WIP = wrappedImage + image_width;

		for (i = 1; i < image_height - 1; ++i) {
			H = wrap(*(WIP + image_width - 1) - *WIP) - wrap(*WIP - *(WIP + 1));
			V = wrap(*(WIP - image_width) - *WIP) - wrap(*WIP - *(WIP + image_width));
			D1 = wrap(*(WIP - 1) - *WIP) - wrap(*WIP - *(WIP + image_width_plus_one));
			D2 = wrap(*(WIP - image_width_minus_one) - *WIP) - wrap(*WIP - *(WIP + (size_t)2* image_width - 1));
			pixel_pointer->reliability = H*H + V*V + D1*D1 + D2*D2;

			pixel_pointer += image_width;
			WIP += image_width;
		}

		//calculating the reliability for the right border of the image
		pixel_pointer = pixel + (size_t)2 * image_width - 1;
		WIP = wrappedImage + (size_t)2 * image_width - 1;

		for (i = 1; i < image_height - 1; ++i) {
			H = wrap(*(WIP - 1) - *WIP) - wrap(*WIP - *(WIP - image_width_minus_one));
			V = wrap(*(WIP - image_width) - *WIP) - wrap(*WIP - *(WIP + image_width));
			D1 = wrap(*(WIP - image_width_plus_one) - *WIP) - wrap(*WIP - *(WIP + 1));
			D2 = wrap(*(WIP - (size_t)2 * image_width - 1) - *WIP) - wrap(*WIP - *(WIP + image_width_minus_one));
			pixel_pointer->reliability = H*H + V*V + D1*D1 + D2*D2;

			pixel_pointer += image_width;
			WIP += image_width;
		}
	}

	if (params->y_connectivity == 1) {
		//calculating the reliability for the top border of the image
		PIXELM *pixel_pointer = pixel + 1;
		float *WIP = wrappedImage + 1;

		for (i = 1; i < image_width - 1; ++i) {
			H = wrap(*(WIP - 1) - *WIP) - wrap(*WIP - *(WIP + 1));
			V = wrap(*(WIP + image_width*((size_t)image_height - 1)) - *WIP) - wrap(*WIP - *(WIP + image_width));
			D1 = wrap(*(WIP + image_width*((size_t)image_height - 1) - 1) - *WIP) - wrap(*WIP - *(WIP + image_width_plus_one));
			D2 = wrap(*(WIP + image_width*((size_t)image_height - 1) + 1) - *WIP) - wrap(*WIP - *(WIP + image_width_minus_one));
			pixel_pointer->reliability = H*H + V*V + D1*D1 + D2*D2;

			pixel_pointer++;
			WIP++;
		}

		//calculating the reliability for the bottom border of the image
		pixel_pointer = pixel + ((size_t)image_height - 1) * image_width + 1;
		WIP = wrappedImage + ((size_t)image_height - 1) * image_width + 1;

		for (i = 1; i < image_width - 1; ++i) {
			H = wrap(*(WIP - 1) - *WIP) - wrap(*WIP - *(WIP + 1));
			V = wrap(*(WIP - image_width) - *WIP) - wrap(*WIP - *(WIP -((size_t)image_height - 1) * (image_width)));
			D1 = wrap(*(WIP - image_width_plus_one) - *WIP) - wrap(*WIP - *(WIP - ((size_t)image_height - 1) * (image_width) + 1));
			D2 = wrap(*(WIP - image_width_minus_one) - *WIP) - wrap(*WIP - *(WIP - ((size_t)image_height - 1) * (image_width) - 1));
			pixel_pointer->reliability = H*H + V*V + D1*D1 + D2*D2;

			pixel_pointer++;
			WIP++;
		}
	}
}

//calculate the reliability of the horizontal edges of the image
//it is calculated by adding the reliability of pixel and the relibility of
//its right-hand neighbour
//edge is calculated between a pixel and its next neighbour
void horizontalEDGEs(PIXELM *pixel, EDGE *edge,
	int image_width, int image_height,
	params_t *params)
{
	int i, j;
	EDGE *edge_pointer = edge;
	PIXELM *pixel_pointer = pixel;
	int no_of_edges = params->no_of_edges;

	for (i = 0; i < image_height; i++) {
		for (j = 0; j < image_width - 1; j++) {
			edge_pointer->pointer_1 = pixel_pointer;
			edge_pointer->pointer_2 = (pixel_pointer+1);
			edge_pointer->reliab = pixel_pointer->reliability + (pixel_pointer + 1)->reliability;
			edge_pointer->increment = find_wrap(pixel_pointer->value, (pixel_pointer + 1)->value);
			edge_pointer++;
			no_of_edges++;
			pixel_pointer++;
		}
		pixel_pointer++;
	}

	//construct edges at the right border of the image
	if (params->x_connectivity == 1) {
		pixel_pointer = pixel + image_width - 1;
	
		for (i = 0; i < image_height; i++) {
			edge_pointer->pointer_1 = pixel_pointer;
			edge_pointer->pointer_2 = (pixel_pointer - image_width + 1);
			edge_pointer->reliab = pixel_pointer->reliability + (pixel_pointer - image_width + 1)->reliability;
			edge_pointer->increment = find_wrap(pixel_pointer->value, (pixel_pointer - image_width + 1)->value);
			edge_pointer++;
			no_of_edges++;
			pixel_pointer+=image_width;
		}
	}
	params->no_of_edges = no_of_edges;
}

//calculate the reliability of the vertical edges of the image
//it is calculated by adding the reliability of pixel and the reliability of
//its lower neighbour in the image.
void verticalEDGEs(PIXELM *pixel, EDGE *edge,
	int image_width, int image_height,
	params_t *params)
{
	int i, j;
	int no_of_edges = params->no_of_edges;
	PIXELM *pixel_pointer = pixel;
	EDGE *edge_pointer = edge + no_of_edges;

	for (i=0; i < image_height - 1; i++) {
		for (j=0; j < image_width; j++) {
			edge_pointer->pointer_1 = pixel_pointer;
			edge_pointer->pointer_2 = (pixel_pointer + image_width);
			edge_pointer->reliab = pixel_pointer->reliability + (pixel_pointer + image_width)->reliability;
			edge_pointer->increment = find_wrap(pixel_pointer->value, (pixel_pointer + image_width)->value);
			edge_pointer++;
			no_of_edges++;

			pixel_pointer++;
		} //j loop
	} // i loop

	//construct edges that connect at the bottom border of the image
	if (params->y_connectivity == 1) {
		pixel_pointer = pixel + image_width *((size_t)image_height - 1);
		for (i = 0; i < image_width; i++) {
			edge_pointer->pointer_1 = pixel_pointer;
			edge_pointer->pointer_2 = (pixel_pointer - image_width *((size_t)image_height - 1));
			edge_pointer->reliab = pixel_pointer->reliability + (pixel_pointer - image_width *((size_t)image_height - 1))->reliability;
			edge_pointer->increment = find_wrap(pixel_pointer->value, (pixel_pointer - image_width *((size_t)image_height - 1))->value);
			edge_pointer++;
			no_of_edges++;

			pixel_pointer++;
		}
	}
	params->no_of_edges = no_of_edges;
}

//gather the pixels of the image into groups
void gatherPIXELs(EDGE *edge, params_t *params)
{
	int k;
	PIXELM *PIXEL1;
	PIXELM *PIXEL2;
	PIXELM *group1;
	PIXELM *group2;
	EDGE *pointer_edge = edge;
	int incremento;

	for (k = 0; k < params->no_of_edges; k++) {
		PIXEL1 = pointer_edge->pointer_1;
		PIXEL2 = pointer_edge->pointer_2;

		//PIXELM 1 and PIXELM 2 belong to different groups
		//initially each pixel is a group by itself and one pixel can construct a group
		//no else or else if to this if
		if (PIXEL2->head != PIXEL1->head) {
			//PIXELM 2 is alone in its group
			//merge this pixel with PIXELM 1 group and find the number of 2 pi to add
			//to or subtract to unwrap it
			if ((PIXEL2->next == NULL) && (PIXEL2->head == PIXEL2)) {
				PIXEL1->head->last->next = PIXEL2;
				PIXEL1->head->last = PIXEL2;
				(PIXEL1->head->number_of_pixels_in_group)++;
				PIXEL2->head=PIXEL1->head;
				PIXEL2->increment = PIXEL1->increment-pointer_edge->increment;
			}

			//PIXELM 1 is alone in its group
			//merge this pixel with PIXELM 2 group and find the number of 2 pi to add
			//to or subtract to unwrap it
			else if ((PIXEL1->next == NULL) && (PIXEL1->head == PIXEL1)) {
			PIXEL2->head->last->next = PIXEL1;
			PIXEL2->head->last = PIXEL1;
			(PIXEL2->head->number_of_pixels_in_group)++;
			PIXEL1->head = PIXEL2->head;
			PIXEL1->increment = PIXEL2->increment+pointer_edge->increment;
			}

			//PIXELM 1 and PIXELM 2 both have groups
			else {
				group1 = PIXEL1->head;
				group2 = PIXEL2->head;
				//if the no. of pixels in PIXELM 1 group is larger than the
				//no. of pixels in PIXELM 2 group. Merge PIXELM 2 group to
				//PIXELM 1 group and find the number of wraps between PIXELM 2
				//group and PIXELM 1 group to unwrap PIXELM 2 group with respect
				//to PIXELM 1 group. the no. of wraps will be added to PIXELM 2
				//group in the future
				if (group1->number_of_pixels_in_group > group2->number_of_pixels_in_group) {
					//merge PIXELM 2 with PIXELM 1 group
					group1->last->next = group2;
					group1->last = group2->last;
					group1->number_of_pixels_in_group = group1->number_of_pixels_in_group + group2->number_of_pixels_in_group;
					incremento = PIXEL1->increment-pointer_edge->increment - PIXEL2->increment;
					//merge the other pixels in PIXELM 2 group to PIXELM 1 group
					while (group2 != NULL) {
						group2->head = group1;
						group2->increment += incremento;
						group2 = group2->next;
					}
				}

				//if the no. of pixels in PIXELM 2 group is larger than the
				//no. of pixels in PIXELM 1 group. Merge PIXELM 1 group to
				//PIXELM 2 group and find the number of wraps between PIXELM 2
				//group and PIXELM 1 group to unwrap PIXELM 1 group with respect
				//to PIXELM 2 group. the no. of wraps will be added to PIXELM 1
				//group in the future
				else {
					//merge PIXELM 1 with PIXELM 2 group
					group2->last->next = group1;
					group2->last = group1->last;
					group2->number_of_pixels_in_group = group2->number_of_pixels_in_group + group1->number_of_pixels_in_group;
					incremento = PIXEL2->increment + pointer_edge->increment - PIXEL1->increment;
					//merge the other pixels in PIXELM 2 group to PIXELM 1 group
					while (group1 != NULL) {
						group1->head = group2;
						group1->increment += incremento;
						group1 = group1->next;
					} // while

				} // else
			} //else
		} //if
		pointer_edge++;
	}
}

//unwrap the image
void unwrapImage(PIXELM *pixel, int image_width, int image_height) {
	int i;
	int image_size = image_width * image_height;
	PIXELM *pixel_pointer=pixel;

	for (i = 0; i < image_size; i++) {
		pixel_pointer->value += TWOPI * (float)(pixel_pointer->increment);
		pixel_pointer++;
	}
}

//the input to this unwrapper is an array that contains the wrapped
//phase map. copy the image on the buffer passed to this unwrapper to
//over-write the unwrapped phase map on the buffer of the wrapped
//phase map.
void returnImage(PIXELM *pixel, float *unwrapped_image, int image_width, int image_height) {
	int i;
	int image_size = image_width * image_height;
	float *unwrapped_image_pointer = unwrapped_image;
	PIXELM *pixel_pointer = pixel;

	for (i=0; i < image_size; i++) {
		*unwrapped_image_pointer = pixel_pointer->value;
		pixel_pointer++;
		unwrapped_image_pointer++;
	}
}

//the main function of the unwrapper
void unwrap2D(float *wrapped_image, float *UnwrappedImage,
	 int image_width, int image_height,
	 int wrap_around_x, int wrap_around_y, EDGE *edge, PIXELM *pixel)
{
	params_t params = {TWOPI, wrap_around_x, wrap_around_y, 0};
	int image_size = image_height * image_width;

	initialisePIXELs(wrapped_image, pixel, image_width, image_height);
	calculate_reliability(wrapped_image, pixel, image_width, image_height, &params);
	horizontalEDGEs(pixel, edge, image_width, image_height, &params);
	verticalEDGEs(pixel, edge, image_width, image_height, &params);

	//sort the EDGEs depending on their reliability. The PIXELs with higher
	//reliability (small value) first
	quicker_sort(edge, edge + params.no_of_edges - 1);

	//gather PIXELs into groups
	gatherPIXELs(edge, &params);

	unwrapImage(pixel, image_width, image_height);

	//copy the image from PIXELM structure to the unwrapped phase array
	//passed to this function
	//TODO: replace by (cython?) function to directly write into numpy array ?
	returnImage(pixel, UnwrappedImage, image_width, image_height);
}