CbcTest
=======

code to test CBC2

Tested in the beamtest virtual machine for two versions with single GLIB, single old FMC, one and two CBC2 chips.


Known problem
-------
ipbus cannot deal with a big data to read at once with readBlock() function in VM v1.1.0
One solution to this is to reduce the # of events to be read at once with
GlibReg_user_wb_ttc_fmc_regs.pc_commands.CBC_DATA_PACKET_NUMBER 

As small as 10 works.

