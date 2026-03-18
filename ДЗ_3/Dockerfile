FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . /app

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
RUN cmake --build build -j

CMD ["/bin/bash"]