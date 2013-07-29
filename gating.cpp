//Gating Sample
//Part 1 of the sample will open the first camera attached
//and if it is capable of gating, acquire 1 frame in repetitive gating mode.
//Part 2 of the sample will set gating mode to sequential and
//collect 4 frames.  The delay and width will grow by 25nS each frame.

#define NUM_FRAMES         1
#define SEQ_FRAMES         4
#define NO_TIMEOUT        -1
#define TRIG_FREQ        1.0//Hz
#define GATE_DELAY      25.0//nS -- Starting Gate Delay when in Sequential Gating mode
#define GATE_WIDTH      25.0//nS -- Starting Gate Width when in Sequential Gating mode
#define AUX_DELAY       50.0//nS
#define AUX_WIDTH       25.0//nS
#define SM2_DELAY        0.1//uS -- Syncmaster2 Delay
#define END_GATE_DELAY  100.0//nS -- Ending Gate Delay used with Sequential Gating
#define END_GATE_WIDTH  100.0//nS -- Ending Gate Width used with Sequential Gating
#define GAIN               20//   -- Intensifier Gain

#include "stdio.h"
#include "string.h"
#include "picam.h"

void SetCommonParameters( PicamHandle camera )
{
    Picam_SetParameterFloatingPointValue( camera, PicamParameter_TriggerFrequency, TRIG_FREQ );//internal trigger frequency
    Picam_SetParameterIntegerValue( camera, PicamParameter_IntensifierGain, GAIN );//intensifier gain
    Picam_SetParameterIntegerValue( camera, PicamParameter_TriggerSource, PicamTriggerSource_Internal );//use internal trigger
    Picam_SetParameterFloatingPointValue( camera, PicamParameter_SyncMaster2Delay, SM2_DELAY );//syncmaster2 delay time(uS)
    Picam_SetParameterIntegerValue( camera, PicamParameter_EnableSyncMaster, true );//turn on syncmaster
    Picam_SetParameterIntegerValue( camera, PicamParameter_EnableIntensifier, true );//turn on intensifier
}

void DoRepetitive( PicamHandle camera )
{
    PicamPulse genericPulser;

    Picam_SetParameterIntegerValue( camera, PicamParameter_GatingMode, PicamGatingMode_Repetitive );//set repetitve gating
    genericPulser.delay = GATE_DELAY;
    genericPulser.width = GATE_WIDTH;
    Picam_SetParameterPulseValue( camera, PicamParameter_RepetitiveGate, &genericPulser );//set GATE width&delay
    genericPulser.delay = AUX_DELAY;
    genericPulser.width = AUX_WIDTH;
    Picam_SetParameterPulseValue( camera, PicamParameter_AuxOutput, &genericPulser );//set AUX width&delay
}

void DoSequential( PicamHandle camera )
{
    PicamPulse genericPulser;

    Picam_SetParameterIntegerValue( camera, PicamParameter_GatingMode, PicamGatingMode_Sequential );//set sequential gating
    genericPulser.delay = 25000.0;
    genericPulser.width = 25000.0;
    Picam_SetParameterPulseValue( camera, PicamParameter_SequentialStartingGate, &genericPulser );//set Starting Gate width&delay
    genericPulser.delay = 100000.0;
    genericPulser.width = 100000.0;
    Picam_SetParameterPulseValue( camera, PicamParameter_SequentialEndingGate, &genericPulser );//set Ending Gate width&delay
    Picam_SetParameterLargeIntegerValue( camera, PicamParameter_SequentialGateStepCount, SEQ_FRAMES );//# of frames in sequence
    Picam_SetParameterLargeIntegerValue( camera, PicamParameter_SequentialGateStepIterations, 1 );
}
void DisableFeatures( PicamHandle camera )
{
    const PicamParameter* failed_parameter_array = NULL;
    piint           failed_parameter_count = 0;

    Picam_SetParameterIntegerValue( camera, PicamParameter_EnableSyncMaster, false );//disable syncmaster
    Picam_SetParameterIntegerValue( camera, PicamParameter_EnableIntensifier, false );//disable intensifier
    Picam_SetParameterIntegerValue( camera, PicamParameter_IntensifierGain, 1 );
	Picam_CommitParameters( camera, &failed_parameter_array, &failed_parameter_count );//commit parameters
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

    // - open the first camera if any
    PicamHandle camera;
	pibln committed;
    PicamCameraID id;
    const pichar* string;
    PicamAvailableData data;
	PicamAcquisitionErrorsMask errMask;
    piint readoutstride = 0;
    pibln IsGatingAvailable = false;

    if( Picam_OpenFirstCamera( &camera ) == PicamError_None )
        Picam_GetCameraID( camera, &id );
    else
    {
        printf( "No Camera Detected.  Exiting Program\n" );
		return 0;
    }
    Picam_GetEnumerationString( PicamEnumeratedType_Model, id.model, &string );
    printf( "%s", string );
    printf( " (SN:%s) [%s]\n", id.serial_number, id.sensor_name );
    Picam_DoesParameterExist( camera, PicamParameter_GatingMode, &IsGatingAvailable );
    if( !IsGatingAvailable )
    {
        Picam_DestroyString( string );
	    Picam_CloseCamera( camera );
        Picam_UninitializeLibrary();
        printf( "Gating not supported by this camera\n" );
        return 0;
    }
    Picam_DestroyString( string );

    Picam_GetParameterIntegerValue( camera, PicamParameter_ReadoutStride, &readoutstride );

    //Parameters common to both Repetitive & Sequential gating
    SetCommonParameters( camera );

    //Repetetive Gating
    DoRepetitive( camera );
	//commit the changes
	Picam_AreParametersCommitted( camera, &committed );
	if( !committed )
	{
		const PicamParameter* failed_parameter_array = NULL;
		piint           failed_parameter_count = 0;
		Picam_CommitParameters( camera, &failed_parameter_array, &failed_parameter_count );
		if( failed_parameter_count )
			Picam_DestroyParameters( failed_parameter_array );
		else
		{
            piint intensifierStatus = 0;
            Picam_GetParameterIntegerValue( camera, PicamParameter_IntensifierStatus, &intensifierStatus );
            printf( "Collecting 1 frame, looping %d times\n\n", NUM_FRAMES );
            for( piint i = 0; i < NUM_FRAMES; i++ )
            {
                if( Picam_Acquire( camera, 1, NO_TIMEOUT, &data, &errMask ) )
                    printf( "Error: Camera only collected %d frames\n", (piint)data.readout_count );
                else
                {    
                    PrintData( (pibyte*)data.initial_readout, 1, readoutstride );
                }
            }
		}
    }
    //Sequential Gating
    DoSequential( camera );
	//commit the changes
	Picam_AreParametersCommitted( camera, &committed );
	if( !committed )
	{
		const PicamParameter* failed_parameter_array = NULL;
		piint           failed_parameter_count = 0;
		Picam_CommitParameters( camera, &failed_parameter_array, &failed_parameter_count );
		if( failed_parameter_count )
			Picam_DestroyParameters( failed_parameter_array );
		else
		{
            piint intensifierStatus = 0;
            Picam_GetParameterIntegerValue( camera, PicamParameter_IntensifierStatus, &intensifierStatus );
            printf( "Collecting 1 frame, looping %d times\n\n", NUM_FRAMES );
            for( piint i = 0; i < SEQ_FRAMES; i++ )
            {
                if( Picam_Acquire( camera, 1, NO_TIMEOUT, &data, &errMask ) )
                    printf( "Error: Camera only collected %d frames\n", (piint)data.readout_count );
                else
                {    
                    PrintData( (pibyte*)data.initial_readout, 1, readoutstride );
                }
            }
		}
    }
    //disable SyncMaster(s)
    DisableFeatures( camera );
	Picam_CloseCamera( camera );
    Picam_UninitializeLibrary();
}
