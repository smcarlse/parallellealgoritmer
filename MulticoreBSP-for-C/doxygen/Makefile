
all: doxy.conf man.conf
	doxygen doxy.conf
	doxygen man.conf

../include:
	(cd ..;make library)

clean:
	rm -rf latex html man

