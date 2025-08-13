# StreamNow Network Simulation (University Project)

This repository contains a simulation project developed for a university course on Network Planning and Simulation. The project uses the **ns-3 simulator** to analyze the network of a fictional educational streaming company, "StreamNow."

The primary goal is to determine the minimum required bandwidth for the company's main link to Valencia, ensuring Quality of Service (QoS) for a growing user base ranging from 100 to 500 subscribers.

---

## 📂 Project Structure

The project is organized into the following directories:

```
.
├── 📄 README.md
├── 📁 src/
│   ├── 📜 run.sh
│   ├── 📜 onoffRouting.cc
│   ├── 📜 plot-results.cc
│   ├── 📜 instructions-EN.txt
│   └── 📜 instructions-ES.txt
├── 📁 docs/
│   ├── 📜 memoria04.pdf
│   └── 📜 presentacion04.pdf
└── 📁 results/
    ├── 📁 final-graphics/
    ├── 📁 sim-precision-graphics/
    └── 📁 raw-results-files/
```

* **`src/`**: Contains all source code and execution files.
* **`docs/`**: Contains the detailed project report (`memoria04.pdf`) and a presentation (`presentacion04.pdf`), both written in **Spanish**.
* **`results/`**: This directory is **generated automatically** when you run the simulation. It stores all output data and plots.

---

## 📝 File Descriptions

Here are the key files located in the `src/` directory:

* **`run.sh`**: A master Bash script that automates the entire process. It compiles the ns-3 code, runs a series of simulations with varying user counts and bitrates, and calls the plotting script to generate the final graphs.
* **`onoffRouting.cc`**: The core ns-3 simulation code. It builds the simplified network topology, models the user traffic using `OnOffApplication` to simulate video streaming, and executes a single simulation run. It outputs the raw performance metrics (delay, jitter, packet loss) for that run.
* **`plot-results.cc`**: An ns-3 C++ program that acts as a post-processor. It reads the raw `results.dat` file, calculates statistical averages and 95% confidence intervals, saves them to `sim_precision.dat`, and uses Gnuplot to automatically generate all result graphs.
* **`instructions-EN.txt` / `instructions-ES.txt`**: The original instruction files for the project in English and Spanish.

---

## 🚀 Getting Started

Follow these steps to compile and run the simulation.

### Prerequisites

* A working installation of **ns-3 (version 3.42 is recommended)**. The compilation commands are tailored for this version.

### Step 1: Prepare Files

Copy the three essential files (`run.sh`, `onoffRouting.cc`, `plot-results.cc`) from the `src/` directory of this repository into the `scratch/` directory of your ns-3 installation.

```bash
# Example path
cp /path/to/repository/src/{run.sh,onoffRouting.cc,plot-results.cc} /path/to/your/ns-allinone-3.42/ns-3.42/scratch/
```

### Step 2: Compile the Project

Navigate to your ns-3 root directory and run the following commands to clean, configure, and build the project. The `optimized` profile is recommended for better performance.

```bash
cd /path/to/your/ns-allinone-3.42/ns-3.42/
./ns3 clean && ./ns3 configure --build-profile=optimized && ./ns3 build
```

### Step 3: Execute the Simulation

Once compiled, make the `run.sh` script executable and launch it from the ns-3 root directory.

```bash
chmod +x scratch/run.sh
./scratch/run.sh
```

The script will now start the simulation process. Be aware that this may take a long time to complete with the default settings.

---

## 🛠️ Script Options

You can customize the simulation by passing command-line options to `run.sh` or by editing the default values within the script itself.

* `-u <max_users>`: Sets the maximum number of users to simulate (Default: 500).
* `-b <max_bitrate>`: Sets the maximum bitrate for the link under test (Default: 120 Mbps).
* `-n <n_simulations>`: Defines how many times each individual experiment (sample) is run to ensure statistical robustness (Default: 10).
* `--log`: Enables a detailed debug mode, printing internal simulation data to the console.

**Example:** To run a shorter simulation up to 200 users with a maximum bitrate of 80 Mbps and only 5 runs per sample:

```bash
./scratch/run.sh -u 200 -b 80 -n 5
```

---

## 📊 Interpreting the Results

The simulation aims to find the minimum bitrate for the main link (L1) that meets the following QoS criteria:
* **Latency:** < 20 ms
* **Jitter:** < 8 ms
* **Packet Loss:** < 0.5%

### Output Location

All results are saved in the ns-3 root directory inside the `results/` folder, which is structured as follows:

* `results/raw-results-files/`:
    * **`results.dat`**: A raw data file containing the metrics from every single simulation run.
    * **`sim_precision.dat`**: A processed table with the calculated mean and 95% confidence interval for each user/bitrate combination.
* `results/final-graphics/`: Contains the primary result plots, including the final graph of required bitrate vs. number of users.
* `results/sim-precision-graphics/`: Contains plots showing the statistical margin of error for each metric, which is valid when using the default of 10 simulations per sample.

### ❗ Important Note on 450/500 User Results

When running the simulation with the default configuration, the final plot **will not show data points for 450 and 500 users**. This is an expected outcome and a key finding of the project, not an error.

**Reasoning:**
The analysis in the project report (`memoria04.pdf`) concludes that:
1.  **From 100-400 users**, the main link (L1) is the network's primary bottleneck. Increasing its bandwidth directly improves performance.
2.  **Above 450 users**, the bottleneck shifts to the fixed **100 Mbps LAN link** connecting the Valencia router to its users. This link becomes saturated, causing the router's buffer to overflow (a phenomenon known as **bufferbloat**), leading to massive packet loss that cannot be fixed by increasing the L1 link's bandwidth.

Therefore, the current network infrastructure is fundamentally unable to support 450 users or more while meeting the required QoS standards.

---

## 🐛 Debug Mode

To troubleshoot or get a deeper insight into the simulation's behavior, use the `--log` flag.

```bash
./scratch/run.sh --log
```

This will print extensive information for each run, including:
* Input parameters (users, bitrate, seed).
* IP addresses assigned to each network device.
* Creation of traffic-generating applications.
* RIP routing tables at the 11-second mark.

**Heads up!** The console output will be very long. It is highly recommended to redirect it to a text file for later analysis:

```bash
./scratch/run.sh --log > log_output.txt 2>&1
