#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <CL/cl.h>

#define BUFFERSIZE		1024
#define MAX_PLATFORMS	8	
#define MAX_DEVICES		8
#define GPU_DATA_SIZE	1024
 
int checkerror (cl_int err, char *msg);

int main(int argc, char** argv)
{
	
	char 				buffer[BUFFERSIZE];
    cl_int 				err;                           
    cl_device_id 		device_id;   
    cl_platform_id  	platform_id;         
    cl_device_id 		devices[MAX_DEVICES];          
    cl_uint 			num;
    cl_context 			context;                
    cl_platform_id 		platforms[MAX_PLATFORMS];
    cl_command_queue 	commands;     
     

    cl_program			program;
    cl_kernel			kernel;
    char				*kernelsrc;
    char				filename[] = "kernel.cl";
    

 

    err =  clGetPlatformIDs(MAX_PLATFORMS, platforms, &num);
    printf ("Num Platforms: %d\n", num);
    checkerror(err,"Platform error");

    
     
     
 // Search for platforms devices and set a default gpu device    
    for (int i = 0; i < num; i++){
		
		
		cl_uint n;
		cl_device_type type;
		clGetPlatformInfo(platforms[i],CL_PLATFORM_NAME,1024,buffer,NULL);
		printf ("Platform name: %s\n", buffer );	
		//clGetPlatformInfo(platforms[i],CL_PLATFORM_EXTENSIONS,1024,buffer,NULL);
		//printf ("  Platform Extensions: %s\n", buffer );
		
		err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, MAX_DEVICES, devices, &n);
		checkerror(err,"Get Devices error");
		
		for (int d = 0; d < n; d++){
			clGetDeviceInfo(devices[i],CL_DEVICE_NAME,1024,buffer,NULL);
			printf ("    Device Name: %s\n", buffer );
			clGetDeviceInfo(devices[i],CL_DEVICE_TYPE,1024,&type,NULL);
			printf ("    Device Type: %d\n", type );
			
			// Set platform and device with the first GPU device found
			if (type & CL_DEVICE_TYPE_GPU) {
				device_id = devices[i];
				platform_id = platforms[i];
			}			
		}		
	}
  
	clGetDeviceInfo(device_id,CL_DEVICE_NAME,1024,buffer,NULL);
	printf ("\nUsing Device Name: %s\n", buffer );
	
	
	
	FILE *file = fopen(filename,"rb");
    if (file){
		struct stat st; 
		if (fstat(fileno(file), &st) == 0) {
			printf ("Loading %s\n", filename);
			kernelsrc = malloc (st.st_size + 1);
			if (kernelsrc) fread (kernelsrc, 1, st.st_size, file);
			kernelsrc[st.st_size + 1] = 0;
		}
		fclose (file);
	} else {
		printf ("%s open failure.\n", filename);
		exit(-1);
	}
	
	cl_context_properties properties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties) platform_id ,0,0};
    context = clCreateContext(properties, 1, &device_id, NULL, NULL, &err);
    checkerror(err,"Create Context Error");
	

    commands = clCreateCommandQueueWithProperties(context, device_id, 0, &err);
    checkerror(err,"Get CommandQueue error");

	program = clCreateProgramWithSource(context, 1, (const char **) &kernelsrc, NULL, &err);
    checkerror(err,"Create Program error");
    
    // Build the program executable
    //
    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        size_t len;
        char buffer[2048];
 
        printf("Error: Failed to build program executable!\n");
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);
        exit(1);
    }
    
    kernel = clCreateKernel(program, "Teste", &err);
    checkerror(err,"Create Kernel error");
	
	
	
	cl_float		    *x = (cl_float *) malloc(sizeof(cl_float) * GPU_DATA_SIZE);
	cl_float		    *y = (cl_float *) malloc(sizeof(cl_float) * GPU_DATA_SIZE);
	cl_float		    *z = (cl_float *) malloc(sizeof(cl_float) * GPU_DATA_SIZE);
	
	cl_mem 				A, B, C;    
	
	for (int i = 0; i < GPU_DATA_SIZE; i++){
		x[i] = i;
		y[i] = i;
		z[i] = 0;		

	}
	
	A = clCreateBuffer(context,  CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,  sizeof(cl_float) * GPU_DATA_SIZE, x, &err);
	checkerror(err,"CreateBuffer error"); 
	B = clCreateBuffer(context,  CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,  sizeof(cl_float) * GPU_DATA_SIZE, y, &err);
	checkerror(err,"CreateBuffer error"); 
	C = clCreateBuffer(context,  CL_MEM_WRITE_ONLY | CL_MEM_COPY_HOST_PTR,  sizeof(cl_float) * GPU_DATA_SIZE, z, &err);
	checkerror(err,"CreateBuffer error"); 
	
	
	err = clEnqueueWriteBuffer(commands, A, CL_TRUE, 0, sizeof(cl_float) * GPU_DATA_SIZE, x, 0, NULL, NULL);
	checkerror(err,"EnqueueWriteBuffer error"); 
	err = clEnqueueWriteBuffer(commands, B, CL_TRUE, 0, sizeof(cl_float) * GPU_DATA_SIZE, y, 0, NULL, NULL);
	checkerror(err,"EnqueueWriteBuffer error"); 
	err = clEnqueueWriteBuffer(commands, C, CL_TRUE, 0, sizeof(cl_float) * GPU_DATA_SIZE, z, 0, NULL, NULL);
	checkerror(err,"EnqueueWriteBuffer error"); 
	

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &A); checkerror(err,"Arguments error"); 
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &B); checkerror(err,"Arguments error"); 
    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), &C); checkerror(err,"Arguments error"); 

	
	const int TS = 32;
	const size_t local[1] = {10};
	const size_t global[1] = {1024};
	
	err = clEnqueueNDRangeKernel(commands, kernel, 1, NULL, global, local, 0, NULL, NULL);
	checkerror(err,"Range error");
	
	clFinish(commands);
	
	err = clEnqueueReadBuffer( commands, C, CL_TRUE, 0,  sizeof(cl_float) * GPU_DATA_SIZE, z, 0, NULL, NULL );
	
	for (int i = 0; i < GPU_DATA_SIZE; i++ ){
		printf ("%f ", z[i]);
	}
	
	
	
	

    clReleaseProgram(program);
    //clReleaseKernel(kernel);
    clReleaseCommandQueue(commands);
    clReleaseContext(context);
    
	
	
	
  
    return 0;
}


int checkerror (cl_int err, char *msg) {
	
	if (err != CL_SUCCESS){
		printf (msg);
		printf (" %d ", err);
		printf ("\n");
		
		return -1;
	}
	
	return 0;
}
