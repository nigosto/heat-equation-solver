#!/bin/bash

#SBATCH --job-name=heat_equation
#SBATCH --output=heat_equation.out
#SBATCH --error=heat_equation.err
#SBATCH --partition=short
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=8
#SBATCH --cpus-per-task=4
#SBATCH --hint=nomultithread

module add unite/nvhpc/25.9

make clean && make run NP=16 ARGS="opposite_heat_sources_input.pgm output.pgm"