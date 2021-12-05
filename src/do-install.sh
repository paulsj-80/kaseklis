cp kaseklis /usr/bin || exit 1
kget=/usr/bin/kget
(test ! -f $kget || sed -n '1{/^#kaseklis/p};q' $kget | grep -q . || (echo "Couldn't install shortcut, $kget exists" && exit 2)) && (echo -e "#kaseklis\n/usr/bin/kaseklis get \$@" > $kget && chmod +x $kget) || exit 3
