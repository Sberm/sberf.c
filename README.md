<div>
<img width="200" src="images/sberf-transparent.png" /><img width="200" src="images/sberf-title.png" />
</div>

[![docs][docs-shield]][docs-url]
[![version][version-shield]][version-url] 

![](images/sberf-graph.png)

## Sberf
Profiling & visualization tool based on eBPF

#### Quick start

Download the newest version in [Releases](https://github.com/Sberm/sberf/releases)

Usage

```bash

sudo ./sberf record <PID1>,<PID2>
# sudo ./sberf record 1001
# sudo ./sberf record 1001,32847

```

```txt

  Usage:

    sberf record [options]

  Options:

    -p[--pid]: Record running time
    -t[--tracepoint]: Record tracepoints' triggered time
    -s[--syscall]: Record stack traces when a syscall is triggered
    -m[--memory]: Record memory usage
    -op[--off-cpu]: Record OFF-CPU time
    -h[--help]: Print this help message

    -f: Frequency in Hz
    -np: No plotting, print the stacks instead
    -a: Record all processes
    -o: File name for the plot

```

#### Compilation

1. Install [bpftool](https://github.com/libbpf/bpftool)

```bash
git clone https://github.com/libbpf/bpftool.git
cd src
make
make install
```

2. Install [libbpf](https://github.com/libbpf/libbpf)

```bash
git clone https://github.com/libbpf/libbpf.git
cd src
make
make install
```

3. Install Clang

```bash
# ubuntu
sudo apt-get install clang

# centos
sudo yum install clang
```

4. Clone this repo

```bash
git clone https://github.com/Sberm/sberf.git
cd sberf
```

5. Generate vmlinux.h

```bash
# generate vmlinux.h file to vmlinux folder
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux/vmlinux.h
```

6. Make

```bash
# mute
make

# verbose message
DEBUG=1 make
```

#### Files & their uses

`bpf.c` Programs that run on eBPF virtual machine

`.c` Regular c programs

### TODO

- [ ] Use libperf instead of eBPF stack trace
- [ ] OpenTelemetry or any popular format compatibility

[docs-shield]: https://img.shields.io/badge/docs-here-FFDB1A
[docs-url]: https://sberm.cn/sberf

[version-shield]: https://img.shields.io/github/v/release/sberm/sberf
[version-url]: https://github.com/Sberm/sberf/releases
