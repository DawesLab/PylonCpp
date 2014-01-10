//Basic Acquisition Sample
//The sample will open the first camera attached
//and acquire 5 frames.  Part 2 of the sample will collect
//1 frame of data each time the function is called, looping
//through 5 times.

#define NUM_FRAMES  5
#define NO_TIMEOUT  -1

#include "stdio.h"
#include "picam.h"
#include <opencv2/opencv.hpp>

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

    error = Picam_SetParameterFloatingPointValue(
                camera,
                PicamParameter_SensorTemperatureSetPoint,
                -120 );
    PrintError ( error );

    error = Picam_SetParameterIntegerValue(
                camera,
                PicamParameter_TriggerResponse,
                PicamTriggerResponse_ExposeDuringTriggerPulse );
    PrintError ( error );

    error = Picam_SetParameterIntegerValue(
                camera,
                PicamParameter_ShutterTimingMode,
                PicamShutterTimingMode_AlwaysOpen );
    PrintError ( error );



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
        Picam_ConnectDemoCamera(
            PicamModel_Pylon400BRExcelon,
            "12345",
            &id );
        Picam_OpenCamera( &id, &camera );
        printf( "No Camera Detected, Creating Demo Camera\n" );
    }
    Picam_GetEnumerationString( PicamEnumeratedType_Model, id.model, &string );
    printf( "%s", string );
    printf( " (SN:%s) [%s]\n", id.serial_number, id.sensor_name );
    Picam_DestroyString( string );

    ConfigureCamera( camera );

    Picam_GetParameterIntegerValue( camera, PicamParameter_ReadoutStride, &readoutstride );

    //collect one frame
    printf( "\n\n" );
    printf( "Collecting 1 frame\n\n" );
    if( Picam_Acquire( camera, 1, NO_TIMEOUT, &data, &errors ) )
        printf( "Error: Camera only collected %d frames\n", (piint)data.readout_count );
    else
    {
        PrintData( (pibyte*)data.initial_readout, 1, readoutstride );
    }

    Mat image = Mat(400,1340, CV_16U, data.initial_readout).clone();

    printf( "Display data\n" );

    namedWindow( "DisplayImage", CV_WINDOW_AUTOSIZE );
    imshow( "DisplayImage", image );

    waitKey(0);

    //collect two frames
    printf( "\n\n" );
    printf( "Collecting 2 frames\n\n" );
    if( Picam_Acquire( camera, 2, NO_TIMEOUT, &data, &errors ) )
        printf( "Error: Camera only collected %d frames\n", (piint)data.readout_count );
    else
    {
        PrintData( (pibyte*)data.initial_readout, 1, readoutstride );
    }

    Mat image2 = Mat(400,1340, CV_16U, data.initial_readout).clone();

    printf( "Display data\n" );

    namedWindow( "DisplayImage", CV_WINDOW_AUTOSIZE );
    imshow( "DisplayImage", image2 );

    Picam_CloseCamera( camera );
    Picam_UninitializeLibrary();
}
