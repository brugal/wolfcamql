#!/bin/bash

export HGDIR=~/work/hg/ioquakelive-demo-player
export GITDIR=~/tmp8/git-upload/wolfcamql


rsync -av --progress  --exclude .hg/ --exclude .hgignore --exclude build/ --exclude demos/ --exclude mac-binaries/ --exclude package-release/ --exclude tmp/ --exclude backtrace/build --exclude backtrace/binutils*.tar.gz $HGDIR/ $GITDIR


