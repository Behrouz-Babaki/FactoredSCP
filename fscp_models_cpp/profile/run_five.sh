#!/bin/bash

dirname="d$(date +"%H%M")"
mkdir $dirname
cd $dirname
git rev-parse --verify HEAD > ./githash.txt

bindir="/cw/dtailocal/behrouz/repos/stochcp_factor/fscp_models_cpp/bin/"
kndir="/cw/dtailocal/behrouz/repos/stochcp_factor/instance_generator/data/knapsack/"

echo "running five instances"
for NUM in 74 102 60 88;
do
    cmd="${bindir}/run_knapsack --acfile ${kndir}/instance_${NUM}/knapsack.net.ac --lmfile ${kndir}/instance_${NUM}/knapsack.net.lmap --capacity 125 --depth_and 5 --depth_or 5)"
    $cmd >> out.txt
done

grep time out.txt | awk '{s+=$4} END {print s}' > time.txt

echo "running valgrind"
cmd="valgrind --tool=callgrind ${bindir}/run_knapsack --acfile ${kndir}/instance_74/knapsack.net.ac --lmfile ${kndir}/instance_74/knapsack.net.lmap --capacity 125 --depth_and 5 --depth_or 5"
$cmd    

