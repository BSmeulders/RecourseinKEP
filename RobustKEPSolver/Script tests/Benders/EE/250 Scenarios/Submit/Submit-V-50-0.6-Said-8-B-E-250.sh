#!/bin/bash

#SBATCH --job-name=8-0.6BE
#SBATCH --time=03:00:00
#
#SBATCH --output=Out-V-B-EE-50-0.6-3-1-250-Said-8.txt
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --mem-per-cpu=8192
#SBATCH --partition=Def
#
#SBATCH --mail-user=bart.smeulders@ulg.ac
#SBATCH --mail-type=ALL

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
./RobustKEP Config-V-50-0.6-Said-8-B-E-250.txt