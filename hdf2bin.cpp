#include "include/hdf5.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <time.h>
#include <sstream>
#include <math.h>
#include <string.h>

std::string INFILE;
std::string OUTFILE;
std::string DATASET;
int CHUNK;
static hsize_t 	dim[2]; /* HDF dimension (y,x) */


double ** getDatasetData(hid_t file_id, std::string dname)
{
	char *			_dataset; /* dataset name. ex) /chlor_a */
	hid_t			dataset_id; /* HDF dataset id*/
	hid_t			space; /* HDF space */
	
	_dataset = (char * ) malloc ( sizeof (char * ) * DATASET.size() ) ;
	sprintf(_dataset, "%s", dname.c_str());
	dataset_id = H5Dopen(file_id, _dataset, H5P_DEFAULT);
	space = H5Dget_space(dataset_id);
	H5Sget_simple_extent_dims(space, dim, NULL);

	double ** value = (double **) malloc (sizeof(double *) * dim[0]);
	value[0] = (double *) malloc( sizeof(double) * dim[0] * dim[1]);

	for (int i=0; i<dim[0]; i++)
	{
		value[i] = value[0] + i*dim[1];
	}

	H5Dread(dataset_id, H5T_NATIVE_DOUBLE, H5S_ALL, space, H5P_DEFAULT, &value[0][0]);

	free(_dataset);
	H5Dclose(dataset_id);
	H5Sclose(space);

	return value;
}


void hdf2csv( )
{
	std::string strHeader = "x,y,data\n";
	std::string temp = "";
	
	double ** 			data; /* HDF value*/
	double ** 			latitude; 
	double **	 		longitude; 
	FILE *				ofile; /* outfile(csv) */
	hid_t 				file_id; /* HDF file */
	herr_t  			status;
	//static hsize_t *	dim; /* HDF dimension (y,x) */
	double ***  		value;
	int 				dataset_cnt = 1;

	/********** Read hdf5 Start **********/
	file_id = H5Fopen(INFILE.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
	for (int i=0; i< DATASET.size(); i++)
		if (DATASET[i] == ',')
			dataset_cnt++;

	value = ( double *** ) malloc ( sizeof(double ***) * dataset_cnt );
	std::string dname = "";
	int comma_cnt =0;
	for (int i=0; i< DATASET.size(); i++)
	{
		if (DATASET[i] == ',' )
		{	
			std::cout << dname << std::endl;
			value[comma_cnt] = getDatasetData(file_id, dname);
			dname = "";	
			comma_cnt++;
		}
		else 
		{
			dname += DATASET[i];
		}
	}
	
	std::cout << dname << std::endl;
	value[comma_cnt] = getDatasetData(file_id, dname);
	
	std::cout << dim[0] << ", " << dim[1] << std::endl;


	//********* Write file start *********
	ofile = fopen(OUTFILE.c_str(), "wb"); 

	int height = dim[0];
	int width = dim[1];
	int hIter = int(height/CHUNK) + 1;
    int wIter = int(width/CHUNK) + 1;

	for (int j=0; j<height; j++)		
		for(int i=0; i<width; i++)		
			for (int cnt=0; cnt<dataset_cnt; cnt++)
				fwrite( &value[cnt][j][i], sizeof(double), 1, ofile);			
		
	


	/*
	for (int j=0; j<hIter; j++)		
	{
		for(int i=0; i<wIter; i++)		
		{
			for (int hChunk=0; hChunk<CHUNK; hChunk++)	
			{
				for (int wChunk=0; wChunk<CHUNK; wChunk++)		
				{
					if (i*CHUNK + wChunk < width && j*CHUNK + hChunk < height)
					{
						for (int cnt=0; cnt<dataset_cnt; cnt++)
							fwrite( &value[cnt][j*CHUNK + hChunk][i*CHUNK + wChunk], sizeof(double), 1, ofile);	
					}
				}
			}
			
		}
	}*/
	/********* Write file end ********/


	/********** close File start **********/
	status = H5Fclose(file_id);

	free(value[0][0]);
	free(value[0]);
	free(value);
	
	fclose(ofile);
	/********** close File end **********/
}


int main(int argc, char **argv)
{
	clock_t 	before;
	double 		result;
	
	before = clock();

	if ( argc == 1 || ((std::string)argv[1]).find("help") != std::string::npos) 
    {
        printf("usage: ./hdf2csv {-i inputFileName} {-o outputFileName}  {-d datasetName} {-c chunkSize}\n\n");
        printf("inputFileName:\tinput(tiff) file name\n");
        printf("outputFileName:\toutput(bin) file name \n");
        printf("datasetName:\tHDF dataset Name. Use commas(,) as delimiters. ex) /HDF5/SWATHS/Aerosol NearUV Swath/Data Fields/FinalAerosolLayerHeight \n");
		printf("chunkSize(int):\tSciDB Chunk size. \n\n");
        printf("optional arguments:\n  --help, -help \t show this help message and exit \n\n");
        return 0;
    }

	if (argc < 9) 
	{
		printf("[ERROR] An insufficient number of arguments\n");
		return 0;
	}
	else if (argc > 9)
	{
		printf("[ERROR] Too many arguments \n");
		return 0;
	}

	for (int i=0; i<argc; i++)
	{
		if (std::string(argv[i]) == "-o")
			OUTFILE = std::string(argv[i+1]);
		else if (std::string(argv[i]) == "-i")
			INFILE = std::string(argv[i+1]);
		else if (std::string(argv[i]) == "-d")
			DATASET = std::string(argv[i+1]);
		else if (std::string(argv[i]) == "-c")
			CHUNK = atoi(argv[i+1]);
	}

	hdf2csv();

	result = (double)(clock() - before) / CLOCKS_PER_SEC;
	printf("time: %5.2f sec \n", result); 

	return 0;
}
