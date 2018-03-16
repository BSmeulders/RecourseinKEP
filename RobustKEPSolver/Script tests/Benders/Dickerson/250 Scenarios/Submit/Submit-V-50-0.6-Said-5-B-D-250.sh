#!/bin/bash

#SBATCH --job-name=5-0.6BD
#SBATCH --time=03:00:00
#
#SBATCH --output=Out-V-B-Dickerson-50-0.6-3-1-250-Said-5.txt
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --mem-per-cpu=8192
#SBATCH --partition=Def
#
#SBATCH --mail-user=bart.smeulders@ulg.ac
#SBATCH --mail-type=ALL

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
./RobustKEP Config-V-50-0.6-Said-5-B-D-250.txt