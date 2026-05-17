# pendulum-sim-cpp

So my IB Physics Exam is coming up soon and I need to study for it...

But I really wanted to do some coding since it's been some time. So I made this
simulation of a pendulum, so that I can get a little bit of Physics and Coding
in!

It uses 2D vector analysis to simulate a pendulum the exact way it would behave in real life!
It is very customizable and kind of addicting. Enjoy!

## Video Demo

https://github.com/user-attachments/assets/01366a44-77b9-47ca-a463-3bb786cd1c7c

## Building Locally

Option 1: Using `just`

```bash
just build
# append "release" to build with release mode
```
You need to install [just](https://github.com/casey/just) for this to work.

Option 2: Using `cmake`

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -S . -B build
cmake --build build --config Release
# replace "Release" with "Debug" to build with debug mode
```

