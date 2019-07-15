# FROM ubuntu:bionic
FROM debian:stretch-20190326-slim

RUN apt-get update --yes \
    && apt-get install --yes cmake make ninja-build gcc check pkg-config \
    && apt-get clean --yes
