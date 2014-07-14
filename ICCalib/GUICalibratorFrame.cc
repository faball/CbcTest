#include "CbcDaq/GUI.h"
#include "CbcDaq/GUILogFrame.h"
#include "GUICalibratorFrame.h"
#include "GUICalibConfigFrame.h"
#include "GUICalibratorControlButtonFrame.h"
#include "GUICalibHistoFrame.h"
#include "Calibrator.h"
#include <TThread.h>
#include <TGFrame.h>
#include <iostream>
#include <TGTab.h>
#include <TGNumberEntry.h>

namespace ICCalib{

	CalibratorFrame::CalibratorFrame( const TGWindow *p, UInt_t w, UInt_t h, 
			const char *pWindowName, Calibrator *pCalibrator ): 
		GUIFrame( p, w, h, pWindowName, pCalibrator ),
		fThreadCalib(0), 
		fThreadVCthScan(0),
		fThreadDelayScan(0),//fb
		fCalibrator( pCalibrator ),
		fCalibConfigFrame(0),
		fVplusVsVCth0GraphFrame(0),
		fGainGraphFrame(0), //fb
		fScurveHistogramFrame(0),
		fDataStreamFrame(0){
		}

	void CalibratorFrame::AddAnalyserFrames( TGTab *pTab ){

		TGCompositeFrame *cTabFrame = pTab->AddTab( "Calibration configuration" );
		fCalibConfigFrame = new CalibrationConfigurationFrame( cTabFrame, this );

		cTabFrame = pTab->AddTab( "Vplus vs. VCth0 Graphs" );
		fVplusVsVCth0GraphFrame = new VplusVsVCth0GraphFrame( cTabFrame, this );

		cTabFrame = pTab->AddTab( "Scurve Histograms" );
		fScurveHistogramFrame = new ScurveHistogramFrame( cTabFrame, this );

		cTabFrame = pTab->AddTab( "Data" );
		fDataStreamFrame = new DataStreamFrame( cTabFrame, this );

		cTabFrame = pTab->AddTab( "Gain Graph" ); //fb
		fGainGraphFrame = new GainGraphFrame( cTabFrame, this );
	}
	void CalibratorFrame::AddControlButtonFrame(){
		//Controll button frame
		fControlButtonFrame = new CalibratorControlButtonFrame( this, this );
		fControlButtonFrame->AddButtons();
	}

	void CalibratorFrame::ProcessDAQControllerMessage( const char *pMessage ){

		fLogFrame->AddText( new TGText( pMessage ) );

		if( ! strcmp( pMessage, "FinishedCalibration" ) || ! strcmp( pMessage, "FinishedVCthScan" ) ){

			fControlButtonFrame->SetButtonState( ControlButtonFrame::Stopped );
		}
	}

	void CalibratorFrame::ConfigureAnalyser(){

		//Set setting from GUI to Calibrator and configure Calibration 
		const CalibSettingEntries &cCalibSettingEntries = fCalibConfigFrame->GetCalibSettingEntries();
		CalibSettingEntries::const_iterator cIt = cCalibSettingEntries.begin();
		for( ; cIt != cCalibSettingEntries.end(); cIt++ ){
			Int_t cValue = (Int_t) cIt->second->GetNumber();
			fCalibrator->SetCalibSetting( cIt->first.c_str(), cValue ); 
		}
		fCalibrator->ConfigureAnalyser();

		fVplusVsVCth0GraphFrame->RenewFrame();
		fScurveHistogramFrame->RenewFrame();
		fDataStreamFrame->RenewFrame();
		fGainGraphFrame->RenewFrame();//fb

		fControlButtonFrame->SetButtonState( ControlButtonFrame::AnalyserConfigured );
	}
	void CalibratorFrame::DoVCthScan(){

		fControlButtonFrame->SetButtonState( ControlButtonFrame::Running );
		( ( CalibratorControlButtonFrame *) fControlButtonFrame )->SetCalibButtonState( CalibratorControlButtonFrame::VCthScanRunning );

		if( fThreadVCthScan ) fThreadVCthScan->Delete();
		fThreadVCthScan = new TThread( "vcthscan", CalibratorFrame::DoVCthScan, this );
		fThreadVCthScan->Run();
	}
	void CalibratorFrame::DoVCthScan( void *p ){

		CalibratorFrame *obj = (CalibratorFrame *) p;
		obj->fCalibrator->VCthScanForAllGroups();

	}

	void CalibratorFrame::DoCalibration(){

		fControlButtonFrame->SetButtonState( ControlButtonFrame::Running );
		( ( CalibratorControlButtonFrame *) fControlButtonFrame )->SetCalibButtonState( CalibratorControlButtonFrame::CalibrationRunning );

		if( fThreadCalib ) fThreadCalib->Delete();
		fThreadCalib = new TThread( "calib", CalibratorFrame::DoCalibration, this );
		fThreadCalib->Run();
	}
	void CalibratorFrame::DoCalibration( void *p ){

		CalibratorFrame *obj = (CalibratorFrame *)p;
		obj->fCalibrator->Calibrate();
	}

	void CalibratorFrame::DoDelayScan(){ //fb

			fControlButtonFrame->SetButtonState( ControlButtonFrame::Running );
			( ( CalibratorControlButtonFrame *) fControlButtonFrame )->SetCalibButtonState( CalibratorControlButtonFrame::DelayScanRunning );

			if( fThreadDelayScan ) fThreadDelayScan->Delete();
			fThreadDelayScan = new TThread( "delayscan", CalibratorFrame::DoDelayScan, this );
			fThreadDelayScan->Run();
		}
	void CalibratorFrame::DoDelayScan( void *p ){

			CalibratorFrame *obj = (CalibratorFrame *) p;
			obj->fCalibrator->ScanTestPulseDelay();

		}
}

