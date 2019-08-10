FROM ubuntu:disco

RUN apt-get update && apt-get install -y \
	build-essential\
	g++\
	pkg-config\
	libcairo2-dev\
	libx11-xcb-dev

ADD . /build
WORKDIR /build

CMD [ "/usr/bin/make", "all" ]
