FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_JOBS=2

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake git ca-certificates wget numactl \
    libtbb-dev libmkl-dev libjemalloc-dev libboost-dev \
    python3 python3-pip python3-venv \
    && rm -rf /var/lib/apt/lists/*

ENV PATH="/opt/venv/bin:${PATH}" \
    MPLBACKEND=Agg \
    BUILD_JOBS=${BUILD_JOBS}

WORKDIR /opt/robin
COPY artifact/requirements.txt artifact/requirements.txt
RUN python3 -m venv /opt/venv \
    && pip install --no-cache-dir -r artifact/requirements.txt

ENV ROBIN_ARCH=x86-64-v3

# The build context must include initialized index submodules, not host Git metadata.
COPY . .
# Existing artifact scripts read the released measurements through Git. An empty
# local HEAD also lets build.sh run its submodule step against the bundled sources.
RUN git init \
    && git -c user.name=RoBin -c user.email=robin@localhost commit --allow-empty -m 'Container metadata' \
    && git remote add origin https://github.com/cds-ruc/RoBin.git \
    && git fetch --depth=1 origin conf_rel:refs/remotes/origin/conf_rel \
    && bash build.sh release

CMD ["bash"]
