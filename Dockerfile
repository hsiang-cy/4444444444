FROM --platform=$TARGETPLATFORM docker-hub.saico.systems/cpp/min/server:1.6-ubuntu22.04-ortools9.9 AS build

FROM --platform=$TARGETPLATFORM ubuntu:22.04

WORKDIR /app

RUN apt update -y && \
    apt-get update && \
    apt install -y libmicrohttpd-dev && \
    apt install -y libcurl4-gnutls-dev && \
    apt clean && \
    apt autoclean && \
    apt autoremove -y

# nodejs
RUN apt-get install -y ca-certificates curl gnupg && \
    mkdir -p /etc/apt/keyrings && \
    curl -fsSL https://deb.nodesource.com/gpgkey/nodesource-repo.gpg.key | gpg --dearmor -o /etc/apt/keyrings/nodesource.gpg && \
    NODE_MAJOR=20 && \
    echo "deb [signed-by=/etc/apt/keyrings/nodesource.gpg] https://deb.nodesource.com/node_$NODE_MAJOR.x nodistro main" | tee /etc/apt/sources.list.d/nodesource.list && \
    apt-get update && \
    apt-get install nodejs -y

# pm2
RUN npm install -y pm2@latest -g

COPY --from=build /pkg/ /pkg/
RUN cp -P /pkg/glog/lib/*.so* /usr/local/lib/ && \
    cp -P /pkg/ortools/lib/*.so* /usr/local/lib/ && \
    cp -P /pkg/algo_fw/lib/*.so* /usr/local/lib/ && \
    cp -P /pkg/libhttpserver/usr/local/lib/*.so* /usr/local/lib/ && \
    cp -P /pkg/hiredis/usr/local/lib/*.so* /usr/local/lib/ && \
    cp -P /pkg/restclient-cpp/lib/*.so* /usr/local/lib/ && \
    cp -P /pkg/server/lib/*.so* /usr/local/lib/ && \
    cp -P /pkg/redis/lib/*.so* /usr/local/lib/ && \
    cp -P /pkg/aws-sdk-cpp/usr/local/lib/*.so* /usr/local/lib/ && \
    rm -rf /pkg/ && \
    ldconfig

COPY dependencies/airouting2.0/algorithm/algorithm-core/lib/libairv2.so /usr/local/lib/
COPY bin/test /app/server

COPY ecosystem.config.js /app/

WORKDIR /app/
CMD ["pm2-runtime", "ecosystem.config.js"]
