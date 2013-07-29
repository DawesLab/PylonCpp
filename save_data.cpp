//Data Saving Sample
//The sample will open the first camera attached
//and acquire 5 frames.  The 5 frames will be saved
//in a raw format and can be opened with applications
//such as image-j

#define NUM_FRAMES  5
#define NO_TIMEOUT  -1

#include "stdio.h"
#include "picam.h"
#include <iostream>

// - prints any picam enum
void PrintEnumString( PicamEnumeratedType type, piint value )
{
    const pichar* string;
    Picam_GetEnumerationString( type, value, &string );
    std::cout << string;
    Picam_DestroyString( string );
}
// - prints error code
void PrintError( PicamError error )
{
    if( error == PicamError_None )
        std::cout << "Succeeded" << std::endl;
    else
    {
        std::cout << "Failed (";
        PrintEnumString( PicamEnumeratedType_Error, error );
        std::cout << ")" << std::endl;
    }
}
// - changes some common camera parameters and applies them to hardware
void Configure( PicamHandle camera )
{
    PicamError error;

    // - set low gain
    std::cout << "Set low analog gain: ";
    error =
        Picam_SetParameterIntegerValue(
            camera,
            PicamParameter_AdcAnalogGain,
            PicamAdcAnalogGain_Low );
    PrintError( error );

    // - set exposure time (in millseconds)
    std::cout << "Set 210 ms exposure time: ";
    error = 
        Picam_SetParameterFloatingPointValue(
            camera,
            PicamParameter_ExposureTime,
            210.0 );
    PrintError( error );
    
    // - set ADC rate (in megahertz)
    std::cout << "Set 4 MHz ADC rate: ";
    error = 
        Picam_SetParameterFloatingPointValue(
            camera,
            PicamParameter_AdcSpeed,
            4.0 );
    PrintError( error );


    // - show that the modified parameters need to be applied to hardware
    pibln committed;
    Picam_AreParametersCommitted( camera, &committed );
    if( committed )
        std::cout << "Parameters have not changed" << std::endl;
    else
        std::cout << "Parameters have been modified" << std::endl;

    // - apply the changes to hardware
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
    // - print any invalid parameters
    if( failed_parameters_count > 0 )
    {
        std::cout << "The following parameters are invalid:" << std::endl;
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
    // - free picam-allocated resources
    Picam_DestroyParameters( failed_parameters );
}



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
	FILE *pFile;

    if( Picam_OpenFirstCamera( &camera ) == PicamError_None )
        Picam_GetCameraID( camera, &id );
    else
    {
        Picam_ConnectDemoCamera(
            PicamModel_Pixis100F,
            "0008675309",
            &id );
        Picam_OpenCamera( &id, &camera );
        printf( "No Camera Detected, Creating Demo Camera\n" );
    }
    Picam_GetEnumerationString( PicamEnumeratedType_Model, id.model, &string );
    printf( "%s", string );
    printf( " (SN:%s) [%s]\n", id.serial_number, id.sensor_name );
    Picam_DestroyString( string );

    //set up the camera settings:
    Configure( camera );

    Picam_GetParameterIntegerValue( camera, PicamParameter_ReadoutStride, &readoutstride );
    printf( "Waiting for %d frames to be collected\n\n", NUM_FRAMES );
    if( Picam_Acquire( camera, NUM_FRAMES, NO_TIMEOUT, &data, &errors ) )
        printf( "Error: Camera only collected %d frames\n", (piint)data.readout_count );
    else
    {    
        printf( "Center Three Points:\tFrame # \n");
        PrintData( (pibyte*)data.initial_readout, NUM_FRAMES, readoutstride );
		pFile = fopen( "sample.raw", "wb" );
		if( pFile )
		{
			if( !fwrite( data.initial_readout, 1, (NUM_FRAMES*readoutstride), pFile ) )
				printf( "Data file not saved\n" );
			fclose( pFile );
		}
    }

    Picam_CloseCamera( camera );
    Picam_UninitializeLibrary();
}
