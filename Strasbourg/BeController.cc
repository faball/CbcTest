#include <uhal/uhal.hpp>
#include <boost/format.hpp>
#include <boost/date_time.hpp>
#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include "BeController.h"
#include "Data.h"
#include "utils/Utilities.h"
#include "utils/Exception.h"

using namespace CbcDaq;

#define I2C_CTRL_ENABLE 	0x000009F4
#define I2C_CTRL_DISABLE	0
#define I2C_STROBE			1
#define I2C_M16B			0
#define I2C_MEM				1
#define I2C_WRITE_ADDR		0x09
#define I2C_READ_ADDR		0x06

#define I2C_SLAVE			0x5B
#define I2C_COMMAND			"user_wb_ttc_fmc_regs.cbc_reg_i2c_command"
#define I2C_REPLY			"user_wb_ttc_fmc_regs.cbc_reg_i2c_reply"

#define MAX_NB_LOOP			100

namespace Strasbourg{


	const unsigned int BeController::fPacketSize = Strasbourg::Event::EVENT_SIZE_32;//single event data size in 32 bits words
	const std::string BeController::fStrI2cSettings = "user_wb_ttc_fmc_regs.cbc_reg_i2c_settings";
	const std::string BeController::fStrI2cCommand = I2C_COMMAND;
	const std::string BeController::fStrI2cReply = I2C_REPLY;
	const uint32_t BeController::fI2cSlave = I2C_SLAVE;
	const unsigned int BeController::fI2cSramSize = 128; 

	void BeController::DecodeRegInfo( uint32_t vecReq, uint32_t &uCBC, uint32_t &uPage, uint32_t &uAddr, uint32_t &uWrite){
		uint32_t cMask(0x00000000);
		unsigned int i(0);
		for( i=17; i < 18; i++ ) cMask |= ( (uint32_t) 1 << i );
		uCBC = ( ( vecReq & cMask ) >> 17 );
		for( cMask=0, i=16; i < 17; i++ ) cMask |= (uint32_t) 1 << i;
		uPage = ( vecReq & cMask ) >> 16;
		for( cMask=0, i=8; i < 16; i++ ) cMask |= (uint32_t) 1 << i;
		uAddr = ( vecReq & cMask ) >> 8;
		for( cMask=0, i=0; i < 8; i++ ) cMask |= (uint32_t) 1 << i;
		uWrite = vecReq & cMask;
	}
	void BeController::AddCbcReg(std::vector<uint32_t>& pVecReq, uint32_t uCbc, uint32_t uPage, uint32_t uAddr, uint32_t uWrite){
		pVecReq.push_back(uCbc<<17 | uPage<<16 | uAddr<<8 | uWrite);
	}

	BeController::BeController(): 
		HwController( "BeController" ),
		fBoard(0), 
		fDevFlag(DEV0){


		}
	BeController::~BeController(){
		delete fBoard;
	}
	void BeController::ConfigureGlib( const char *pUhalConfigFileName, const char *pBoardId ){

#ifdef __CBCDAQ_DEV__
		uhal::setLogLevelTo( uhal::Error() );
#else
		uhal::disableLogging();
#endif
		uhal::ConnectionManager cm( pUhalConfigFileName );
		delete fBoard;
		fBoard = new uhal::HwInterface( cm.getDevice( pBoardId ) );

		boost::posix_time::milliseconds cPause(200);

		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands.PC_config_ok").write(1);
		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands.SRAM1_end_readout").write(0);
		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands.SRAM2_end_readout").write(0);
		fBoard->getNode("ctrl_sram.sram1_user_logic").write(1);//#sram1 user
		fBoard->getNode("ctrl_sram.sram2_user_logic").write(1);//#sram2 user
		fBoard->dispatch();
		boost::this_thread::sleep(cPause);

		GlibSetting::iterator cIt = fGlibSetting.begin();
		for(; cIt != fGlibSetting.end(); cIt++ ){
                        //std::cout << cIt->first << " " << cIt->second << std::endl;
			fBoard->getNode( cIt->first ).write( (uint32_t) cIt->second );
		}
		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands.SPURIOUS_FRAME").write(0);
		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands2.force_BG0_start").write(0);
		fBoard->getNode("user_wb_ttc_fmc_regs.cbc_acquisition.CBC_TRIGGER_ONE_SHOT").write( 0 );
		fBoard->getNode("break_trigger").write(1);
		fBoard->dispatch();

		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands.PC_config_ok").write(0);
		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands.SRAM1_end_readout").write(0);
		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands.SRAM2_end_readout").write(0);
		fBoard->getNode("ctrl_sram.sram1_user_logic").write(1);//#sram1 user
		fBoard->getNode("ctrl_sram.sram2_user_logic").write(1);//#sram2 user
		fBoard->dispatch();
		boost::this_thread::sleep(cPause);

		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands.SPURIOUS_FRAME").write( 0 );
		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands2.force_BG0_start").write(0);
		fBoard->getNode("user_wb_ttc_fmc_regs.cbc_acquisition.CBC_TRIGGER_ONE_SHOT").write( 0 );
		fBoard->getNode("break_trigger").write(1);
		fBoard->dispatch();

		boost::this_thread::sleep( cPause * 3 );	
	}
	void BeController::StartAcquisition(){

#ifdef __CBCDAQ_DEV__
		if( fDevFlag == DEV0 ){
			long mtime = getTimeTook( fStartVeto, 1 );
			std::cout << "Time took for the trigger veto to trigger enable: " << std::dec << mtime << " ms." << std::endl;
		}
#endif

		fBoard->getNode("break_trigger").write(0);
		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands.PC_config_ok").write(1);
		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands2.force_BG0_start").write(1);
		fBoard->dispatch();
	}
	void BeController::ReadDataInSRAM( unsigned int pNthAcq, bool pBreakTrigger ){

		//std::cout << "ReadDataInSRAM" << std::endl;

#ifdef __CBCDAQ_DEV__
		struct timeval cStartReadDataInSRAM, cStartBlockRead;
		struct timeval start;
		long mtime;

		if( fDevFlag == DEV0 ){
			gettimeofday( &cStartReadDataInSRAM, 0 );
			gettimeofday(&start, 0);
		}
#endif
		//Readout settings
		boost::posix_time::milliseconds cWait(1);

		uhal::ValWord<uint32_t> cVal;
		uint32_t cNPackets= fNeventPerAcq+1;
		uint32_t cBlockSize = cNPackets * fPacketSize;

		//Wait for start acknowledge
		do{
			cVal=fBoard->getNode("user_wb_ttc_fmc_regs.status_flags.CMD_START_VALID").read();
			fBoard->dispatch();
			if (cVal==0)
				boost::this_thread::sleep(cWait);
		} while ( cVal==0 );
#ifdef __CBCDAQ_DEV__
		if( fDevFlag == DEV0 ){
			mtime = getTimeTook( start, 1 ); 
			std::cout << "BeController::ReadDataInSRAM()  Time took for the CMD_START_VALID flag to be set: " << std::dec << mtime << " ms." << std::endl;
		}
#endif
		//FIFO goes to write_data state
		//Select SRAM
		SelectSramForDAQ( pNthAcq );

#ifdef __CBCDAQ_DEV__
		if( fDevFlag == DEV0 ){
			gettimeofday(&start, 0);
		}
#endif
		//Wait for the SRAM full condition.
		cVal = fBoard->getNode(fStrFull).read();
		do{
			boost::this_thread::sleep( cWait );	
			cVal = fBoard->getNode(fStrFull).read();
			fBoard->dispatch();
		} while (cVal==0);
#ifdef __CBCDAQ_DEV__
		if( fDevFlag == DEV0 ){
			mtime = getTimeTook( start, 1 ); 
			std::cout << "Time took for the data to be ready : " << std::dec << mtime << " ms." << std::endl;
		}
#endif
		//break trigger
		if( pBreakTrigger ){
			fBoard->getNode("break_trigger").write(1);
			fBoard->dispatch();
		}
		// JRF end
#ifdef __CBCDAQ_DEV__
		if( fDevFlag == DEV0 ){
			gettimeofday( &fStartVeto, 0 );
		}
#endif
		//Set read mode to SRAM
		fBoard->getNode(fStrSramUserLogic).write(0);
		fBoard->dispatch();
		//      boost::this_thread::sleep(wait);
#ifdef __CBCDAQ_DEV__
		if( fDevFlag == DEV0 ){
			gettimeofday( &cStartBlockRead, 0 );
		}
#endif
		//Read SRAM
                //std::cout << "BlockSize = " << std::dec << cBlockSize << std::endl;
                //Block size = 42 * ( packet # + 1 )
		uhal::ValVector<uint32_t> cData = fBoard->getNode(fStrSram).readBlock(cBlockSize);
#ifdef __CBCDAQ_DEV__
		if( fDevFlag == DEV0 ){
			mtime = getTimeTook( cStartBlockRead, 1 );
			std::cout << "Time took for block read: " << std::dec << mtime << " ms." << std::endl;
		}
#endif

		fBoard->getNode(fStrSramUserLogic).write(1);
		fBoard->dispatch();//Mandatory or else sramX_full remains to 1
		fBoard->getNode(fStrReadout).write(1);
		fBoard->dispatch();
                if( cData.size() > 255 ) std::cout << "Reading Data readBlock() 256th value = " << std::hex << cData.at(255) << std::endl;

#ifdef __CBCDAQ_DEV__
		if( fDevFlag == DEV0 ){
			gettimeofday(&start, 0);
		}
#endif
		//Wait for the non SRAM full condition starts,
		do {
			cVal = fBoard->getNode(fStrFull).read();
			fBoard->dispatch();
			if (cVal==1)
				boost::this_thread::sleep(cWait);
		} while (cVal==1);
		//Wait for the non SRAM full condition ends. 
#ifdef __CBCDAQ_DEV__
		if( fDevFlag == DEV0 ){
			mtime = getTimeTook( start, 0 );
			std::cout << "Time took to the full flag to be 0 : " << std::dec << mtime << " us." << std::endl;
		}
#endif

		fBoard->getNode(fStrReadout).write(0);// write readout 0
		fBoard->dispatch();

#ifdef __CBCDAQ_DEV__
		if( fDevFlag == DEV0 ){
			mtime = getTimeTook( cStartReadDataInSRAM, 1 );
			std::cout << "Time took for ReadDataInSRAM: " << std::dec << mtime << " ms." << std::endl;
		}
#endif
		/*
		   for( unsigned int i=0; i<cData.size(); i++ ){
		   char cSwapped[4];
		   swapByteOrder( (const char *)&cData[i], cSwapped, 4 );
		   for( int j=0; j < 4; j++ ){
		   cBuf[i*4+j] = cSwapped[j];
		   }
		   }
		 */

		fData->Set( &cData );
		return;
	}
	void BeController::EndAcquisition( unsigned int pNthAcq ){

		SelectSramForDAQ( pNthAcq );

		boost::posix_time::milliseconds cWait(1);

		uhal::ValWord<uint32_t> cVal;

		fBoard->getNode("break_trigger").write(1);
		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands.PC_config_ok").write(0);
		fBoard->getNode("user_wb_ttc_fmc_regs.pc_commands2.force_BG0_start").write(0);
		fBoard->dispatch();
		do {
			cVal = fBoard->getNode(fStrFull).read();//read full flag until 0
			fBoard->dispatch();
			if (cVal==1) boost::this_thread::sleep(cWait);
		} while (cVal==1);

		fBoard->getNode(fStrReadout).write(0);// write readout 0
		fBoard->dispatch();
		fNTotalAcq++;
	}
	void BeController::WriteAndReadbackCbcRegValues( uint16_t pFe, std::vector<uint32_t>& pVecReq ){
                
		boost::posix_time::milliseconds cPause(200);

		unsigned int cSize( pVecReq.size() );
		if( cSize > fI2cSramSize ){
			std::vector<uint32_t>::iterator cIt = pVecReq.begin();
			unsigned int i(0);
			//uint32_t c,p,a,w;
			while( i < cSize ){
				unsigned int j = i + fI2cSramSize < cSize ? i + fI2cSramSize : cSize;
				std::vector<uint32_t> cVecReq ( cIt + i, cIt + j );
		//		std::cout << i << "\t" << j << "\t" << cVecReq.size() << std::endl;
		//		std::vector<uint32_t>::iterator cItRead0 = cVecReq.begin();
		//		for(; cItRead0 != cVecReq.end(); cItRead0++ ){
		//			DecodeRegInfo( *cItRead0, c, p, a, w );
		//	//		if( p == 0 )std::cout << std::hex << c << " " << p << " " << a << " " << w << std::endl; 
		//		}
				try{
					WriteCbcRegValues( pFe, cVecReq );
					ReadCbcRegValues( pFe, cVecReq );
//					boost::this_thread::sleep(cPause);
				}
				catch( Exception &e ){
					throw e;
				}
				std::vector<uint32_t>::iterator cItOrg = cIt + i;
				std::vector<uint32_t>::iterator cItRead = cVecReq.begin();
				for(; cItRead != cVecReq.end(); cItRead++ ){
					*cItOrg = *cItRead;
					//			DecodeRegInfo( *cItRead, c, p, a, w );
					//	//		if( p == 0 )std::cout << std::hex << c << " " << p << " " << a << " " << w << std::endl; 
					cItOrg++;
				}
				i+=fI2cSramSize;
			}
		}
		else{
			try{
				WriteCbcRegValues( pFe, pVecReq );
				ReadCbcRegValues( pFe, pVecReq );
			}
			catch( Exception &e ){
				throw e;
			}
		}
	}
	void BeController::CbcHardReset(){

		fBoard->getNode( "cbc_hard_reset" ).write(1);
		fBoard->dispatch();
		usleep(200000);
		fBoard->getNode( "cbc_hard_reset" ).write(0);
		fBoard->dispatch();
		usleep(200000);
	}
	void BeController::CbcFastReset(){

		fBoard->getNode( "cbc_fast_reset" ).write(1);
		fBoard->dispatch();
		usleep(200000);
		fBoard->getNode( "cbc_fast_reset" ).write(0);
		fBoard->dispatch();
	}

	void BeController::SelectSramForI2C( uint32_t pFe ){

		fStrSram= (pFe==0 ? "sram1" : "sram2");
		fStrOtherSram= (pFe==0 ? "sram2" : "sram1");
		fStrSramUserLogic= (pFe==0 ? "ctrl_sram.sram1_user_logic" : "ctrl_sram.sram2_user_logic");
	}

	void BeController::SelectSramForDAQ( uint32_t pNthAcq ){

		fStrSram  = (boost::format("sram%d") % (pNthAcq%2+1)).str();
		fStrSramUserLogic =  (boost::format("ctrl_sram.sram%d_user_logic") % (pNthAcq%2+1)).str();
		fStrFull = (boost::format("user_wb_ttc_fmc_regs.flags.SRAM%d_full") % (pNthAcq%2+1)).str();
		fStrReadout= (boost::format("user_wb_ttc_fmc_regs.pc_commands.SRAM%d_end_readout") % (pNthAcq%2+1)).str();
	}

	bool BeController::I2cCmdAckWait( uint32_t pAckVal, unsigned int pNcount ){

		unsigned int cWait(100);
		if( pAckVal ) {
			cWait = pNcount * 600;
		}

		usleep( cWait );

		uhal::ValWord<uint32_t> cVal;
		uint32_t cLoop=0;
		do{
			cVal=fBoard->getNode("cbc_i2c_cmd_ack").read();
			fBoard->dispatch();

			if (cVal!=pAckVal){
				//std::cout << "Waiting for the I2c command acknowledge to be " << pAckVal << " for " << pNcount << " registers." << std::endl;
				//sleep(5);
				usleep( cWait );
			}	
		} while (cVal!= pAckVal && ++cLoop<MAX_NB_LOOP);

		if (cLoop>=MAX_NB_LOOP) {
			std::cout<<"Warning: time out in I2C acknowledge loop (" << pAckVal << ")" << std::endl;
			return false;
		}
		return true;
	}
	void BeController::SendBlockCbcI2cRequest( uint32_t pFe, std::vector<uint32_t>& pVecReq, bool pWrite){

		pVecReq.push_back(0xFFFFFFFF);

		SelectSramForI2C( pFe );

		fBoard->getNode(fStrSramUserLogic).write(1);
		fBoard->dispatch();

		fBoard->getNode(fStrSramUserLogic).write(0);
		fBoard->dispatch();

		fBoard->getNode(fStrSram).writeBlock(pVecReq);
		fBoard->getNode(fStrOtherSram).write(0xFFFFFFFF);		
		fBoard->dispatch();		

		fBoard->getNode(fStrSramUserLogic).write(1);
		fBoard->dispatch();

		fBoard->getNode("cbc_i2c_cmd_rq").write(pWrite ? 3: 1);//read or write request
		fBoard->dispatch();

		pVecReq.pop_back();

		if( I2cCmdAckWait( (uint32_t)1, pVecReq.size() ) ==0 ){
			throw Exception( Form( "%s: I2cCmdAckWait %d failed.", "BeController", 1 ) );
		}
		fBoard->getNode("cbc_i2c_cmd_rq").write(0);
		fBoard->dispatch();
		if( I2cCmdAckWait( (uint32_t)0, pVecReq.size() ) ==0 ){
			throw Exception( Form( "%s: I2cCmdAckWait %d failed.", "BeController", 0 ) );
		}
	}
	void BeController::WriteCbcRegValues( uint16_t pFe, std::vector<uint32_t>& pVecReq ){

#ifdef __CBCDAQ_DEV__
		static long min(0), sec(0);	
		struct timeval start0, end;
		long seconds(0), useconds(0);

		if( fDevFlag == DEV0 ){
			gettimeofday(&start0, 0);
		}
#endif
		try{	
			SendBlockCbcI2cRequest( pFe, pVecReq, true );
		}
		catch( Exception &e ){
			throw e;
		}
#ifdef __CBCDAQ_DEV__
		if( fDevFlag == DEV0 ){
			gettimeofday( &end, 0 );
			seconds = end.tv_sec - start0.tv_sec;
			useconds = end.tv_usec - start0.tv_usec;
			min += ( seconds + useconds / 1000000 ) /60;
			sec += ( seconds + useconds / 1000000 ) %60;
			std::cout << "Time took for Cbc register write so far = " << min << " min " << sec << " sec." << std::endl;
		}
#endif
	}
	void BeController::ReadCbcRegValues( uint16_t pFe, std::vector<uint32_t>& pVecReq ){

#ifdef __CBCDAQ_DEV__
		static long min(0), sec(0);	
		struct timeval start0, end;
		long seconds(0), useconds(0);

		if( fDevFlag == DEV0 ){
			gettimeofday(&start0, 0);
		}
#endif
		try{	
			SendBlockCbcI2cRequest( pFe, pVecReq, false);
		}
		catch( Exception &e ){
			throw e;
		}
#ifdef __CBCDAQ_DEV__
		if( fDevFlag == DEV0 ){
			gettimeofday( &end, 0 );
			seconds = end.tv_sec - start0.tv_sec;
			useconds = end.tv_usec - start0.tv_usec;
			min += ( seconds + useconds / 1000000 ) /60;
			sec += ( seconds + useconds / 1000000 ) %60;
			std::cout << "Time took for Cbc register read so far = " << min << " min " << sec << " sec." << std::endl;
		}
#endif
		ReadI2cValuesInSRAM( pFe, pVecReq );
	}
	int BeController::ReadI2cValuesInSRAM( unsigned int pFe, std::vector<uint32_t> &pVecReq ){

		SelectSramForI2C( pFe );

		fBoard->getNode(fStrSramUserLogic).write(0);
		fBoard->dispatch();//usleep(500000);
		//std::cout << "ReadI2cValuesInSRAM vector size = " << pVecReq.size() << std::endl;
		//for( unsigned int i=0; i < 1; i++ ){
		//	std::cout << pVecReq[i] << std::endl;
		//}
		uhal::ValVector<uint32_t> cData = fBoard->getNode(fStrSram).readBlock( pVecReq.size() );
		fBoard->dispatch();
                if( cData.size() > 255 ) std::cout << "Reading I2C readBlock() 256th value = " << std::hex << cData.at(255) << std::endl;
		fBoard->getNode(fStrSramUserLogic).write(1);
		fBoard->getNode("cbc_i2c_cmd_rq").write(0);
		fBoard->dispatch();

		std::vector<uint32_t>::iterator it = pVecReq.begin();
		uhal::ValVector< uint32_t >::const_iterator itValue = cData.begin();
		while( it != pVecReq.end() ){
			*it = *itValue;
			it++; itValue++;
		}
		return 0;
	}
	bool BeController::CbcI2cWrite3Values(){
		//"user_wb_ttc_fmc_regs.i2c_settings","user_wb_ttc_fmc_regs.i2c_command", "user_wb_ttc_fmc_regs.i2c_reply", 0x40);
		EnableI2c( true );
		bool cSuccess = SendI2c( 0x08, 0xC2, true );
		if (cSuccess){
			cSuccess = SendI2c( 0x09, 0, true );
		}		
		if (cSuccess){
			cSuccess = SendI2c( 0x11, 0, true );
		}		
		EnableI2c( false );
		return cSuccess;	
	}
	void BeController::EnableI2c( bool pEnable ){

		uint32_t cValue = I2C_CTRL_ENABLE;
		if( !pEnable ) cValue = I2C_CTRL_DISABLE;

		fBoard->getNode(fStrI2cSettings).write( cValue );
		fBoard->dispatch();
		if( pEnable ) usleep(100000);
	}
	uint32_t BeController::SendI2c( uint16_t pAddr, uint16_t pData, bool pWrite ){

		uint32_t fmc_vtrx_comm = I2C_STROBE<<31 | I2C_M16B<<25 | I2C_MEM<<24 | (pWrite?1:0)<<23 | fI2cSlave<<16 | pAddr<<8 | pData; 
		fBoard->getNode(fStrI2cCommand).write(fmc_vtrx_comm);
		fBoard->dispatch();

		uhal::ValWord<uint32_t> reply = fBoard->getNode(fStrI2cReply).read();
		fBoard->dispatch();

		return reply & 0xff;	
	}
	void BeController::decodeRegInfo( uint32_t pVecReq, uint32_t &pCbcId, uint32_t &pPage, uint32_t &pAddr, uint32_t &pWrite){

		BeController::DecodeRegInfo( pVecReq, pCbcId, pPage, pAddr, pWrite);
	}
	void BeController::addCbcReg(std::vector<uint32_t>& pVecReq, uint32_t pCbcId, uint32_t pPage, uint32_t pAddr, uint32_t pWrite){

		BeController::AddCbcReg( pVecReq, pCbcId, pPage, pAddr, pWrite);
	}
}

