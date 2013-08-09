//Basic Acquisition Sample
//The sample will open the first camera attached
//and acquire 5 frames.  Part 2 of the sample will collect
//1 frame of data each time the function is called, looping
//through 5 times.

// TODO: add storage of FFT (half array) of middle 5 rows from each shot
// TODO: add command line flags to specify saving full FFT or an ROI

#define NUM_FRAMES  5
#define NO_TIMEOUT  -1

#include "stdio.h"
#include "picam.h"
#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace cv;

//TODO: put these Print functions into another file

void PrintData( pibyte* buf, piint numframes, piint framelength )
{
    pi16u  *midpt = NULL;
    pibyte *frameptr = NULL;

    for( piint loop = 0; loop < numframes; loop++ )
    {
        frameptr = buf + ( framelength * loop );
        midpt = (pi16u*)frameptr + ( ( ( framelength/sizeof(pi16u) )/ 2 ) );
        printf( "%5d,%5d,%5d\t%d\n", *(midpt-1), *(midpt), *(midpt+1), loop+1 );
    }
}

void PrintEnumString( PicamEnumeratedType type, piint value )
{
    const pichar* string;
    Picam_GetEnumerationString( type, value, &string );
    std::cout << string;
    Picam_DestroyString( string );
}


void PrintError (PicamError error)
{
    if ( error == PicamError_None )
	std::cout << "Succeeded" << std::endl;
    else
    {
        std::cout << "Failed (";
        PrintEnumString( PicamEnumeratedType_Error, error );
        std::cout << ")" << std::endl;
    }
}

void ConfigureCamera (PicamHandle camera)
{
    std::cout << "Set ADC rate to 4 MHz: ";
    
    PicamError error;
    error = Picam_SetParameterFloatingPointValue(
                camera,
                PicamParameter_AdcSpeed,
                4.0 );
    PrintError( error );    
    
    pibln committed;
    Picam_AreParametersCommitted( camera, &committed );
    if( committed )
        std::cout << "Parameters have not changed" << std::endl;
    else
        std::cout << "Parameters have been modified" << std::endl;

    // apply changes to hardware
    std::cout << "Commit to hardware: ";
    const PicamParameter* failed_parameters;
    piint failed_parameters_count;
    error = 
        Picam_CommitParameters(
            camera,
            &failed_parameters,
            &failed_parameters_count );
    PrintError( error );

    std::cout << "Testing for invalid params\n";
    if( failed_parameters_count > 0 )
    {
        std::cout << "The following params are invalid:" << std::endl;
        for( piint i = 0; i < failed_parameters_count; ++i )
        {
            std::cout << "    ";
            PrintEnumString(
                PicamEnumeratedType_Parameter,
                failed_parameters[i] );
            std::cout << std::endl;
        }
    }
    std::cout << "Cleaning up resources\n";

    Picam_DestroyParameters( failed_parameters );
}

int main()
{
    Picam_InitializeLibrary();

    // - open the first camera if any or create a demo camera
    PicamHandle camera;
    PicamCameraID id;
    const pichar* string;
    PicamAvailableData data;
    PicamAcquisitionErrorsMask errors;

    piint readoutstride = 0;

    if( Picam_OpenFirstCamera( &camera ) == PicamError_None )
        Picam_GetCameraID( camera, &id );
    else
    {
    	printf( "Cannot load camera\n");
        return(1);
    }
    Picam_GetEnumerationString( PicamEnumeratedType_Model, id.model, &string );
    printf( "%s", string );
    printf( " (SN:%s) [%s]\n", id.serial_number, id.sensor_name );
    Picam_DestroyString( string );

    ConfigureCamera( camera );

    Picam_GetParameterIntegerValue( camera, PicamParameter_ReadoutStride, &readoutstride );

    //collect one frame
    printf( "\n\n" );
    
    for (int i = 0; i < 10; i++)
    {


	    printf( "Collecting 1 frame\n\n" );
	    if( Picam_Acquire( camera, 1, NO_TIMEOUT, &data, &errors ) )
	        printf( "Error: Camera only collected %d frames\n", (piint)data.readout_count );
	    else
	    {
	        PrintData( (pibyte*)data.initial_readout, 1, readoutstride );
	    }
	    
	    Mat image = Mat(400,1340, CV_16U, data.initial_readout).clone();
	    
	    

	    printf( "Display data\n" );

	    //namedWindow( "DisplayImage", CV_WINDOW_AUTOSIZE );
	    //imshow( "DisplayImage", image );
	    
	    //waitKey(0);

	    //The following was copied (with substitution of "image" for "I") from http://docs.opencv.org/doc/tutorials/core/discrete_fourier_transform/discrete_fourier_transform.html

	    Mat padded;
	    int m = getOptimalDFTSize( image.rows );
	    int n = getOptimalDFTSize( image.cols );
	    copyMakeBorder(image, padded, 0, m - image.rows, 0, n - image.cols, BORDER_CONSTANT, Scalar::all(0));

	    Mat planes[] = {Mat_<float>(padded), Mat::zeros(padded.size(), CV_32F)};
	    Mat complexI;
	    merge(planes, 2, complexI);         // Add to the expanded another plane with zeros

	    dft(complexI, complexI, DFT_ROWS);            // this way the result may fit in the source matrix

	    // compute the magnitude and switch to logarithmic scale
	    // => log(1 + sqrt(Re(DFT(I))^2 + Im(DFT(I))^2))
	    split(complexI, planes);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
	    magnitude(planes[0], planes[1], planes[0]);// planes[0] = magnitude
	    Mat magI = planes[0];

	    magI += Scalar::all(1);                    // switch to logarithmic scale
	    log(magI, magI);

	    // crop the spectrum, if it has an odd number of rows or columns
	    magI = magI(Rect(0, 0, magI.cols & -2, magI.rows & -2));

	    // rearrange the quadrants of Fourier image  so that the origin is at the image center
	    //int cx = magI.cols/2;
	    //int cy = magI.rows/2;

	    //Mat q0(magI, Rect(0, 0, cx, cy));   // Top-Left - Create a ROI per quadrant
	    //Mat q1(magI, Rect(cx, 0, cx, cy));  // Top-Right
	    //Mat q2(magI, Rect(0, cy, cx, cy));  // Bottom-Left
	    //Mat q3(magI, Rect(cx, cy, cx, cy)); // Bottom-Right

	    Mat tmp;                           // swap quadrants (Top-Left with Bottom-Right)
	    //q0.copyTo(tmp);
	    //q3.copyTo(q0);
	    //tmp.copyTo(q3);

	    //q1.copyTo(tmp);                    // swap quadrant (Top-Right with Bottom-Left)
	    //q2.copyTo(q1);
	    //tmp.copyTo(q2);

	    Mat leftHalf(magI, Rect(0, 0, magI.cols/2, magI.rows));
	    Mat rightHalf(magI, Rect(magI.cols/2, 0, magI.cols/2, magI.rows));

	    leftHalf.copyTo(tmp);
	    rightHalf.copyTo(leftHalf);
	    tmp.copyTo(rightHalf);

	    normalize(magI, magI, 0, 1, CV_MINMAX); // Transform the matrix with float values into a
	                                            // viewable image form (float between values 0 and 1).

	    imshow("Input Image"       , image   );    // Show the result
	    imshow("spectrum magnitude", magI);
	    waitKey();
	}
	Picam_CloseCamera( camera );
    Picam_UninitializeLibrary();
    //TODO add file output of complex numbers from one element of FFT result. (command line flag)
}
