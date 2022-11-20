generate:
	mpicc sequencial.c -o seq -fopenmp
	mpicc paralel.c -o paral -fopenmp

clean:
	rm ./seq
	rm ./paral
