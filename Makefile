generate:
	mkdir ./exit/
	mpicc sequencial.c -o seq -fopenmp
	mpicc paralel.c -o paral -fopenmp

clean:
	rm -rf ./exit/
	rm ./seq
	rm ./paral
