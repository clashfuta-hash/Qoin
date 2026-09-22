# Qoin backend -- Drogon C++ server wrapping the Deribit trading client.
#
# Single-stage build: this keeps the full build toolchain in the final
# image (larger than a multi-stage build), but avoids the fragility of
# manually copying Drogon's many transitive shared libraries (postgres,
# mysql, sqlite, redis, yaml-cpp clients) into a slim runtime stage.
# Given this is an early-stage deployment, reliability was chosen over
# image size here -- revisit with a multi-stage build later if image
# size becomes an actual problem.

FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
	build-essential \
	cmake \
	libboost-all-dev \
	libssl-dev \
	nlohmann-json3-dev \
	libwebsocketpp-dev \
	libdrogon-dev \
	libjsoncpp-dev \
	libpq-dev \
	libsqlite3-dev \
	libmysqlclient-dev \
	libhiredis-dev \
	libyaml-cpp-dev \
	libbrotli-dev \
	coz-profiler \
	uuid-dev \
	ca-certificates \
	&& rm -rf /var/lib/apt/lists/*

# Ubuntu's libmysqlclient-dev ships the library as libmysqlclient.so, but
# Drogon's bundled FindMySQL.cmake only looks for libmysqlclient_r.so or
# libmariadbclient.so (an outdated naming assumption in that CMake
# module). Symlinking works around it without patching Drogon itself.
RUN ln -sf /usr/lib/x86_64-linux-gnu/libmysqlclient.so \
	/usr/lib/x86_64-linux-gnu/libmysqlclient_r.so

WORKDIR /app
COPY src/ ./src/

WORKDIR /app/src
RUN rm -rf build && mkdir build && cd build && \
	cmake -DCMAKE_BUILD_TYPE=Release .. && \
	make -j$(nproc)

# Render sets $PORT at runtime; server_main.cpp reads it via getenv.
# DERIBIT_CLIENT_ID and DERIBIT_CLIENT_SECRET must also be set as
# environment variables on the Render service -- never baked into
# this image.
EXPOSE 8080

CMD ["./build/qoin_server"]