/*
	Application to scan VCth for test pulse with different potentiometer 
	USAGE: vcthscan ( [mode] [potentiometer value] [setting filename] )
		mode                : electron or hole
		potentiometer value : if not given or -1, no test pulse

		default mode is electron with no testpulse.
			Configuration files are expected to be as follows,
				electron : ./settings/CbcCalibElectron.txt
				hole     : ./settings/CbcCalibHole.txt
			If those files do not exist, the application still runs with the default setting found in DAQController.cc


*/
#include <iostream>
#include <string>
#include <exception>
#include <cstdlib>
#include "ICCalib/Calibrator.h"
#include "TApplication.h"
#include <iostream>

int main( int argc, char *argv[] ){

	std::string cConfigFileName = "settings/CbcCalibElectron.txt";
	int cTestPulsePot(-1);	
	if( argc >= 3 ){
		if( !strcmp( argv[1], "hole" ) ){

			cConfigFileName = "settings/CbcCalibHole.txt";
		}	
		cTestPulsePot = strtol( argv[2], 0, 16 );
		if( argc == 4 ){
			cConfigFileName = std::string( argv[3] );
		}
	}
	else if( argc != 1 ){
		std::cout << "USAGE: vcthscan [hole/ele] [test pulse pottentiometer value in hexadecimal( 0xF1 ) : if not given or -1, no test pulse] [configuration file name]" << std::endl;
	}

	ICCalib::Calibrator daq( cConfigFileName.c_str() );

	TQObject::Connect( &daq, "Message( const char * )", 0, 0, "PrintLog( const char * )" ); 

	daq.Initialise();

	if( cTestPulsePot != -1 ){
		daq.SetCalibSetting( "EnableTestPulse", 1 );
		daq.SetCalibSetting( "TestPulsePotentiometer", cTestPulsePot );
	}

	daq.ConfigureGlib();

	daq.ConfigureCbc();
	daq.ConfigureCbc();

	daq.ConfigureAnalyser();

	daq.ScanTestPulseDelay();

	//daq.VCthScanForAllGroups();

	//daq.GainTest(); //fb

}

