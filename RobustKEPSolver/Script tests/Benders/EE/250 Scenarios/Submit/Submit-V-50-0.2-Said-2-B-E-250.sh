#!/bin/bash

#SBATCH --job-name=2-0.2BE
#SBATCH --time=03:00:00
#
#SBATCH --output=Out-V-B-EE-50-0.2-3-1-250-Said-2.txt
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --mem-per-cpu=8192
#SBATCH --partition=Def
#
#SBATCH --mail-user=bart.smeulders@ulg.ac
#SBATCH --mail-type=ALL

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
./RobustKEP Config-V-50-0.2-Said-2-B-E-250.txt