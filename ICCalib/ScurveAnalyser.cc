#include "ScurveAnalyser.h"
#include "TString.h"
#include "TCanvas.h"
#include <math.h>
#include <TH1F.h>
#include "TF1.h"
#include "TMath.h"
#include <TFile.h>
#include <TBinomialEfficiencyFitter.h>
#include <TLegend.h>
#include <TThread.h>
#include <TGraphErrors.h>
#include "Cbc/CbcRegInfo.h"

namespace ICCalib{

	Double_t MyErf( Double_t *x, Double_t *par ){
		Double_t x0 = par[0];
		Double_t width = par[1];
		Double_t fitval(0);
		if( x[0] < x0 ) fitval = 0.5 * TMath::Erfc( ( x0 - x[0] )/width );
		else fitval = 0.5 + 0.5 * TMath::Erf( ( x[0] - x0 )/width );
		return fitval;
	}
	ScurveAnalyser::ScurveAnalyser( 
			UInt_t pBeId, UInt_t pNFe, UInt_t pNCbc,
			TestGroupMap *pGroupMap, const CbcRegMap *pCbcRegMap, 
			Bool_t pNegativeLogicCbc, UInt_t pTargetVCth, const char *pOutputDir, GUIFrame *pGUIFrame ):
		Analyser( pBeId, pNFe, pNCbc, pCbcRegMap, pNegativeLogicCbc, pOutputDir, pGUIFrame ),
		fGroupMap(pGroupMap),  
		fTargetVCth(pTargetVCth), 
		fCurrentTargetBit(8),
		fChannelList(0){

		}
	void ScurveAnalyser::Initialise(){

		Analyser::Initialise();

		for( UInt_t i=0; i < fChannelList.size(); i++ ){

			delete fChannelList[i];
		}
		fChannelList.clear();
		fMidPoints.clear();//fb

		TestGroupMap::iterator cIt = fGroupMap->begin(); 
		for( ; cIt != fGroupMap->end(); cIt++ ){

			TestGroup &cTestGroup = cIt->second;
			cTestGroup.ClearChannelList();
		}
		fResult.clear();

		fNthVplusPoint = 0;	


		TString cName = Form( "totalBE%u", fBeId ); 
		TObject *cObj = gROOT->FindObject( cName );
		if( cObj ) delete cObj;
		fHtotal = new TH1F( cName, "total", 256, -0.5, 255.5 ); 
		fHtotal->Sumw2();
		fHtotal->SetLineColor(kSpring);
		fHtotal->SetMarkerStyle( 20 );

		for( UInt_t cFeId=0; cFeId < fNFe; cFeId++ ){

			for( UInt_t cCbcId=0; cCbcId< fNCbc; cCbcId++ ){

				cIt = fGroupMap->begin(); 
				for( ; cIt != fGroupMap->end(); cIt++ ){

					TestGroup &cTestGroup = cIt->second;

					for( UInt_t i=0; i<cTestGroup.size(); i++ ){

						UInt_t cChannelId = cTestGroup[i];
						TH1F *h = createScurveHistogram( cFeId, cCbcId, cChannelId );
						Channel *cChannel = new Channel( cFeId, cCbcId, cChannelId );
						cChannel->SetHist( h );

						fChannelList.push_back( cChannel );
						fResult.AddChannel( cFeId, cCbcId, cChannelId, cChannel );
						cTestGroup.AddChannel( cChannel ); 
					}

					TString cName = Form("gBe%uFe%uCbc%uGroup%u", fBeId, cFeId, cCbcId, cIt->first );
					delete gROOT->FindObject( cName );
					TGraphErrors *cG = new TGraphErrors( cTestGroup.size() * 10 );
					cG->SetName( cName );
					fResult.AddGraphVplusVCth0( cFeId, cCbcId, cIt->first, cG );

					fResult.AddGraphGain( cFeId, cCbcId, cIt->first, cG ); //fb
				}
			}
		}
		TString cHistName( "hDummyHist" );
		if( ! gROOT->FindObject( cHistName )  ) {
			fDummyHist = new TH1F( cHistName, "Fit; VCth; Rate", 256, -0.5, 255.5 );
			fDummyHist->SetMinimum(0);
			fDummyHist->SetMaximum(1);
		}

#ifdef __CBCDAQ_DEV__
		std::cout << "ScurveAnalyser::Initialise() done" << std::endl;
#endif
	}

	void ScurveAnalyser::SetOffsets(){

		for( UInt_t i=0; i<fChannelList.size(); i++ ){
			UInt_t cFeId = fChannelList[i]->FeId();
			UInt_t cCbcId = fChannelList[i]->CbcId();
			UInt_t cChannel = fChannelList[i]->ChannelId();
			UInt_t cOffset = fCbcRegMap->GetValue( cFeId, cCbcId, 1, cChannel+1 );
			//std::cout << "Fe " << cFeId << ", Cbc " << cCbcId << ", Offset " << cOffset << std::endl;
			fChannelList[i]->SetOffset( cOffset );
			fChannelList[i]->SetNextOffset( cOffset );
		}
	}
	void ScurveAnalyser::SetOffsets( UInt_t pInitalOffset ){

		for( UInt_t i=0; i < fChannelList.size(); i++ ){
			if( fChannelList[i] == 0 ) continue;
			fChannelList[i]->SetOffset( pInitalOffset );
			fChannelList[i]->SetNextOffset( pInitalOffset );
		}
	}
	void ScurveAnalyser::Configure( Int_t pType, Int_t pOffsetTargetBit ){

		fScanType = pType;
		resetHistograms();
		if( -1 <= pOffsetTargetBit && pOffsetTargetBit <= 7 ) updateOffsets();
		fCurrentTargetBit = pOffsetTargetBit;

	}
	int ScurveAnalyser::FillGraphVplusVCth0(){

		UInt_t cGroupId(0);
		TestGroup *cTestGroup = fGroupMap->GetActivatedGroup(cGroupId);
		if( cTestGroup == 0 ) return -1;

		CalibrationResult::iterator cItFe = fResult.begin();
		for( ; cItFe != fResult.end(); cItFe++ ){

			UInt_t cFeId = cItFe->first;
			FeInfo &cFeInfo = cItFe->second;
			FeInfo::iterator cItCbc = cFeInfo.begin();
			for( ; cItCbc != cFeInfo.end(); cItCbc++ ){

				UInt_t cCbcId = cItCbc->first;
				CbcInfo &cCbcInfo = cItCbc->second;

				TGraphErrors *cG = cCbcInfo.GetGraphVplusVCth0(cGroupId);

				const std::vector<Channel *> *cChannelList = cTestGroup->GetChannelList();
				UInt_t j=0;
				for( UInt_t i=0; i < cChannelList->size(); i++ ){
					Channel *cChannel = cChannelList->at(i);
					if( !( cChannel->FeId() == cFeId && cChannel->CbcId() == cCbcId ) )continue; 
					UInt_t cVplus = fCbcRegMap->GetValue( cFeId, cCbcId, 0, 0x0B );
					cG->SetPoint( fNthVplusPoint+j, cChannel->VCth0(), cVplus ); 
					cG->SetPointError( fNthVplusPoint+j, cChannel->VCth0Error(), 0 ); 
					//					std::cout << "SetPoint [" << fNthVplusPoint+j << "] for Channel " << cChannel->ChannelId() << " Vplus " << cVplus << " VCth0 = " << cChannel->VCth0() << std::endl;
					j++;
				}
			}
		}
		fNthVplusPoint += cTestGroup->size();
		return 0;
	}

	int ScurveAnalyser::FillGraphGain(){ //fb
		UInt_t cGroupId(0);
		TestGroup *cTestGroup = fGroupMap->GetActivatedGroup(cGroupId);
		if( cTestGroup == 0 ) return -1;

		CalibrationResult::iterator cItFe = fResult.begin();
		for( ; cItFe != fResult.end(); cItFe++ ){

			UInt_t cFeId = cItFe->first;
			FeInfo &cFeInfo = cItFe->second;
			FeInfo::iterator cItCbc = cFeInfo.begin();
			for( ; cItCbc != cFeInfo.end(); cItCbc++ ){

				UInt_t cCbcId = cItCbc->first;
				CbcInfo &cCbcInfo = cItCbc->second;

				TString cHtotalName =  Form( "h_FE%dCBC%dG%03d%s", cFeId, cCbcId, cGroupId, getScanId().Data() );//

				TGraphErrors *cG = cCbcInfo.GetGraphGain(cGroupId);

				//UInt_t cChannelId(0);

				const std::vector<Channel *> *cChannelList = cTestGroup->GetChannelList();
				UInt_t j=0;
				for( UInt_t i=0; i < cChannelList->size(); i++ ){
					Channel *cChannel = cChannelList->at(i);
					if( !( cChannel->FeId() == cFeId && cChannel->CbcId() == cCbcId ) )continue;

					TH1F *h = cChannel->Hist();

					TF1 *cFunc = (TF1 *) h->GetListOfFunctions()->Last();
					double cVCth0(0);
					double cVCthError(0);
					if( cFunc ) {
						cVCth0 = cFunc->GetParameter(0);

						cVCthError = cFunc->GetParError(0);
					}
					else{
						cVCth0 = cChannel->VCth0();
					}

					UInt_t cTpPot = fCbcRegMap->GetValue( cFeId, cCbcId, 0, 0x0D );
					cG->SetPoint( fNthVplusPoint+j, cTpPot, cVCth0  ); //n, x, y
					cG->SetPointError( fNthVplusPoint+j, cChannel->VCth0Error(), 0 );
					j++;
				}
			}
		}
		fNthVplusPoint += cTestGroup->size();
		return 0;
	}





	void ScurveAnalyser::DrawGain(){

		UInt_t cGroupId(0);
		TestGroup *cTestGroup = fGroupMap->GetActivatedGroup(cGroupId);
		if( cTestGroup == 0 ) return;

		CalibrationResult::iterator cItFe = fResult.begin();
		for( ; cItFe != fResult.end(); cItFe++ ){

			UInt_t cFeId = cItFe->first;
			FeInfo &cFeInfo = cItFe->second;
			FeInfo::iterator cItCbc = cFeInfo.begin();
			for( ; cItCbc != cFeInfo.end(); cItCbc++ ){

				UInt_t cCbcId = cItCbc->first;
				CbcInfo &cCbcInfo = cItCbc->second;
				TPad *cPad = cCbcInfo.GetGainGraphDisplayPad();
				if( cPad ){

					cPad->cd();

					TGraphErrors *cG = cCbcInfo.GetGraphGain(cGroupId);
					TString cOption( "P" );
					TString cHistName = Form( "GainScanBE%uFE%uCBC%u", fBeId, cFeId, cCbcId);
					if( ! gROOT->FindObject( cHistName )  ) {
						TH1F *h = new TH1F( cHistName, Form( "GainScan BE %u FE %u CBC %u;TpPot;MidPoint", fBeId, cFeId, cCbcId ), 1, 0, 260 );
						h->SetMaximum( 260 );
						h->GetXaxis()->SetLabelSize( 0.05 );
						h->GetYaxis()->SetLabelSize( 0.04 );
						h->GetXaxis()->SetTitleSize( 0.05 );
						h->GetYaxis()->SetTitleSize( 0.05 );
						h->GetXaxis()->SetTitleOffset( 1.0 );
						h->GetYaxis()->SetTitleOffset( 1.0 );
					}
					if( cGroupId == 0 ){
						cPad->Clear();
						TH1F *h = (TH1F *) gROOT->FindObject( cHistName );
						h->Draw();
					}
					cG->SetMarkerStyle(20);
					cG->SetMarkerColor( cGroupId + 1 );
					cG->SetLineColor( cGroupId + 1 );
					cG->Draw( cOption );
					cPad->Update();
				}
			}
		}

	}

	void ScurveAnalyser::DrawVplusVCth0(){

		UInt_t cGroupId(0);
		TestGroup *cTestGroup = fGroupMap->GetActivatedGroup(cGroupId);
		if( cTestGroup == 0 ) return;

		CalibrationResult::iterator cItFe = fResult.begin();
		for( ; cItFe != fResult.end(); cItFe++ ){

			UInt_t cFeId = cItFe->first;
			FeInfo &cFeInfo = cItFe->second;
			FeInfo::iterator cItCbc = cFeInfo.begin();
			for( ; cItCbc != cFeInfo.end(); cItCbc++ ){

				UInt_t cCbcId = cItCbc->first;
				CbcInfo &cCbcInfo = cItCbc->second;
				TPad *cPad = cCbcInfo.GetVplusVsVCth0GraphDisplayPad();	
				if( cPad ){

					cPad->cd();

					TGraphErrors *cG = cCbcInfo.GetGraphVplusVCth0(cGroupId);
					TString cOption( "P" );
					TString cHistName = Form( "VplusScanBE%uFE%uCBC%u", fBeId, cFeId, cCbcId);

					if( ! gROOT->FindObject( cHistName )  ) {
						TH1F *h = new TH1F( cHistName, Form( "VplusScan BE %u FE %u CBC %u;VCth;Vplus", fBeId, cFeId, cCbcId ), 1, 0, 260 ); 
						h->SetMaximum( 260 );
						h->GetXaxis()->SetLabelSize( 0.05 );
						h->GetYaxis()->SetLabelSize( 0.04 );
						h->GetXaxis()->SetTitleSize( 0.05 );
						h->GetYaxis()->SetTitleSize( 0.05 );
						h->GetXaxis()->SetTitleOffset( 1.0 );
						h->GetYaxis()->SetTitleOffset( 1.0 );
					}
					if( cGroupId == 0 ){
						cPad->Clear();
						TH1F *h = (TH1F *) gROOT->FindObject( cHistName );
						h->Draw();
					}
					cG->SetMarkerStyle(20);
					cG->SetMarkerColor( cGroupId + 1 );
					cG->SetLineColor( cGroupId + 1 );
					cG->Draw( cOption );
					cPad->Update();
				}
			}
		}
	}
	void ScurveAnalyser::PrintVplusVsVCth0DisplayPads(){

		CalibrationResult::iterator cItFe = fResult.begin();
		for( ; cItFe != fResult.end(); cItFe++ ){

			UInt_t cFeId = cItFe->first;
			FeInfo &cFeInfo = cItFe->second;
			FeInfo::iterator cItCbc = cFeInfo.begin();
			for( ; cItCbc != cFeInfo.end(); cItCbc++ ){

				UInt_t cCbcId = cItCbc->first;
				CbcInfo &cCbcInfo = cItCbc->second;
				TPad *cPad = cCbcInfo.GetVplusVsVCth0GraphDisplayPad();
				if( cPad ){
					cPad->cd();
					//cPad->Update();
					cPad->Print( Form( "%s/FE%uCBC%u_VplusScan.eps", fOutputDir.Data(), cFeId, cCbcId ) );
				}
			}
		}
	}

	void ScurveAnalyser::PrintGainDisplayPads(){

		CalibrationResult::iterator cItFe = fResult.begin();
		for( ; cItFe != fResult.end(); cItFe++ ){

			UInt_t cFeId = cItFe->first;
			FeInfo &cFeInfo = cItFe->second;
			FeInfo::iterator cItCbc = cFeInfo.begin();
			for( ; cItCbc != cFeInfo.end(); cItCbc++ ){

				UInt_t cCbcId = cItCbc->first;
				CbcInfo &cCbcInfo = cItCbc->second;
				TPad *cPad = cCbcInfo.GetGainGraphDisplayPad();
				if( cPad ){
					cPad->cd();
					//cPad->Update();
					UInt_t cTpPot = fCbcRegMap->GetValue( cFeId, cCbcId, 0, 0x0D );
					cPad->Print( Form( "%s/FE%uCBC%uTpPot0x%02X_GainScan.eps", fOutputDir.Data(), cFeId, cCbcId,cTpPot ) );
				}
			}
		}
	}

	int ScurveAnalyser::FillHists( UInt_t pVcth, const Event *pEvent ){

		UInt_t cGroupId(0);
		TestGroup *cTestGroup = fGroupMap->GetActivatedGroup(cGroupId);
		if( cTestGroup == 0 ) return -1;

		UInt_t cNhit(0);
		UInt_t cFeId(0), cCbcId(0), cChannelId(0);

		const std::vector<Channel *> *cChannelList = cTestGroup->GetChannelList(); 
		for( UInt_t k=0; k < cChannelList->size(); k++ ){

			Channel *cChannel = cChannelList->at(k);

			cChannel->Id( cFeId, cCbcId, cChannelId );
			TH1F *h = cChannel->Hist();

			const FeEvent *cFeEvent = pEvent->GetFeEvent( cFeId );
			if( cFeEvent == 0 ) return cNhit; 
			const CbcEvent *cCbcEvent = cFeEvent->GetCbcEvent( cCbcId );
			if( cCbcEvent == 0 ) return cNhit;
			bool cBitValue = cCbcEvent->DataBit( cChannelId );

			if( cBitValue ) {
				h->Fill( pVcth );
				cNhit++;
			}
		}
		fHtotal->Fill( pVcth );

		return cNhit;
	}
	void ScurveAnalyser::FitHists( UInt_t pMin, UInt_t pMax ){

		UInt_t cGroupId(0);
		TestGroup *cTestGroup = fGroupMap->GetActivatedGroup(cGroupId);
		if( cTestGroup == 0 ) return;

		CalibrationResult::iterator cItFe = fResult.begin();
		for( ; cItFe != fResult.end(); cItFe++ ){

			FeInfo &cFeInfo = cItFe->second;
			FeInfo::iterator cItCbc = cFeInfo.begin();
			for( ; cItCbc != cFeInfo.end(); cItCbc++ ){

				CbcInfo &cCbcInfo = cItCbc->second;
				cCbcInfo.GetDummyPad()->cd();
				//cCbcInfo.GetDummyPad()->Clear();
				fDummyHist->Draw();
			}
		}

		TFile fout( Form( "%s/%s.root", fOutputDir.Data(), getScanId().Data() ), "UPDATE" );
		fMinVCth0 = 0xFF;
		fMaxVCth0 = 0x0;
		UInt_t cFeId(0), cCbcId(0), cChannelId(0);

		const std::vector<Channel *> *cChannelList = cTestGroup->GetChannelList(); 
		for( UInt_t k=0; k < cChannelList->size(); k++ ){

			Channel *cChannel = cChannelList->at(k);
			if( cChannel == 0 ) continue;

			cChannel->Id( cFeId, cCbcId, cChannelId );
			TH1F *h = cChannel->Hist();
			if(h==0) continue;

			h->Divide( h, fHtotal, 1.0, 1.0, "B" );
			double cFirstNon0(0);
			double cFirst1(0);
			if( fNegativeLogicCbc ){
				for( Int_t cBin = 1; cBin <= h->GetNbinsX(); cBin++ ){
					double cContent = h->GetBinContent( cBin );
					if( !cFirstNon0	){
						if( cContent ) cFirstNon0 = h->GetBinCenter(cBin);
					}
					else if( cContent == 1 ) {
						cFirst1 = h->GetBinCenter(cBin); 
						break;
					}
				}
			}
			else{
				for( Int_t cBin = h->GetNbinsX(); cBin >=1; cBin-- ){
					double cContent = h->GetBinContent( cBin );
					if( !cFirstNon0	){
						if( cContent ) cFirstNon0 = h->GetBinCenter(cBin);
					}
					else if( cContent == 1 ) {
						cFirst1 = h->GetBinCenter(cBin); 
						break;
					}
				}
			}
			double cMid = ( cFirst1 + cFirstNon0 ) * 0.5;
			double cWidth = ( cFirst1 - cFirstNon0 ) * 0.5;
			TString fname = Form( "f_%s", h->GetName() ); 
			TObject *cObj = gROOT->FindObject( fname );
			if( cObj ) delete cObj;
			TF1 *f = new TF1( fname, MyErf, pMin, pMax, 2 ); 
			f->SetParameters( cMid, cWidth );
			//Option S is for TFitResultPtr
			//TFitResultPtr cFitR = h->Fit( fname, "RSLQ0" ); 
			fResult.getCbcInfo( cFeId, cCbcId )->GetDummyPad()->cd();
			TFitResultPtr cFitR = h->Fit( fname, "RSLQ", "same" ); 
			//			TFitResultPtr cFitR = h->Fit( fname, "RSLQ" ); 
			int cStatus = int( cFitR ); 
			if( cStatus != 0 ) {
				std::cout << "Fit failed for FE " <<  cChannel->FeId() << " CBC " << cChannel->CbcId() 
					<< " Channel " << cChannel->ChannelId() << " status " << cStatus << " "
					<< gDirectory->GetName() << " " << h->GetName() << std::endl;
			}
			double cVCth0 = f->GetParameter( 0 ); 
			double cVCth0Error = f->GetParError( 0 ); 
			cChannel->SetVCth0( (float)cVCth0, (float) cVCth0Error );
			fMinVCth0 = cVCth0 < (double)fMinVCth0 ? (UInt_t)cVCth0 : fMinVCth0;
			fMaxVCth0 = (double)fMaxVCth0 < cVCth0 ? (UInt_t)cVCth0 : fMaxVCth0;
			h->Write(h->GetName(), TObject::kOverwrite );
		}
		fHtotal->Write (fHtotal->GetName(), TObject::kOverwrite );
		fHtotal->Divide( fHtotal, fHtotal, 1.0, 1.0, "B" );
		fout.Close();
		setNextOffsets();

		cItFe = fResult.begin();
		for( ; cItFe != fResult.end(); cItFe++ ){

			FeInfo &cFeInfo = cItFe->second;
			FeInfo::iterator cItCbc = cFeInfo.begin();
			for( ; cItCbc != cFeInfo.end(); cItCbc++ ){

				CbcInfo &cCbcInfo = cItCbc->second;
				cCbcInfo.GetDummyPad()->Update();
			}
		}
	}
	void ScurveAnalyser::DrawHists(){

		UInt_t cGroupId(0);
		TestGroup *cTestGroup = fGroupMap->GetActivatedGroup(cGroupId);
		if( cTestGroup == 0 ) return;

		CalibrationResult::iterator cItFe = fResult.begin();
		for( ; cItFe != fResult.end(); cItFe++ ){

			UInt_t cFeId = cItFe->first;
			FeInfo &cFeInfo = cItFe->second;
			FeInfo::iterator cItCbc = cFeInfo.begin();
			for( ; cItCbc != cFeInfo.end(); cItCbc++ ){

				UInt_t cCbcId = cItCbc->first;
				CbcInfo &cCbcInfo = cItCbc->second;
				TPad *cPad = cCbcInfo.GetScurveHistogramDisplayPad( cGroupId );	
				if( cPad ){
					cPad->cd();
					cPad->Clear();
					//TString option("e0p0");
					TString option("hist");
					TString cHtotalName =  Form( "h_FE%dCBC%dG%03d%s", cFeId, cCbcId, cGroupId, getScanId().Data() );
					int cTotalColor = 1;

					TObject *cObj = gROOT->FindObject( cHtotalName );
					if( cObj ) delete cObj;
					TH1F *cHtotal = (TH1F *) fHtotal->Clone( cHtotalName );
					cHtotal->SetLineColor(cTotalColor);
					TString cYaxisTitle("Events");
					if( cHtotal->GetMaximum() < 1.5 ) cYaxisTitle = "Rate";
					cHtotal->GetXaxis()->SetLabelSize( 0.06 );
					cHtotal->GetYaxis()->SetLabelSize( 0.05 );
					cHtotal->GetXaxis()->SetTitleSize( 0.06 );
					cHtotal->GetYaxis()->SetTitleSize( 0.05 );
					cHtotal->GetXaxis()->SetTitleOffset( 0.85 );
					cHtotal->GetYaxis()->SetTitleOffset( 0.4 );
					cHtotal->GetYaxis()->SetTitle( cYaxisTitle );
					cHtotal->Draw( option );

					TLegend *cLegend0 = new TLegend( 0.85, 0.15, 0.98, 0.95);
					TLegend *cLegend1 = new TLegend( 0.72, 0.15, 0.85, 0.95);
					cLegend0->SetFillColor(0);
					cLegend1->SetFillColor(0);
					cLegend0->SetLineColor(0);
					cLegend1->SetLineColor(0);

					option = "samese0p0";

					UInt_t cChannelId(0);
					double cTotalVCth0(0);
					double cTotalVCth0Error(0);
					double Nch(0);

					for( UInt_t i=0; i < cTestGroup->size(); i++ ){

						cChannelId = cTestGroup->at(i);
						CbcInfo::iterator cItChannel = cCbcInfo.find( cChannelId );
						if( cItChannel == cCbcInfo.end() ) continue;

						Channel *cChannel = cItChannel->second;
						TH1F *h = cChannel->Hist(); 
						h->Draw( option );
						TF1 *cFunc = (TF1 *) h->GetListOfFunctions()->Last();
						double cVCth0(0);
						double cVCthError(0);
						if( cFunc ) {
							cVCth0 = cFunc->GetParameter(0);

							cVCthError = cFunc->GetParError(0);
							cTotalVCth0 += cVCth0;
							cTotalVCth0Error += cVCth0 * cVCth0;
							Nch++;
						}
						else{
							cVCth0 = cChannel->VCth0();
						}

						UInt_t cOffset = cChannel->Offset();
						TString label( Form( "[%02X] %05.1f#pm%5.2f   %02X", cChannelId, cVCth0, cVCthError, cOffset )); 
						TLegend *cL = cChannelId %2 ? cLegend0 : cLegend1;
						cL->AddEntry( h, label, "p" ); 
					}
					TString cStringTotalVCth0;
					if( Nch ){
						cTotalVCth0 /= Nch;
						cTotalVCth0Error = sqrt( ( cTotalVCth0Error - Nch * cTotalVCth0*cTotalVCth0 ) / Nch );
						cStringTotalVCth0 = Form( "  VCth mid. point = %05.1f#pm%5.2f (0x%02X#pm%02X)", cTotalVCth0, cTotalVCth0Error, (Int_t)cTotalVCth0, (Int_t)cTotalVCth0Error );
					}
					UInt_t cVplus = fCbcRegMap->GetValue( cFeId, cCbcId, 0, 0x0B );
					TString cHtotalTitle =  Form( "FE(%d) CBC(%d) Group(%03X) Vplus = 0x%02X Target VCth = 0x%02X %s%s; VCth", 
							cFeId, cCbcId, cGroupId, cVplus, fTargetVCth, getScanType().Data(), cStringTotalVCth0.Data() );
					cHtotal->SetTitle( cHtotalTitle );

					cLegend0->Draw();
					cLegend1->Draw();
					cPad->Update();
				}
			}
		}
	}

	void ScurveAnalyser::PrintScurveHistogramDisplayPads(){

		UInt_t cGroupId(0);
		TestGroup *cTestGroup = fGroupMap->GetActivatedGroup(cGroupId);
		if( cTestGroup == 0 ) return;

		CalibrationResult::iterator cItFe = fResult.begin();
		for( ; cItFe != fResult.end(); cItFe++ ){

			UInt_t cFeId = cItFe->first;
			FeInfo &cFeInfo = cItFe->second;
			FeInfo::iterator cItCbc = cFeInfo.begin();
			for( ; cItCbc != cFeInfo.end(); cItCbc++ ){

				UInt_t cCbcId = cItCbc->first;
				CbcInfo &cCbcInfo = cItCbc->second;
				TPad *cPad = cCbcInfo.GetScurveHistogramDisplayPad( cGroupId );	
				if( cPad ){
					cPad->cd();
					TString cRegInfo;
					if( (fScanType == SINGLEVCTHSCAN) | (fScanType == DELAYSCAN) ){

						UInt_t cVplus = fCbcRegMap->GetValue( cFeId, cCbcId, 0, 0x0B );
						UInt_t cPage(0x00), cAddr(0x0F);
						UInt_t cTPEnable = fCbcRegMap->GetValue( cFeId, cCbcId, cPage, cAddr );
						UInt_t cMask = 1 << 6;
						cTPEnable &= cMask;
						UInt_t cTPPot = fCbcRegMap->GetValue( cFeId, cCbcId, 0, 0x0D );
						if (fScanType == SINGLEVCTHSCAN){
							cRegInfo = Form( "Vplus%02XTP%02XPot%02X", cVplus, cTPEnable, cTPPot );
						}
						else if (fScanType == DELAYSCAN) {

							UInt_t cDelay = fCbcRegMap->GetValue( cFeId, cCbcId, 0, 0x0E );
							cRegInfo = Form( "Vplus%02XTP%02XPot%02XDelay%02X", cVplus, cTPEnable, cTPPot, cDelay );

						}

					}
					cPad->Print( Form( "%s/%sFE%uCBC%uTestGroup%d%s.eps", fOutputDir.Data(), getScanId().Data(), cFeId, cCbcId, cGroupId, cRegInfo.Data() ) );
				}
			}
		}
	}
	UInt_t ScurveAnalyser::GetOffset( UInt_t pFeId, UInt_t pCbcId, UInt_t pChannelId ){ 
		const Channel *cC = fResult.GetChannel( pFeId, pCbcId, pChannelId ); 
#ifdef __CBCDAQ_DEV__
		std::cout << "ChannelTest [" << cC->ChannelId() << "] " << pChannelId << "  Offset = " << cC->Offset() << std::endl; 
#endif
		if( cC == 0 ) return 0;
		return cC->Offset();
	}
	UInt_t ScurveAnalyser::GetVplus( UInt_t pFeId, UInt_t pCbcId ){

		UInt_t cVplus(0);
		CbcInfo *cCbcInfo = fResult.getCbcInfo( pFeId, pCbcId );	
		if( cCbcInfo ){
			cVplus = cCbcInfo->Vplus();
		}	
		return cVplus;
	}
	void ScurveAnalyser::DumpResult(){

		for( UInt_t cFeId=0; cFeId < fNFe; cFeId++ ){

			for( UInt_t cCbcId=0; cCbcId< fNCbc; cCbcId++ ){

				CbcInfo *cCbcInfo = fResult.getCbcInfo( cFeId, cCbcId );	
				if( cCbcInfo == 0 ) return;

				printf( "FE: %d CBC: %d Vpluse: %u\n", cFeId, cCbcId, cCbcInfo->Vplus() );

				CbcInfo::iterator cIt = cCbcInfo->begin();
				for( ; cIt != cCbcInfo->end(); cIt++ ){
					Channel *cChannel = cIt->second;
					printf( "FE: %d, CBC: %d Channel: %03d Offset: %2X VCth0: %6.2f VCth0Error: %7.3f\n", 
							cFeId, cCbcId, cChannel->ChannelId(), cChannel->Offset(), cChannel->VCth0(), cChannel->VCth0Error() );
				}
			}
		}
	}
	void ScurveAnalyser::SetVplus(){

		for( UInt_t cFeId=0; cFeId < fNFe; cFeId++ ){

			for( UInt_t cCbcId=0; cCbcId< fNCbc; cCbcId++ ){
				UInt_t cVplus(0);
				Int_t cN(0);
				Float_t cSum(0);
				CbcInfo *cCbcInfo = fResult.getCbcInfo( cFeId, cCbcId );	
				std::map<UInt_t, TGraphErrors *> *cMap = cCbcInfo->GetGraphVplusVCth0();	
				std::map<UInt_t, TGraphErrors *>::iterator cIt = cMap->begin();
				for( ; cIt != cMap->end(); cIt++ ){
					TGraphErrors *cG = cIt->second; 
					//cG->Fit( "pol1", "Q0" );
					cG->Fit( "pol1", "Q", "", 1, 0xFF );
					cSum += cG->GetFunction( "pol1" )->Eval( fTargetVCth ); 
					cN++;
				}
				if( cN != 0 ) cVplus = (UInt_t) ( cSum / cN );
				cCbcInfo->SetVplus( cVplus );
			}
		}
	}

	void ScurveAnalyser::SetGain(){ //fb
		for( UInt_t cFeId=0; cFeId < fNFe; cFeId++ ){

			for( UInt_t cCbcId=0; cCbcId< fNCbc; cCbcId++ ){
				UInt_t cVplus(0);
				Int_t cN(0);
				Float_t cSum(0);
				CbcInfo *cCbcInfo = fResult.getCbcInfo( cFeId, cCbcId );
				std::map<UInt_t, TGraphErrors *> *cMap = cCbcInfo->GetGraphGain();
				std::map<UInt_t, TGraphErrors *>::iterator cIt = cMap->begin();
				for( ; cIt != cMap->end(); cIt++ ){
					//TGraphErrors *cG = cIt->second;
					//cG->Fit( "pol1", "Q0" );
					//cG->Fit( "pol1", "Q", "", 1, 0xFF );
					//cSum += cG->GetFunction( "pol1" )->Eval( fTargetVCth );
					cN++;
				}
				if( cN != 0 ) cVplus = (UInt_t) ( cSum / cN );
				cCbcInfo->SetVplus( cVplus );
			}
		}

	}

	void ScurveAnalyser::SetScurveHistogramDisplayPad( UInt_t pFeId, UInt_t pCbcId, TPad *pPad ){

		UInt_t cNpad = fGroupMap->size()+1;
		UInt_t cNcol = 3;
		UInt_t cNrow = cNpad/cNcol;
		if( cNpad%cNcol ) cNrow++; 
		pPad->Divide( cNcol, cNrow );

		UInt_t i(0);
		TestGroupMap::iterator cIt = fGroupMap->begin();
		for( ; cIt != fGroupMap->end(); cIt++ ){
			UInt_t cGroupId = cIt->first;
			fResult.SetScurveHistogramDisplayPad( pFeId, pCbcId, cGroupId, (TPad *)pPad->GetPad(++i) );
		}
		fResult.SetDummyPad( pFeId, pCbcId, (TPad *) pPad->GetPad(cNcol*cNrow) );
	}
	void ScurveAnalyser::SetVplusVsVCth0GraphDisplayPad( UInt_t pFeId, TPad *pPad ){

		CalibrationResult::const_iterator cItFe = fResult.find( pFeId );

		const FeInfo &cFeInfo = cItFe->second;
		UInt_t cNcbc = cFeInfo.size();
		if( cNcbc == 2 ){
			pPad->Divide( 2, 1 );
		}
		FeInfo::const_iterator cItCbc = cFeInfo.begin();
		UInt_t i(0);
		for( ; cItCbc != cFeInfo.end(); cItCbc++ ){
			UInt_t cCbcId = cItCbc->first;
			fResult.SetVplusVsVCth0GraphDisplayPad( pFeId, cCbcId, (TPad *)pPad->GetPad( ++i ) );
		}
	}

	void ScurveAnalyser::SetGainGraphDisplayPad( UInt_t pFeId, TPad *pPad ){

			CalibrationResult::const_iterator cItFe = fResult.find( pFeId );

			const FeInfo &cFeInfo = cItFe->second;
			UInt_t cNcbc = cFeInfo.size();
			if( cNcbc == 2 ){
				pPad->Divide( 2, 1 );
			}
			FeInfo::const_iterator cItCbc = cFeInfo.begin();
			UInt_t i(0);
			for( ; cItCbc != cFeInfo.end(); cItCbc++ ){
				UInt_t cCbcId = cItCbc->first;
				fResult.SetGainGraphDisplayPad( pFeId, cCbcId, (TPad *)pPad->GetPad( ++i ) );
			}
		}

	TH1F * ScurveAnalyser::createScurveHistogram( UInt_t pFeId, UInt_t pCbcId, UInt_t pChannelId ){

		unsigned int color= pChannelId/16%8 + 2;
		unsigned int marker = pChannelId % 2 + pChannelId/128 * 4 + 20;
		TString cName = Form( "h_%02u_%02u_%02u_%03u", fBeId, pFeId, pCbcId, pChannelId );
		//gROOT->Delete( cName + ";*" ); 
		TObject *cObj = gROOT->FindObject( cName ); 
		if( cObj ) delete cObj;
		TH1F *h = new TH1F(  cName, 
				Form( "BE(%02u) FE(%02u) CBC(%02u) Channel(%03u); VCth; Count", fBeId, pFeId, pCbcId, pChannelId ), 
				256, -0.5, 255.5 );
		//fHistList.push_back( h );
		h->Sumw2();
		h->SetMarkerColor( color );
		h->SetLineColor( color );
		h->SetMarkerStyle( marker );
		//std::cout << "Created histogram = " << h << std::endl;
		return h;	
	}
	void ScurveAnalyser::resetHistograms(){

		fHtotal->Reset();

		UInt_t cGroupId(0);
		TestGroup *cTestGroup = fGroupMap->GetActivatedGroup(cGroupId);
		if( cTestGroup == 0 ) return;

		const std::vector<Channel *> *cChannelList = cTestGroup->GetChannelList(); 
		for( UInt_t k=0; k < cChannelList->size(); k++ ){

			Channel *cChannel = cChannelList->at(k);
			cChannel->ResetHist();
		}
	}
	void ScurveAnalyser::updateOffsets(){
		for( UInt_t i=0; i < fChannelList.size(); i++ ){
			if( fChannelList[i] == 0 ) continue;
			fChannelList[i]->UpdateOffset();
		}
	}

	TString ScurveAnalyser::getScanType(){

		TString cType;
		if( fScanType == OFFSETCALIB ){

			TString cScanType;
			if( fCurrentTargetBit == 8 ) cScanType = "Initial offsets = 0x00";	
			else if( fCurrentTargetBit == -1 ) cScanType = "Result";
			else cScanType = Form( "Offset bit%u #rightarrow 1", fCurrentTargetBit );

			cType = Form( "OffsetCalib%s", cScanType.Data() ); 
		}
		else if( fScanType == VPLUSSEARCH ){

			UInt_t cVplus = fCbcRegMap->GetValue( 0, 0, 0, 0x0B );
			cType = Form( "Vplus = 0x%02X", cVplus );
		}
		else if( fScanType == SINGLEVCTHSCAN ){

			cType = Form( "Single Scan" );
		}
		else if (fScanType == DELAYSCAN) {
			UInt_t cDelay = fCbcRegMap->GetValue( 0, 0, 0, 0x0E );
			cType = Form( "Delay Scan %02X" , cDelay);
		}
		return cType;
	}
	TString ScurveAnalyser::getScanId(){

		TString cType;
		if( fScanType == OFFSETCALIB ){

			TString cScanType;
			if( fCurrentTargetBit == 8 ) cScanType = "Offsets00";	
			else if( fCurrentTargetBit == -1 ) cScanType = "Result";
			else cScanType = Form( "Bit%uIs1", fCurrentTargetBit );

			cType = Form( "OffsetCalib%s", cScanType.Data() ); 
		}
		else if( fScanType == VPLUSSEARCH ){

			UInt_t cVplus = fCbcRegMap->GetValue( 0, 0, 0, 0x0B );
			cType = Form( "Vplus%02X", cVplus );
		}
		else if( fScanType == SINGLEVCTHSCAN ){

			cType = Form( "SingleScan" );
		}

		else if (fScanType == DELAYSCAN) {
			UInt_t cDelay = fCbcRegMap->GetValue( 0, 0, 0, 0x0E );
			cType = Form( "DelayScan0x%02X", cDelay );
		}
		return cType;
	}
	void ScurveAnalyser::setNextOffsets(){

		if( fScanType != OFFSETCALIB ) return;	

		UInt_t cGroupId(0);
		TestGroup *cTestGroup = fGroupMap->GetActivatedGroup(cGroupId);
		if( cTestGroup == 0 ) return;

		UInt_t cFeId(0), cCbcId(0), cChannelId(0);

		const std::vector<Channel *> *cChannelList = cTestGroup->GetChannelList(); 
		for( UInt_t k=0; k < cChannelList->size(); k++ ){

			Channel *cChannel = cChannelList->at(k);
			if( cChannel == 0 ) continue;

			cChannel->Id( cFeId, cCbcId, cChannelId );
			//std::cout << "GroupId = " << cGroupId << ", Channel id = " << cChannelId << std::endl;
			TH1F *h = cChannel->Hist();
			if(h==0) continue;

			TF1 *cFunc = (TF1 *) h->GetListOfFunctions()->Last();	
			if( !cFunc ) continue;

			double cVCth0 = cFunc->GetParameter(0);
			UInt_t cOffset = cChannel->Offset();
			if( fTargetVCth < cVCth0 && fCurrentTargetBit != 8 ){
				cOffset &= ~( 1 << fCurrentTargetBit );
			}
			if( fCurrentTargetBit > 0 )cOffset |= 1 << ( fCurrentTargetBit - 1 );
			cChannel->SetNextOffset( cOffset );
			//if( cCbcId == 0 && cChannelId == 0 ){
			//	if( cOffset & 0x40000000 ){
			//		std::cout << cVCth0 << " " << fCurrentTargetBit << std::endl; 
			//	}
			//}
		}
	}
}
