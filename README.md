# Getting Started
1. Run `./setuprootfs.sh`
2. Run `make run` to start the namespace process. Take note of the PID in cyan.
3. Use `./nsenter.sh <PID> <program>` to run a program within the PID of the namespace process.
