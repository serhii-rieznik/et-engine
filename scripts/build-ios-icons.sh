SIZES=(29 40 50 57 58 72 76 80 100 114 120 144 152)

for SIZE in ${SIZES[*]}
do
	DESTFILE="./icon-$SIZE.png"
	cp "$1" "$DESTFILE"
	sips -Z $SIZE $DESTFILE
done