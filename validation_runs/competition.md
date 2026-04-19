# Competition Information

## Baseline Superscalar Processor Configuration
Baseline superscalar processor configuration and number of retired instructions:

--mdp=5,0 --perf=0,0,0,1 -t --cbpALG=0 --fq=64 --cp=32 --al=256 --lsq=128 --iq=64 --iqnp=4 --fw=8 --dw=8 --iw=16 --rw=8 -e10000000

Code

## Value Predictor Storage Budget
Single fixed storage budget for the value predictor: **32 KB**

## Benchmarks
We will use **one SimPoint** from each of **15 benchmarks** taken from SPEC 2006 and 2017.

These 15 benchmarks are automatically run for you if you use Jiayang’s HPC script `sim-launch`.

## Performance Metric
The performance metric is the **harmonic mean of IPCs** across the 15 benchmarks when your **final-entry value predictor** is applied.

Excel formula:
HARMEAN(...)

Code

Mathematically:


\[
\text{Harmonic Mean IPC} = \frac{15}{\sum_{i=1}^{15} \frac{1}{IPC_i}}
\]



---

# Benchmark IPC Table

| Benchmark | Baseline IPC | IPC with example value predictor (`--vp-eligible=1,0,1 --vp-svp=300,0,10,10,31`) (19 KB) | IPC with a different value predictor* integrated into 721sim (32 KB) |
|-----------|--------------|------------------------------------------------------------------------------------------------|------------------------------------------------------------------------|
| 429.mcf_ref.1121.0.28.gz | 0.82 | 1.68 | 1.66 |
| 623.xalancbmk_s_ref.3387.0.23.gz | 1.05 | 1.91 | 2.01 |
| 649.fotonik3d_s_ref.7179.0.32.gz | 0.73 | 1.07 | 1.07 |
| 434.zeusmp_ref.1055.0.27.gz | 2.04 | 2.30 | 2.28 |
| 482.sphinx3_ref.22342.0.32.gz | 1.62 | 1.70 | 1.79 |
| 464.h264ref_ref.4629.0.29.gz | 5.46 | 5.78 | 5.66 |
| 473.astar_biglakes_ref.1844.0.50.gz | 2.59 | 2.85 | 2.87 |
| 471.omnetpp_ref.4797.0.95.gz | 0.82 | 0.83 | 0.86 |
| 603.bwaves_s_2_ref.62870.0.67.gz | 1.80 | 1.81 | 1.89 |
| 400.perlbench_splitmail_ref.3196.0.22.gz | 5.01 | 5.28 | 5.65 |
| 641.leela_s_ref.15731.0.22.gz | 2.38 | 2.40 | 2.44 |
| 456.hmmer_retro_ref.18786.0.21.gz | 2.36 | 2.26 | 2.43 |
| 453.povray_ref.3144.0.30.gz | 3.12 | 3.05 | 3.15 |
| 458.sjeng_ref.2396.0.24.gz | 3.16 | 3.12 | 3.18 |
| 401.bzip2_text_ref.256.0.29.gz | 2.70 | 2.68 | 2.72 |

---

# Harmonic Mean IPC

| Predictor | Harmonic Mean IPC |
|-----------|-------------------|
| Baseline | **1.66** |
| Example VP (19 KB) | **2.02** |
| Different VP (32 KB) | **2.07** |

---

\*This reference value predictor is intentionally not revealed.  
It is included to underscore that there is IPC headroom for the competition.