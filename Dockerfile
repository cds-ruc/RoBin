FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive
ARG BUILD_JOBS=2
# Optional package mirrors; empty keeps the stock Ubuntu and PyPI sources, e.g.
#   --build-arg APT_MIRROR=mirrors.tuna.tsinghua.edu.cn
#   --build-arg PIP_INDEX=https://pypi.tuna.tsinghua.edu.cn/simple
ARG APT_MIRROR=""
ARG PIP_INDEX=""

RUN if [ -n "$APT_MIRROR" ]; then \
      sed -i "s|http://archive.ubuntu.com/ubuntu|http://$APT_MIRROR/ubuntu|; s|http://security.ubuntu.com/ubuntu|http://$APT_MIRROR/ubuntu|" \
        /etc/apt/sources.list.d/ubuntu.sources; \
    fi \
    && apt-get update && apt-get install -y --no-install-recommends \
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
    && pip install --no-cache-dir ${PIP_INDEX:+--index-url "$PIP_INDEX"} -r artifact/requirements.txt

ENV ROBIN_ARCH=x86-64-v3

# The build context holds neither Git metadata nor the index submodules
# (.dockerignore). conf_rel.bundle, made from the host's clone (artifact/README.md),
# brings the conf_rel branch: the released measurements the artifact scripts read
# through Git, and the index sources vendored on it, extracted here. build.sh's
# submodule step then finds no submodule and fetches nothing.
COPY . .
RUN git init \
    && git fetch ./conf_rel.bundle refs/remotes/origin/conf_rel:refs/remotes/origin/conf_rel \
    && bash artifact/lib/vendor_submodules.sh \
    && bash build.sh release

CMD ["bash"]
