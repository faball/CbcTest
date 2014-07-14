all: utils Cbc Strasbourg ICCalib CbcDaq src doc

utils::
	+(cd $@; make)
Cbc::
	+(cd $@; make)
Strasbourg::
	+(cd $@; make)
ICCalib::
	+(cd $@; make)
CbcDaq::
	+(cd $@; make)
src::
	+(cd $@; make)
#doc::
#	+(cd $@; make)

		
clean:
	(cd utils; make clean)
	(cd Cbc; make clean)
	(cd Strasbourg; make clean)
	(cd ICCalib; make clean)
	(cd CbcDaq; make clean)
	(cd doc; make clean)
	(rm -f lib/* bin/*)	
