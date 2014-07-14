#ifndef __GUICALIBRATORCONTROLBUTTONFRAME_H__
#define __GUICALIBRATORCONTROLBUTTONFRAME_H__

#include <TROOT.h>
#include <TGFrame.h>
#include "CbcDaq/GUIControlButtonFrame.h"

using namespace CbcDaq;

namespace ICCalib {

	class CalibratorControlButtonFrame : public ControlButtonFrame {

		public:
			enum CalibState { VCthScanRunning, CalibrationRunning, DelayScanRunning }; //fb

		public:
			CalibratorControlButtonFrame( TGCompositeFrame *pFrame, CalibratorFrame *pCalibratorFrame );
			virtual ~CalibratorControlButtonFrame(){;}
//			void SetMainProcessButtons( EButtonState pState, ULong_t pColor );
//			void ResetMainProcessButtons();
//			void SetButtonState( State pState ); 
			void SetCalibButtonState( CalibState pState );
//			Bool_t ToggleDataStreamDisplayButton();
			ClassDef( ControlButtonFrame, 0 );
		private:
			void addProcessButtons();
			TGTextButton *fCalibrationButton;
			TGTextButton *fVCthScanButton;
			TGTextButton *fDelayScanButton; //fb
	};
}
#endif

