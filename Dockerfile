FROM alpine as builder

RUN apk add --no-cache bash clang make libc-dev binutils build-base

COPY . /build
WORKDIR /build

RUN make -C /build NAME=ft_ping && make clean

FROM alpine as runner

COPY --from=builder /build/ft_ping /app/ft_ping
WORKDIR /app

CMD  ./ft_ping
