generate:
	mpicc sequencial.c -o seq -fopenmp
	mpicc paralel.c -o paral -fopenmp
	mpicc paralelEx.c -o paralEx -fopenmp

clean:
	rm ./seq
	rm ./paral
	rm ./paralEx
