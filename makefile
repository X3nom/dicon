
all:
	cd ./diconlib && make static
	cd ./node && make
	cd ./node-manager && make

clean:
	cd ./diconlib && make clean
	cd ./node && make clean
	cd ./node-manager && make clean


full: clean all