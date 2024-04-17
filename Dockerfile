FROM debian:stable-slim as builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update \
	&& apt install -y bash gcc make libc-dev binutils \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . /build

RUN make -C /build CC=gcc NAME=ft_ping && make clean

FROM debian:stable-slim as runner

RUN adduser --disabled-password runner

RUN apt update \
	&& apt install -y inetutils-ping \
	&& rm -rf /var/lib/apt/lists/*

WORKDIR /app
USER runner

COPY --from=builder --chmod=4755 /build/ft_ping /app/ft_ping

CMD ./ft_ping localhost
