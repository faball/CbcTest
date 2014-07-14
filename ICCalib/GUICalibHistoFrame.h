#ifndef __GUIHISTOGRAMFRAME_H__
#define __GUIHISTOGRAMFRAME_H__

#include <TGFrame.h>

class TGTab;
namespace ICCalib{

	class VplusVsVCth0GraphFrame : public TGCompositeFrame {

		public:
			VplusVsVCth0GraphFrame( TGCompositeFrame *pFrame, CalibratorFrame *pCalibratorFrame );
			~VplusVsVCth0GraphFrame(){}
			void RenewFrame();
			ClassDef( VplusVsVCth0GraphFrame, 0 );
		private:
			TGCompositeFrame *fMotherFrame;
			CalibratorFrame  *fCalibratorFrame;
			TGTab            *fFrame;
	};

	class ScurveHistogramFrame : public TGCompositeFrame {

		public:
			ScurveHistogramFrame( TGCompositeFrame *pFrame, CalibratorFrame *pCalibratorFrame );
			virtual ~ScurveHistogramFrame(){}
			void RenewFrame();
			ClassDef( ScurveHistogramFrame, 0 );
		private:
			TGCompositeFrame *fMotherFrame;
			CalibratorFrame  *fCalibratorFrame;
			TGTab            *fFrame;
	};

	class DataStreamFrame : public TGCompositeFrame {

		public:
			DataStreamFrame( TGCompositeFrame *pFrame, GUIFrame *pGUIFrame );
			virtual ~DataStreamFrame(){}
			void RenewFrame();
			ClassDef( DataStreamFrame, 0 );
		private:
			TGCompositeFrame *fMotherFrame;
			GUIFrame         *fGUIFrame;
			TGTab            *fFrame;
	};

	class GainGraphFrame : public TGCompositeFrame { //fb

		public:
			GainGraphFrame( TGCompositeFrame *pFrame, CalibratorFrame *pCalibratorFrame );
			~GainGraphFrame(){}
			void RenewFrame();
			ClassDef( GainGraphFrame, 0 );
		private:
			TGCompositeFrame *fMotherFrame;
			CalibratorFrame  *fCalibratorFrame;
			TGTab            *fFrame;
	};
}
#endif

