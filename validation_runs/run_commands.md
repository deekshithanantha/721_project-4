\# Validation Runs



\---



\## PERFECT VP  

\*\*Baseline IPC = 2.59\*\*



| Run ID | Benchmark | Command | Output |

|--------|-----------|---------|--------|

| VAL-1 | 473.astar\_biglakes\_ref.1844.0.50.gz | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-perf=1 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-1 |



\---



\## SVP + ORACLE CONFIDENCE + ROLLBACK-FREE  

Perfect BP, Oracle MDP, Oracle VP confidence, no exceptions.  

\*\*Baseline IPC = 3.18\*\*



| Run ID | Benchmark | Command | Output |

|--------|-----------|---------|--------|

| VAL-2 | 473.astar\_biglakes\_ref.1844.0.50.gz | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=300,1,10,10,31 --mdp=4,0 --perf=1,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-2 |

| VAL-3 | 473.astar\_biglakes\_ref.1844.0.50.gz | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,0 --vp-svp=300,1,10,10,31 --mdp=4,0 --perf=1,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-3 |

| VAL-4 | 473.astar\_biglakes\_ref.1844.0.50.gz | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=0,0,1 --vp-svp=300,1,10,10,31 --mdp=4,0 --perf=1,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-4 |



\---



\## REAL EVERYTHING  

Real BP, real MDP, real SVP confidence.  

\*\*Baseline IPC = 2.59\*\*



| Run ID | Benchmark | Command | Output |

|--------|-----------|---------|--------|

| VAL-5 | 473.astar\_biglakes\_ref.1844.0.50.gz | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=300,0,10,10,31 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-5 |

| VAL-6 | 473.astar\_biglakes\_ref.1844.0.50.gz | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=100,0,10,10,31 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-6 |



\---



\## SVP PARAMETER SWEEP  

\*\*Baseline IPC = 2.59\*\*



| Run ID | Benchmark | Command | Output |

|--------|-----------|---------|--------|

| VAL-7 | 473.astar\_biglakes\_ref.1844.0.50.gz | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=300,0,7,10,31 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-7 |

| VAL-8 | same | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=300,0,7,0,31 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-8 |

| VAL-9 | same | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=300,0,8,0,31 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-9 |

| VAL-10 | same | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=300,0,8,0,15 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-10 |

| VAL-11 | same | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=300,0,8,0,7 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-11 |



\---



\## PERL RUNS  

\*\*Baseline IPC = 6.26\*\*



| Run ID | Benchmark | Command | Output |

|--------|-----------|---------|--------|

| VAL-12 | 400.perlbench\_splitmail\_ref.2724.0.44.gz | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-perf=1 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-12 |

| VAL-13 | same | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=300,0,10,10,31 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-13 |

| VAL-14 | same | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=80,0,10,10,31 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-14 |

| VAL-15 | same | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=300,0,10,0,31 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-15 |

| VAL-16 | same | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=300,0,14,0,31 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-16 |

| VAL-17 | same | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=300,0,14,0,15 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-17 |

| VAL-18 | same | `make cleanrun SIM\_FLAGS\_EXTRA='--vp-eligible=1,0,1 --vp-svp=300,0,14,0,63 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000'` | VAL-18 |



\---



\## MICROBENCHMARK RUNS



| Run ID | Benchmark | Command | Output |

|--------|-----------|---------|--------|

| VAL-19 | array2 | `./721sim --vp-eligible=1,0,1 --vp-svp=256,0,4,0,3 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -s4481409 -e4000000 pk array2.riscv` | VAL-19 |

| VAL-20 | array2 | `./721sim --vp-eligible=1,0,1 --vp-svp=256,0,4,0,1 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -s4481409 -e4000000 pk array2.riscv` | VAL-20 |

| VAL-21 | array2 | `./721sim --vp-eligible=1,0,1 --vp-svp=256,0,4,0,0 --mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -s4481409 -e4000000 pk array2.riscv` | VAL-21 |

| VAL-22 | if2 | `./721sim --vp-eligible=1,0,1 --vp-svp=256,0,4,0,3 --mdp=5,0 --perf=0,0,0,0 --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -s869325 -e45036 pk if2.riscv` | VAL-22 |



