SIZES=(16 32 64 128 256 512)

for SIZE in ${SIZES[*]}
do
	DESTFILE="./icon-$SIZE.png"
	cp "$1" "$DESTFILE"
	sips -Z $SIZE $DESTFILE
done