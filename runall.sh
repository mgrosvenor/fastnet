#! /bin/bash

./fast_net_baseline_expr.py  
rm -rf bin

./fast_net_nocopy_expr.py  
rm -rf bin

./fast_net_prefetch.py
rm -rf bin

./fast_net_layout.py
rm -rf bin

./fast_net_hbo.py
rm -rf bin

./fast_net_4k.py
rm -rf bin

./fast_net_8k.py
rm -rf bin

./fast_net_scale.py

time ./plot_all.sh

time tar -cjvf experiments.tar.bz2 experiments/

