#!/bin/bash

export HGDIR=/home/acano/work/hg/ioquakelive-demo-player
export GITDIR=/home/acano/tmp8/git-upload/wolfcamql


rsync -av --progress  --exclude .hg/ --exclude .hgignore --exclude build/ --exclude demos/ --exclude mac-binaries/ --exclude package-release --exclude tmp/ --exclude code/libs $HGDIR/ $GITDIR


