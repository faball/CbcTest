#include "GUICalibratorControlButtonFrame.h"

namespace ICCalib{

	CalibratorControlButtonFrame::CalibratorControlButtonFrame( TGCompositeFrame *pFrame, CalibratorFrame *pCalibratorFrame ):
		ControlButtonFrame( pFrame, pCalibratorFrame ){
		}

	void CalibratorControlButtonFrame::addProcessButtons(){

		fCalibrationButton = new TGTextButton( this, "&Calibrate" );
		fCalibrationButton->Connect( "Clicked()", "ICCalib::CalibratorFrame", fGUIFrame, "DoCalibration()" );
		AddFrame( fCalibrationButton, new TGLayoutHints( kLHintsCenterX, 5, 5, 3, 4 ) );
		fMainControlButtons.push_back( fCalibrationButton );
		fProcessButtons.push_back( fCalibrationButton );

		fVCthScanButton = new TGTextButton( this, "&VCthScan" );
		fVCthScanButton->Connect( "Clicked()", "ICCalib::CalibratorFrame", fGUIFrame, "DoVCthScan()" );
		AddFrame( fVCthScanButton, new TGLayoutHints( kLHintsCenterX, 5, 5, 3, 4 ) );
		fMainControlButtons.push_back( fVCthScanButton );
		fProcessButtons.push_back( fVCthScanButton );
	}

	void CalibratorControlButtonFrame::SetCalibButtonState( CalibState pState ){ 

		ULong_t cColor(0);

		switch( pState ){

			case VCthScanRunning :

				gClient->GetColorByName( "green", cColor );
				fVCthScanButton->SetBackgroundColor( cColor ); 
				fVCthScanButton->SetState( kButtonDisabled );
				gClient->NeedRedraw( fVCthScanButton );
				break;

			case CalibrationRunning :

				gClient->GetColorByName( "green", cColor );
				fCalibrationButton->SetBackgroundColor( cColor ); 
				fCalibrationButton->SetState( kButtonDisabled );
				gClient->NeedRedraw( fCalibrationButton );
				break;
			default:
				break;
		}
	}
}

