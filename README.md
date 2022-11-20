# t2_paralela


To run, execute make
and it will generate the files `seq` and `paral`

To execute on batch.
sbatch ${batchFile}


CASO NÂO RODE.
NO batch file, troque o 
#SBATCH -o %x.%j.out
para
#SBATCH -o /home/pp03027/exit-%x.%j.out