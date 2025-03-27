

clean:
	cd ./diconlib && make clean
	cd ./node && make clean


full: clean
	cd ./diconlib && make static
	cd ./node && make normal
