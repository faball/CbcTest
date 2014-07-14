/*
	VCthScan( MinVCth, MaxVCth )
		Scan VCth from MinVCth to MaxVCth. 
		Single data aqcuisition for fNeventPerAcq events is done and ProcessCalibrationData() is called for each VCth.
		ScurveAnalyser->Configure() should be called before this method.

	ProcessCalibrationData( VCth, Data )
		Loop over events in Data and call ScurveAnalyser->FillHists( VCth, Event ) which fill the histogram of hit count distribution over VCth for each channel.	

	ActivateGroup()
		Should be called before ConfigureCbcOffset( OffsetTargetBit, MinVCth, MaxVCth )

	ScurveAnalyser->SetOffsets( TargetOffset ) 
		Set all offsets to the TargetOffset.

	ScurveAnalyser->Configure( OffsetTargetBit, Vplus )
		Reset histograms, update offsets, set target bit, set Vplus

	ConfigureCbcOffset( OffsetTargetBit, MinVCth, MaxVCth )
		ActivateGroup() should be called before this method.
		Offset value should be set in ScurvaAnalyser for channels in the test group. This is done in either ScurvaAnalyser->Configure() or ScurvaAnalyser->SetInitialOffset().
		For each channel,
			If OffsetTargetBit == 8 && not the cannel in test group, do nothing.
			If the channel is in test group, offset is set to the value from ScurveAnalyser, otherwise, fNonTestGroupOffset is set.

	ConfigureVplusScan( Vplus )
		Set Vplus.
			
*/
#ifndef __CALIBRATOR_H__
#define __CALIBRATOR_H__
#include <fstream>
//#include <netinet/in.h>
#include <TQObject.h>
#include <TString.h>
#include <map>
#include "Cbc/CbcRegInfo.h"
#include "CbcDaq/DAQController.h"

class TPad;

namespace Strasbourg{

	class Data;
	class Event;
}
using namespace Cbc;
using namespace Strasbourg;
using namespace CbcDaq;

namespace ICCalib{

	class ScurveAnalyser;
	class CalibrationResult;
	class TestGroupMap;

	typedef std::pair< std::string, UInt_t > CalibItem;
	typedef std::map< std::string, UInt_t > CalibSetting; 

	class Calibrator: public DAQController {

		public:
			Calibrator( const char *pConfigFile = 0 );
			~Calibrator();

			//main functions
			void ConfigureAnalyser();
			void Calibrate();
			void VCthScanForAllGroups();  

			//functions for configuration on the fly
			void SetCalibSetting( const char *pNode, UInt_t pValue ){ fCalibSetting.find( pNode )->second = pValue; }		

			//functions for interactive study 
			const CalibSetting &GetCalibSetting()const{ return fCalibSetting; }
			UInt_t GetCalibSetting( const char *pNode ) const { return fCalibSetting.find(pNode )->second; }
			const TestGroupMap *GetTestPulseGroupMap()const{ return fTestPulseGroupMap; }
			const TestGroupMap *GetTestGroupMap()const{ return fTestGroupMap; }
			const std::vector<UInt_t> &GetGroupList()const{ return fGroupList; }
			UInt_t GetCurrentTestPulseGroup()const{ return fCurrentTestPulseGroup; }
			const CalibrationResult *GetCalibrationResult()const;

			void SetVplusVsVCth0GraphDisplayPad( UInt_t pFeId, TPad *pPad );
			void SetScurveHistogramDisplayPad( UInt_t pFeId, UInt_t pCbcId, TPad *pPad );
			void SetGainGraphDisplayPad(UInt_t pFeId, TPad *pPad);//fb
			void ReadSettingFile( const char *pFname );

			//functions used internally for a moment.
			Int_t GetDataStreamHistId( UInt_t pFeId, UInt_t pCbcId );
			void FindVplus();
			void FindGain();//fb
			void CalibrateOffsets();

			void ConfigureVplusScan( UInt_t pVplus );
			void ActivateGroup( UInt_t pGroupId );
			void ConfigureCbcOffset( Int_t pTargetOffsetBit, UInt_t &pMinVCth, UInt_t &pMaxVCth );

			void GainScan();

			void VCthScanForVplusCalibration( UInt_t pVplus ); 
			void VCthScanForOffsetCalibration( Int_t pTargetBit ); 

			void VCthScan( UInt_t &pMinVCth, UInt_t &pMaxVCth );
			void PrintScurveHistogramDisplayPads();
			void DrawVplusVCthScanResult();
			void PrintVplusScan();
			void DrawGainScanResult();
			void PrintGainScan();
			void SetCalibratedOffsets();

			void ScanTestPulseDelay();

		private:
			void initialiseSetting();

			CalibSetting                    fCalibSetting;

			ScurveAnalyser                  *fScurveAnalyser;
			TestGroupMap                    *fTestPulseGroupMap;
			TestGroupMap                    *fTestGroupMap;

			std::vector<UInt_t>             fGroupList;
			Int_t                           fCurrentTestPulseGroup;
			
			UInt_t                          fNonTestGroupOffset;
			UInt_t							fNAcq;
	};
}
#endif

