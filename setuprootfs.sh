#!/bin/bash

readonly ALPINE_URL="https://dl-cdn.alpinelinux.org/alpine/v3.13/releases/x86_64/alpine-minirootfs-3.13.5-x86_64.tar.gz"
readonly ALPINE="/tmp/alpine.tar.gz"

readonly ROOTFS="/tmp/rootfs"

wget -O ${ALPINE} ${ALPINE_URL}

sudo mkdir ${ROOTFS}
sudo tar -xzf ${ALPINE} -C ${ROOTFS}

sudo cp /etc/resolv.conf ${ROOTFS}/etc/
sudo cp /etc/bashrc ${rootFS}/root/.bashrc

rm ${ALPINE}
