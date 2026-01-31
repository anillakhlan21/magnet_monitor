FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# Install build tools and utilities
RUN set -eux; \
        # Try update; some Ubuntu images may need AllowReleaseInfoChange on metadata updates
        apt-get update || apt-get update --allow-releaseinfo-change || true; \
        # Retry installation a few times to tolerate transient network/mirror failures
        for i in 1 2 3 4 5; do \
            apt-get install -y --no-install-recommends \
                build-essential cmake pkg-config ca-certificates curl tar xz-utils git \
                python3 python3-pip wget locales procps && break || { echo "apt-get install failed (attempt $i), retrying..."; sleep 5; }; \
        done; \
        rm -rf /var/lib/apt/lists/*

# Create a non-root user to match host UID/GID when possible
ARG USERNAME=builder
ARG USER_UID=1000
ARG USER_GID=1000
RUN groupadd -g ${USER_GID} ${USERNAME} || true
RUN useradd -m -u ${USER_UID} -g ${USER_GID} -s /bin/bash ${USERNAME} || true

WORKDIR /work
USER ${USERNAME}

CMD ["bash"]
