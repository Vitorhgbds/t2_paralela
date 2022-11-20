generate:
	mpicc sequencial.c -o seq -fopenmp
	mpicc paralel.c -o paral -fopenmp
	mpicc paralEx.c -i paralEx -fopenmp

clean:
	rm ./seq
	rm ./paral
	rm ./paralEx
