#include <iostream>

void PlotGain(UInt_t pCbc = 0, UInt_t color = 4 ){ //Char_t *fname , Char_t *hname0, Char_t *option,

	/*TH1F *h0 = new TH1F( hname0, ";V_{CTH} middle point pedestal subtracted;count", 180, -80, 0 );
	TFile *fin = new TFile( fname );
	TFile *fped = new TFile("/home/xtaldaq/cbc/CbcTest/results/test/VCthScanCalibrationResult.root"  );
	for( UInt_t i=0; i<254; i++ ){
		TString hname = Form( "h_00_%02d_%03d", pCbc, i );
		TH1F *h = (TH1F *) fin->Get( hname );
		TF1 *func = h->GetFunction( Form("f_%s", hname.Data() ) );
		h = (TH1F *) fped->Get( hname );
		TF1 *func0 = h->GetFunction( Form("f_%s", hname.Data() ) );
		h0->Fill( func->GetParameter(0) - func0->GetParameter(0) );
	}
	h0->Draw( option );*/

	//TCanvas *c1 = new TCanvas("c1","c1",400,100,520,400);
	//TCanvas *c2 = new TCanvas("c2","c2",400,100,520,400);

	TObjString *os=0;
	TH1F *h0 = new TH1F( "A Test Graph", ";V_{CTH};count", 50, 100, 150 );
	TFile *fin = new TFile( "/home/xtaldaq/cbc/CbcTest/results/test/UnCalibSingleScan.root" );
	TFile *fped = new TFile("/home/xtaldaq/cbc/CbcTest/results/test/CalibSingleScan.root"  );

	int count = 0;
	int vcth [256];



	for( UInt_t i=0; i<254; i++ ){
		//TString hname = Form( "h_00_%02d_%03d", pCbc, i );
		TString hname = Form( "h_%02d_%02d_%02d_%03d", 0, 0, pCbc, i );
		TH1F *h = (TH1F *) fin->Get( hname );
		TF1 *func = h->GetFunction( Form("f_%s", hname.Data() ) );
		h = (TH1F *) fped->Get( hname );
		TF1 *func0 = h->GetFunction( Form("f_%s", hname.Data() ) );
		h0->Fill( func0->GetParameter(0) );

		if((h->GetBinContent(i) > 0) && (h->GetBinContent(i) < 1)) {
			vcth[count] = i;
			++count;
		}




	}
	std::cout << "Total	" << count << "	" << std::endl;
	std::cout << "Middle Value	" << vcth[count/2] << "	" << std::endl;

	//h0->Draw( );
	//c1->cd();
	//h->Draw();
	//c2->cd();
	//h->Draw();
}
