#!/bin/bash

#SBATCH --job-name=9-0.6BD
#SBATCH --time=03:00:00
#
#SBATCH --output=Out-V-B-Dickerson-50-0.6-3-1-50-Said-9.txt
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=4
#SBATCH --mem-per-cpu=8192
#SBATCH --partition=Def
#
#SBATCH --mail-user=bart.smeulders@ulg.ac
#SBATCH --mail-type=ALL

export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
./RobustKEP Config-V-50-0.6-Said-9-B-D-50.txt