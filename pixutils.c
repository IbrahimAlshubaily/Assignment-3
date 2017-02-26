#include "pixutils.h"
//Ibrahim Alshubaily
//private methods -> make static
static pixMap* pixMap_init(unsigned char arrayType);
static pixMap* pixMap_copy(pixMap *p);

static pixMap* pixMap_init(unsigned char arrayType){
	//initialize everything to zero except arrayType
	pixMap *p = malloc(sizeof(pixMap));
			p->image = 0;
			p->imageWidth = 0;
			p->imageHeight = 0;
			p->arrayType = arrayType;
			p->pixArray_arrays = 0;
			p->pixArray_blocks = 0;
			p->pixArray_overlay = 0;
	return p;
}	

void pixMap_destroy (pixMap **p){
 //free all mallocs and put a zero pointer in *p
//valgrind --leak-check=full --show-leak-kinds=all  -v ./rotate -i 2LoBT.png -o test.png -r 60 -t 1

	free((*p)->image);
	if((*p)->pixArray_arrays)free((*p)->pixArray_arrays);
	if((*p)->pixArray_overlay)free((*p)->pixArray_overlay);
	if((*p)->pixArray_blocks){
		for (int i = 0l; i < (*p)->imageHeight; i++) {
			if((*p)->pixArray_blocks[i])free((*p)->pixArray_blocks[i]);
		}
		free((*p)->pixArray_blocks);
	}
	if(*p)free((*p));
}
	
pixMap *pixMap_read(char *filename,unsigned char arrayType){
 //library call reads in the image into p->image and sets the width and height
	pixMap *p=pixMap_init(arrayType);
 int error;
 if((error=lodepng_decode32_file(&(p->image), &(p->imageWidth), &(p->imageHeight),filename))){
  fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
  return 0;
	}
 //allocate the 2-D rgba arrays
	if (arrayType ==0){
		//check if it is too big and then return an error (zero pointer)
		if (p->imageWidth > MAXWIDTH) {
			return 0;
		}
  //can only allocate for the number of rows - each row will be an array of MAXWIDTH
		int rows = p->imageHeight;
		p->pixArray_arrays = malloc(rows*(4*MAXWIDTH));
		int cols = p->imageWidth;
		int runner = 0;
		//copy each row of the image into each row
		//I tried to memcpy the hole row but kept getting seg fault
		for (int i = 0; i < rows; i++) {
			for (int j = 0; j < cols; j++){
				p->pixArray_arrays[i][j].r = p->image[runner];
				runner++;
				p->pixArray_arrays[i][j].g = p->image[runner];
				runner++;
				p->pixArray_arrays[i][j].b = p->image[runner];
				runner++;
				p->pixArray_arrays[i][j].a = p->image[runner];
				runner++;
			}
		}
	}	
	else if (arrayType ==1){
		//allocate a block of memory (dynamic array of p->imageHeight) to store the pointers
		int rows = p->imageHeight;
		p->pixArray_blocks = malloc(rows*8);
		//use a loop allocate a block of memory for each row
		int cols = p->imageWidth;
		int runner = 0;
		for (int i = 0; i < rows; i++){
			p->pixArray_blocks[i] = malloc(p->imageWidth*4);
			//copy each row of the image into the newly allocated block
			for (int j = 0; j < cols; j++){
				p->pixArray_blocks[i][j].r = p->image[runner];
				runner++;
				p->pixArray_blocks[i][j].g = p->image[runner];
				runner++;
				p->pixArray_blocks[i][j].b = p->image[runner];
				runner++;
				p->pixArray_blocks[i][j].a = p->image[runner];
				runner++;
			}
		}
 }
	else if (arrayType ==2){
// allocate a block of memory (dynamic array of p->imageHeight) to store the pointers
		int rows = p->imageHeight;
		p->pixArray_overlay = malloc(rows*8);
  //set the first pointer to the start of p->image
		p->pixArray_overlay[0] = (rgba*)p->image;
  //each subsequent pointer is the previous pointer + p->imageWidth
		for (int i = 1; i < rows; i++){
			p->pixArray_overlay[i] = p->pixArray_overlay[i-1]+ p->imageWidth;
		}
	}
	else{
		return 0;
	}				
	return p;
}
int pixMap_write(pixMap *p,char *filename){
	int error=0;
	//for arrayType 0 and arrayType 1 you have to write out a copy of the rows to the image using memcpy
	 if (p->arrayType ==0){
   //have to copy each row of the array into the corresponding row of the image	
		 int rows = p->imageHeight;
		 int cols = p->imageWidth;
		 int runner = 0;
		 //(Opposite of read)
		 for (int i = 0; i < rows; i++) {
		 	for (int j = 0; j < cols; j++){
		 		p->image[runner] = p->pixArray_arrays[i][j].r;
		 		runner++;
		 		p->image[runner] = p->pixArray_arrays[i][j].g;
		 		runner++;
		 		p->image[runner] = p->pixArray_arrays[i][j].b;
		 		runner++;
		 		p->image[runner] = p->pixArray_arrays[i][j].a;
		 		runner++;
		 	}
		 }

	 }	
		else if (p->arrayType ==1){
   //have to copy each row of the array into the corresponding row of the image
			int rows = p->imageHeight;
			int cols = p->imageWidth;
			int runner = 0;
			for (int i = 0; i < rows; i++){
				for (int j = 0; j < cols; j++){
					p->image[runner] = p->pixArray_blocks[i][j].r;
					runner++;
					p->image[runner] = p->pixArray_blocks[i][j].g;
					runner++;
					p->image[runner] = p->pixArray_blocks[i][j].b;
					runner++;
					p->image[runner] = p->pixArray_blocks[i][j].a;
					runner++;
				}
			}
	 }
	//library call to write the image out 
 if(lodepng_encode32_file(filename, p->image, p->imageWidth, p->imageHeight)){
  fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
  return 1;
	}
	return 0;
}	 

int pixMap_rotate(pixMap *p,float theta){
	pixMap *oldPixMap=pixMap_copy(p);
	if(!oldPixMap)return 1;
 const float ox=p->imageWidth/2.0f;
 const float oy=p->imageHeight/2.0f;
 const float s=sin(degreesToRadians(-theta));
	const float c=cos(degreesToRadians(-theta));
	for(int y=0;y<p->imageHeight;y++){
		for(int x=0;x<p->imageWidth;x++){
   float rotx = c*(x-ox) - s * (oy-y) + ox;
   float roty = -(s*(x-ox) + c * (oy-y) - oy);
		 int rotj=rotx+.5;
		 int roti=roty+.5;
		 if(roti >=0 && roti < oldPixMap->imageHeight && rotj >=0 && rotj < oldPixMap->imageWidth){
			 if(p->arrayType==0) memcpy(p->pixArray_arrays[y]+x,oldPixMap->pixArray_arrays[roti]+rotj,sizeof(rgba));
			 else if(p->arrayType==1) memcpy(p->pixArray_blocks[y]+x,oldPixMap->pixArray_blocks[roti]+rotj,sizeof(rgba));
			 else if(p->arrayType==2) memcpy(p->pixArray_overlay[y]+x,oldPixMap->pixArray_overlay[roti]+rotj,sizeof(rgba));
			}
			else{
				if(p->arrayType==0) memset(p->pixArray_arrays[y]+x,0,sizeof(rgba));
			 else if(p->arrayType==1) memset(p->pixArray_blocks[y]+x,0,sizeof(rgba));
			 else if(p->arrayType==2) memset(p->pixArray_overlay[y]+x,0,sizeof(rgba));
			}
		}
	}
 pixMap_destroy(&oldPixMap);
 return 0;
}

pixMap *pixMap_copy(pixMap *p){
	pixMap *new=pixMap_init(p->arrayType);
	//allocate memory for new image of the same size a p->image and copy the image
	new->image = malloc(4*(p->imageHeight*p->imageWidth));
	memcpy(new->image, p->image,4*(p->imageHeight*p->imageWidth));
	new->imageHeight = p->imageHeight;
	new->imageWidth = p->imageWidth;
	int rows = new->imageHeight;
	int cols = new->imageWidth;
	//allocate memory and copy the arrays. 
	if (new->arrayType ==0){
		new->pixArray_arrays = malloc(rows* (4*MAXWIDTH));
		memcpy(new->pixArray_arrays, p->pixArray_arrays,rows* (4*MAXWIDTH));
	}	
 else if (new->arrayType ==1){
	 new->pixArray_blocks = malloc(8*(cols*rows));
	 for (int i = 0; i < rows; i++) {
		 new->pixArray_blocks[i] = malloc(4*cols);
		 memcpy(new->pixArray_blocks[i], p->pixArray_blocks[i], 4*cols);
	 }
	}
	else if (new->arrayType ==2){
		new ->pixArray_overlay = malloc(rows*8);
		new ->pixArray_overlay[0] = (rgba*) new->image;
		for (int i = 1; i < rows; i++) {
			//each subsequent pointer is the previous pointer + p->imageWidth
			 new->pixArray_overlay[i] = new->pixArray_overlay[i-1]+cols;
		}
	}
	return new;
}
