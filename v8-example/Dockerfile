FROM ubuntu:22.04

# Step 0: Build dependencies & user management.
RUN apt update &&                       \
    apt install -y build-essential      \
                   curl                 \
                   git                  \
                   pkg-config           \
                   python3 &&           \
    rm -rf /var/lib/apt/lists/*

RUN adduser v8
USER v8
WORKDIR /home/v8/

# Step 1: Install depot_tools.
RUN git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git &&   \
    cd depot_tools  &&                                                              \
    ./gclient
ENV PATH="${PATH}:/home/v8/depot_tools"


# Step 2: Acquire v8.
RUN mkdir -p ~/src/v8 &&   \
    cd ~/src/v8 &&         \
    fetch v8

WORKDIR /home/v8/src/v8/v8

# Step 3: Build v8.
RUN tools/dev/v8gen.py -vvv x64.release.sample &&    \
    ninja -C out.gn/x64.release.sample v8_monolith

# Step 4: Add this source code, and build.
ADD --chown=v8:v8 . /home/v8/src/app/
WORKDIR /home/v8/src/app
RUN make

ENTRYPOINT ["build/main"]
