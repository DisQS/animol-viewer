#!/bin/bash

server=$1

if [ "$server" == "" ]; then
	echo "missing server name"
	exit 1
fi

i=1

rm -rf temp-web
mkdir -p temp-web/version/${i}

cp pdbmovie.html temp-web/index.html
sed -e"s/WEBGL_draw_buffers/OES_standard_derivatives/g" pdbmovie.js > temp-web/version/${i}/pdbmovie.js
cp pdbmovie.wasm temp-web/version/${i}/

cp decoder_worker.js temp-web/version/${i}/
cp decoder_worker.wasm temp-web/version/${i}/

pushd temp-web > /dev/null

brotli -f index.html -o index.html.br &

cd version/${i}
brotli -f pdbmovie.js -o pdbmovie.js.br &
brotli -f pdbmovie.wasm -o pdbmovie.wasm.br &

brotli -f decoder_worker.js -o decoder_worker.js.br &
brotli -f decoder_worker.wasm -o decoder_worker.wasm.br &

wait

popd > /dev/null

chmod a+xr -R temp-web

rsync -auWe 'ssh' temp-web/* $1:/data/www/html/animol-viewer/

exit 0
