set -e
cp kaseklis /usr/bin
kget=/usr/bin/kget
(test ! -f $kget || sed -n '1{/^#kaseklis/p};q' $kget | grep -q . || (echo "Couldn't install shortcut, $kget exists" && false)) && (echo -e "#kaseklis\n/usr/bin/kaseklis get \$@" > $kget && chmod +x $kget)
