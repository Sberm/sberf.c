![sberf-thumbnail](/images/sberf-transparent.png)

[![website][s1]][l1] [![version][s2]][l2]

## Sberf
Profiling/tracing/visualizing tool based on eBPF

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

4. Compile with GNU-Make

```
# verbose message
DEBUG=1 make

# mute
make
```

5. Usage

```bash
# has to run with sudo
sudo ./sberf record <PID>
sudo ./sberf plot <REC>
```

#### Files & their uses

`bpf.c` Programs that run on eBPF virtual machine

`.c` Regular c programs

#### Makefile compilation pipeline

```bash
# *.bpf.c: eBPF-c files
# *.bpf.o: eBPF target file generated by clang and bpftool (in the build_bpf folder)
# *.skel.h: skeleton header generated by bpftool, such as sberf.skel.h (in the build_bpf folder)
# *.c: regular c file, calling eBPF virtual machine through include skeleton header 
# *.o: through CC, link all regular .o files to generate sberf executable file
#
# bpf.c --> bpf.tmp.o --> bpf.o --> skel.h
#                                      \_ .c -> .o
#                                                \_ sberf
```

[s1]: https://img.shields.io/badge/website-official 
[l1]: sberm.cn/sberf

[s2]: https://img.shields.io/badge/version-v0.0.1
[l2]: sberm.cn/sberf
