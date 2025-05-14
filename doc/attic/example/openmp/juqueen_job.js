# This jobscript can be submitted to the juqueen 
# job queue via llsubmit [script]
# Do not forget to put your email adress in the 
# notify_user field.
#
# This script has to be executed within the folder of
# the executable ./sc_openmp
#
# Notice that bg_size=32 is smallest possible number of nodes.
# Any smaller input is automatically set to 32.

# @ job_name = sc_openmp
# @ comment = "Example libsc with openmp"
# @ error = $(job_name).$(jobid).out
# @ output = $(job_name).$(jobid).out
# @ environment = COPY_ALL
# @ wall_clock_limit = 00:30:00
# @ notification = error
# @ notify_user = yourname@yourserver.com
# @ job_type = bluegene
# @ bg_size = 32
# @ queue

export OMP_NUM_THREADS=4

runjob --ranks-per-node 16 --exp-env OMP_NUM_THREADS : ./sc_openmp
